#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <future>
#include <mutex>

#include "CompressorFactory.h"
#include "FileCompressor.h"
#include "FileMeta.h"
#include "HashUtils.h"
#include "IO.h"
#include "ThreadPool.h"

namespace compression {

std::pair<std::unordered_map<std::string, std::string>, std::unordered_map<std::string, std::string>>
FileCompressor::computeHashes(const std::vector<std::filesystem::path>& filePaths, const std::filesystem::path& rootPath) {
    threading::ThreadPool& threadPool = threading::ThreadPool::getInstance();  // Get thread pool for parallel processing
    std::mutex hashMapMutex;  // Mutex for thread-safe access to hash maps
    
    std::unordered_map<std::string, std::string> hashToPathMap;  // Maps hash to the first file with that hash
    std::unordered_map<std::string, std::string> pathToHashMap;  // Maps each file path to its hash
    
    // Process files in parallel using thread pool
    threadPool.parallelFor(filePaths.begin(), filePaths.end(), 
        [&](auto fileIt, size_t) {
            const auto& filePath = *fileIt;
            std::string relativePath = std::filesystem::relative(filePath, rootPath).string();  // Get path relative to root
            std::string hash = hashutils::computeSHA256FromFile(filePath.string());  // Calculate file hash
            
            std::lock_guard<std::mutex> lock(hashMapMutex);  // Thread-safe updates to maps
            pathToHashMap[relativePath] = hash;  // Store hash for each file
            if (hashToPathMap.find(hash) == hashToPathMap.end()) {
                hashToPathMap[hash] = relativePath;  // Store first occurrence of each hash
            }
        });
    
    return {hashToPathMap, pathToHashMap};
}

FileCompressor::FileGroup 
FileCompressor::groupFiles(const std::vector<std::filesystem::path>& filePaths,
                         const std::filesystem::path& rootPath,
                         const std::unordered_map<std::string, std::string>& pathToHashMap,
                         const std::unordered_map<std::string, std::string>& hashToPathMap) {
    FileGroup result;  // Container for grouped files
    
    // Categorize each file as unique or duplicate based on its hash
    for (const auto& filePath : filePaths) {
        std::string relativePath = std::filesystem::relative(filePath, rootPath).string();
        std::string hash = pathToHashMap.at(relativePath);  // Get hash for this file
        
        if (hashToPathMap.at(hash) == relativePath) {
            result.uniqueFiles.push_back({filePath, relativePath});  // First occurrence of hash is unique
        } else {
            result.duplicateFiles.push_back({filePath, relativePath});  // Additional occurrences are duplicates
        }
    }
    
    return result;
}

std::vector<meta::FileMeta> 
FileCompressor::processFiles(const FileGroup& fileGroup,
                           std::ofstream& archive,
                           const std::unordered_map<std::string, std::string>& pathToHashMap,
                           CompressionType compType) {
    
    threading::ThreadPool& threadPool = threading::ThreadPool::getInstance();  // Get thread pool
    std::vector<meta::FileMeta> metadata;  // Container for file metadata
    metadata.reserve(fileGroup.uniqueFiles.size() + fileGroup.duplicateFiles.size());  // Pre-allocate space
    
    std::mutex archiveMutex;  // Mutex for thread-safe archive writing
    std::mutex hashOffsetMutex;  // Mutex for thread-safe hash-to-offset map updates
    std::mutex metadataMutex;  // Mutex for thread-safe metadata vector updates
    std::mutex streamMutex;  // Mutex for thread-safe console output
    std::unordered_map<std::string, uint64_t> hashToOffsetMap;  // Maps hash to offset in archive
    
    // Create compressor once and reuse it
    auto compressor = createCompressor(compType);
    
    // Process unique files in parallel
    threadPool.parallelFor(fileGroup.uniqueFiles.begin(), fileGroup.uniqueFiles.end(),
        [&](auto fileIt, size_t) {
            const auto& [filePath, relativePath] = *fileIt;
            std::string hash = pathToHashMap.at(relativePath);  // Get file hash
            uint64_t fileSize = std::filesystem::file_size(filePath);  // Get original file size

            // Special case for empty files
            if (fileSize == 0) {
                uint64_t emptyFileOffset;
                {
                    std::scoped_lock lock(archiveMutex, hashOffsetMutex, metadataMutex);  // Thread-safe archive write
                    emptyFileOffset = archive.tellp();
                    hashToOffsetMap[hash] = emptyFileOffset;  // Store data location by hash
                    meta::FileMeta meta(relativePath);  // Create metadata for file
                    meta.hash = hash;
                    meta.dataOffset = emptyFileOffset;
                    meta.compressedSize = 0;
                    meta.originalSize = 0;
                    metadata.push_back(std::move(meta));
                }
                
                return;
            }

            uint64_t dataOffset;
            uint64_t compressedSize;
            {
                std::lock_guard<std::mutex> lock(archiveMutex);  // Thread-safe archive write
                dataOffset = archive.tellp();  // Get current position in archive
                
                // Open file for streaming
                std::ifstream inputFile(filePath.string(), std::ios::binary);
                io::checkOpen(inputFile, filePath.string(), "Compression");
                
                // Get current archive position to calculate compressed size later
                uint64_t startPos = archive.tellp();
                
                // Stream compress the file directly into the archive
                compressor->compressStream(inputFile, archive);
                
                // Calculate the size of the compressed data
                compressedSize = archive.tellp() - startPos;
            }
            
            {
                std::scoped_lock lock(hashOffsetMutex, metadataMutex);  // Thread-safe update
                hashToOffsetMap[hash] = dataOffset;  // Store data location by hash
                meta::FileMeta meta(relativePath);  // Create metadata for file
                meta.hash = hash;
                meta.dataOffset = dataOffset;
                meta.compressedSize = compressedSize;
                meta.originalSize = fileSize;
                metadata.push_back(std::move(meta));  // Add to metadata collection
            }
            
            {
                std::lock_guard<std::mutex> lock(streamMutex);  // Thread-safe console output
                std::cout << "Compressed file: " << relativePath 
                      << " (" << fileSize << " -> " << compressedSize << " bytes)" << std::endl;
            }
        });

    // Process duplicate files in parallel
    threadPool.parallelFor(fileGroup.duplicateFiles.begin(), fileGroup.duplicateFiles.end(),
        [&](auto fileIt, size_t) {
            const auto& [filePath, relativePath] = *fileIt;
            std::string hash = pathToHashMap.at(relativePath);  // Get file hash
            uint64_t fileSize = std::filesystem::file_size(filePath);  // Get original file size
            uint64_t dataOffset = hashToOffsetMap[hash];  // Find offset of already stored content
            
            {
                std::lock_guard<std::mutex> lock(metadataMutex);  // Thread-safe metadata update
                meta::FileMeta meta(relativePath);  // Create metadata for duplicate file
                meta.hash = hash;
                meta.dataOffset = dataOffset;  // Point to existing data
                meta.compressedSize = 0;  // Mark as duplicate (no unique compressed data)
                meta.originalSize = fileSize;
                metadata.push_back(std::move(meta));  // Add to metadata collection
            }
            
            {
                std::lock_guard<std::mutex> lock(streamMutex);  // Thread-safe console output
                std::cout << "Duplicate file: " << relativePath << std::endl;
            }
        });

    return metadata;
}

void FileCompressor::compress(const std::string& rootDir, const std::string& outputFile, CompressionType compType) {
    std::ofstream archive(outputFile, std::ios::binary);  // Create output archive file
    io::checkOpen(archive, outputFile, "Archive creation");
    io::write(archive, compType);  // Write compression type to archive header
    
    std::filesystem::path rootPath(rootDir);  // Root directory for relative paths
    auto filePaths = io::scanDirectory(rootDir);  // Find all files to compress
    auto [hashToPathMap, pathToHashMap] = computeHashes(filePaths, rootPath);  // Calculate file hashes
    auto fileGroup = groupFiles(filePaths, rootPath, pathToHashMap, hashToPathMap);  // Group by uniqueness
    auto metadata = processFiles(fileGroup, archive, pathToHashMap, compType);  // Process and compress files
    io::writeMetadata(archive, metadata);  // Write metadata to end of archive
    
    std::cout << "Archive created successfully: " << outputFile << std::endl;
}

std::unordered_map<std::string, std::vector<std::reference_wrapper<const meta::FileMeta>>>
FileCompressor::groupMetadata(const std::vector<meta::FileMeta>& metadata) {
    std::unordered_map<std::string, std::vector<std::reference_wrapper<const meta::FileMeta>>> hashToMetaMap;
    
    for (const auto& meta : metadata) {
        hashToMetaMap[meta.hash].push_back(std::cref(meta));  // Group files by hash
    }
    
    return hashToMetaMap;
}

void FileCompressor::displayStats(const std::vector<meta::FileMeta>& metadata) {
    uint32_t uniqueCount = 0;  // Counter for unique files
    uint32_t duplicateCount = 0;  // Counter for duplicate files
    
    for (const auto& meta : metadata) {
        if (meta.compressedSize > 0) {
            uniqueCount++;  // Files with compressed content are unique
        } else {
            duplicateCount++;  // Files without compressed content are duplicates
        }
    }
    
    std::cout << "Total files in archive: " << metadata.size() << std::endl;
    std::cout << "Unique files: " << uniqueCount << ", Duplicate files: " << duplicateCount << std::endl;
}

void FileCompressor::decompressFiles(std::ifstream& archive,
                                     const std::string& outputDir,
                                     const std::unordered_map<std::string, std::vector<std::reference_wrapper<const meta::FileMeta>>>& groupedMeta,
                                     std::unique_ptr<Compressor>& decompressor) {
    std::filesystem::create_directories(outputDir);  // Ensure output directory exists
    threading::ThreadPool& threadPool = threading::ThreadPool::getInstance();  // Get thread pool

    std::mutex archiveMutex;  // Mutex for thread-safe archive reading
    std::mutex streamMutex;  // Mutex for thread-safe console output
    
    // Convert map to vector for parallel processing
    std::vector<std::pair<std::string, std::vector<std::reference_wrapper<const meta::FileMeta>>>> workItems(
        groupedMeta.begin(), groupedMeta.end()
    );
    
    // Process each hash group in parallel
    threadPool.parallelFor(workItems.begin(), workItems.end(), [&](auto itemIt, size_t) {
        
            const auto& [hash, metaRefs] = *itemIt;
            
            // Check if this group contains empty files (originalSize=0, compressedSize=0)
            bool hasEmptyFiles = false;
            for (const meta::FileMeta& meta : metaRefs) {
                if (meta.originalSize == 0) {
                    hasEmptyFiles = true;
                    break;
                }
            }
            
            const meta::FileMeta* originalMeta = nullptr;
            for (const meta::FileMeta& meta : metaRefs) {
                if (meta.compressedSize > 0) {
                    originalMeta = &meta;  // Find the original file metadata
                    break;
                }
            }
            
            // If we couldn't find an original file and this isn't an empty file group, report error
            if (!originalMeta && !hasEmptyFiles) {
                std::lock_guard<std::mutex> lock(streamMutex);
                std::cout << "Error: Could not find original file for hash " << hash << std::endl;
                return;
            }
            
            // For each file sharing this hash (original and duplicates)
            for (const meta::FileMeta& meta : metaRefs) {
                // Create output path and ensure directories exist
                std::filesystem::path outputPath = std::filesystem::path(outputDir) / meta.relativePath;
                std::filesystem::create_directories(outputPath.parent_path());
                
                // Special case for empty files
                if (meta.originalSize == 0) {
                    // Just create an empty file
                    std::ofstream emptyFile(outputPath, std::ios::binary);
                    emptyFile.close();
                    continue;
                }

                // Decompress the original file directly to the output path
                {
                    std::lock_guard<std::mutex> lock(archiveMutex);  // Thread-safe archive read
                    archive.clear();
                    archive.seekg(originalMeta->dataOffset);  // Seek to compressed data
                    
                    if (!archive.good()) {
                        std::cout << "Error seeking to position " << originalMeta->dataOffset << " for file " 
                                  << meta.relativePath << std::endl;
                        return;
                    }
                    
                    // Create output file
                    std::ofstream outputFile(outputPath, std::ios::binary);
                    io::checkOpen(outputFile, outputPath.string(), "Output file creation");
                    
                    // Stream decompress directly to the file
                    size_t totalBytesDecompressed = decompressor->decompressStream(archive, outputFile);

                    // Verify original size if provided
                    if (originalMeta->originalSize > 0 && totalBytesDecompressed != originalMeta->originalSize) {
                        throw std::runtime_error("Decompressed size mismatch. Expected: " + 
                                            std::to_string(originalMeta->originalSize) + ", Actual: " + 
                                            std::to_string(totalBytesDecompressed));
                    }
                }
                
                // Verify the hash of decompressed data
                std::string computedHash = hashutils::computeSHA256FromFile(outputPath.string());
                if (computedHash != hash) {
                    std::lock_guard<std::mutex> lock(streamMutex);
                    std::cout << "Error: Hash mismatch for file " << meta.relativePath << std::endl;
                    std::cout << "Expected: " << hash << std::endl;
                    std::cout << "Got: " << computedHash << std::endl;
                    std::filesystem::remove(outputPath);  // Remove corrupted file
                    continue;
                }
                
                {
                    std::lock_guard<std::mutex> lock(streamMutex);  // Thread-safe console output
                    std::cout << (meta.compressedSize > 0 ? "Extracted original" : "Extracted duplicate")
                              << " file: " << meta.relativePath 
                              << " (Size: " << meta.originalSize << " bytes, Hash verified)" << std::endl;
                }
            }
        });
}

void FileCompressor::decompress(const std::string& archiveFile, const std::string& outputDir) {
    std::ifstream archive(archiveFile, std::ios::binary);  // Open archive file
    io::checkOpen(archive, archiveFile, "Archive reading");

    CompressionType compType;
    io::read(archive, compType);  // Read compression type from header
    auto metadata = io::readMetadata(archive);  // Read file metadata
    auto groupedMeta = groupMetadata(metadata);  // Group files by hash
    auto decompressor = createCompressor(compType);  // Create appropriate decompressor
    decompressFiles(archive, outputDir, groupedMeta, decompressor);  // Extract files
    displayStats(metadata);  // Show statistics
}

}
