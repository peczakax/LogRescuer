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
    std::mutex streamMutex;   // Mutex for thread-safe console output
    
    std::unordered_map<std::string, std::string> hashToPathMap;  // Maps hash to the first file with that hash
    std::unordered_map<std::string, std::string> pathToHashMap;  // Maps each file path to its hash
    
    // Process files in parallel using thread pool
    threadPool.parallelFor(filePaths.begin(), filePaths.end(), 
        [&](auto fileIt, size_t) {
            const auto& filePath = *fileIt;
            std::string relativePath = std::filesystem::relative(filePath, rootPath).string();  // Get path relative to root
            
            // Skip empty files
            if (std::filesystem::file_size(filePath) == 0) {
                return;
            }
            
            std::string hash = hashutils::computeSHA256FromFile(filePath.string());  // Calculate file hash
            
            std::lock_guard<std::mutex> lock(hashMapMutex);  // Thread-safe updates to maps
            pathToHashMap[relativePath] = hash;  // Store hash for each file
            if (hashToPathMap.find(hash) == hashToPathMap.end()) {
                hashToPathMap[hash] = relativePath;  // Store first occurrence of each hash
            }
        });
    
    return {hashToPathMap, pathToHashMap};
}

std::vector<meta::FileMeta> 
FileCompressor::compressFiles(const std::string& inputDir,
                            std::ofstream& archive,
                            const Compressor& compressor) {
    
    threading::ThreadPool& threadPool = threading::ThreadPool::getInstance();
    std::vector<meta::FileMeta> metadata;
    
    // Compute hashes first
    auto filePaths = io::scanDirectory(inputDir);
    auto [hashToPathMap, pathToHashMap] = computeHashes(filePaths, std::filesystem::path(inputDir));
    
    // Internal structure for grouping files
    struct FileGroup {
        std::vector<std::pair<std::filesystem::path, std::string>> uniqueFiles;
        std::vector<std::pair<std::filesystem::path, std::string>> duplicateFiles;
    };

    // Group files by uniqueness
    FileGroup fileGroup;
    std::filesystem::path rootPath(inputDir);
    
    for (const auto& filePath : filePaths) {        
        if (std::filesystem::file_size(filePath) == 0) {
            std::cout << "Skipping empty file: " << filePath << std::endl;
            continue;
        }
        
        std::string relativePath = std::filesystem::relative(filePath, rootPath).string();
        std::string hash = pathToHashMap.at(relativePath);
        
        if (hashToPathMap.at(hash) == relativePath) {
            fileGroup.uniqueFiles.push_back({filePath, relativePath});
        } else {
            fileGroup.duplicateFiles.push_back({filePath, relativePath}); 
        }
    }
    
    metadata.reserve(fileGroup.uniqueFiles.size() + fileGroup.duplicateFiles.size());
    
    std::mutex archiveMutex;
    std::mutex hashOffsetMutex;
    std::mutex metadataMutex;
    std::mutex streamMutex;
    std::unordered_map<std::string, uint64_t> hashToOffsetMap;

    // Process unique files in parallel
    threadPool.parallelFor(fileGroup.uniqueFiles.begin(), fileGroup.uniqueFiles.end(),
        [&](auto fileIt, size_t) {
            const auto& [filePath, relativePath] = *fileIt;
            uint64_t fileSize = std::filesystem::file_size(filePath);  // Get original file size            
            // Special case for empty files
            if (fileSize == 0) {
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
                compressor.compressStream(inputFile, archive);
                
                // Calculate the size of the compressed data
                compressedSize = archive.tellp() - startPos;
            }
            
            {
                std::scoped_lock lock(hashOffsetMutex, metadataMutex);  // Thread-safe update
                std::string hash = pathToHashMap.at(relativePath);  // Get file hash
                hashToOffsetMap[hash] = dataOffset;  // Store data location by hash
                meta::FileMeta meta(dataOffset, hash, relativePath);  // Create metadata for file
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
            
            {
                std::lock_guard<std::mutex> lock(metadataMutex);  // Thread-safe metadata update
                std::string hash = pathToHashMap.at(relativePath);  // Get file hash
                meta::FileMeta meta(-1, hash, relativePath);  // Create metadata for duplicate file
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
    std::ofstream archive(outputFile, std::ios::binary);
    io::checkOpen(archive, outputFile, "Archive creation");
    
    auto compressor = createCompressor(compType);
    auto metadata = compressFiles(rootDir, archive, *compressor);
    io::writeMetadata(archive, metadata, compType);
    displayStats(metadata);
}

void FileCompressor::displayStats(const std::vector<meta::FileMeta>& metadata) {
    uint32_t uniqueCount = 0;  // Counter for unique files
    uint32_t duplicateCount = 0;  // Counter for duplicate files
    
    for (const auto& meta : metadata) {
        if (meta.isOriginal()) {
            uniqueCount++;  // Files with original content
        } else if (meta.isDuplicate()) {
            duplicateCount++;  // Files that are duplicates
        }
    }
    
    std::cout << "Total files in archive: " << metadata.size() << std::endl;
    std::cout << "Unique files: " << uniqueCount << ", Duplicate files: " << duplicateCount << std::endl;
}

void FileCompressor::decompressFiles(const std::string& outputDir,
                                     std::ifstream& archive,
                                     const Compressor& decompressor,
                                     const std::vector<meta::FileMeta>& metadata) {
    std::filesystem::create_directories(outputDir);
    threading::ThreadPool& threadPool = threading::ThreadPool::getInstance();
    std::mutex archiveMutex, outputMutex;
    
    // Group files by hash
    std::unordered_map<std::string, std::vector<const meta::FileMeta*>> filesByHash;
    for (const auto& meta : metadata) {
        filesByHash[meta.hash].push_back(&meta);
    }
    
    // Track successfully decompressed original files
    std::unordered_map<std::string, std::string> hashToPathMap;
    
    // First extract all original files
    threadPool.parallelFor(filesByHash.begin(), filesByHash.end(), [&](auto it, size_t) {
        const auto& hash = it->first;
        const auto& files = it->second;
        
        // Find the original file
        const meta::FileMeta* originalFileMeta = nullptr;
        for (const auto* meta : files) {
            if (meta->isOriginal()) {
                originalFileMeta = meta;
                break;
            }
        }
        
        if (!originalFileMeta) {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "Error: No original file found for hash " << hash << std::endl;
            return;
        }
        
        // Extract the original file
        std::filesystem::path outputPath = std::filesystem::path(outputDir) / originalFileMeta->relativePath;
        std::filesystem::create_directories(outputPath.parent_path());
        
        {
            std::lock_guard<std::mutex> lock(archiveMutex);
            archive.clear();
            archive.seekg(originalFileMeta->dataOffset);
            
            std::ofstream outputFile(outputPath, std::ios::binary);
            io::checkOpen(outputFile, outputPath.string(), "Output file creation");
            decompressor.decompressStream(archive, outputFile);
        }
        
        // Verify hash
        if (hashutils::computeSHA256FromFile(outputPath.string()) != hash) {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "Error: Hash mismatch for " << originalFileMeta->relativePath << std::endl;
            std::filesystem::remove(outputPath);
            return;
        }
        
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            hashToPathMap[hash] = outputPath.string();
            std::cout << "Extracted: " << originalFileMeta->relativePath << std::endl;
        }
    });
    
    // Then handle duplicates
    threadPool.parallelFor(filesByHash.begin(), filesByHash.end(), [&](auto it, size_t) {
        const auto& hash = it->first;
        const auto& files = it->second;
        
        std::string sourcePath;
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            if (hashToPathMap.find(hash) == hashToPathMap.end()) {
                return; // Skip if original extraction failed
            }
            sourcePath = hashToPathMap[hash];
        }
        
        // Copy for all duplicates
        for (const auto* meta : files) {
            if (meta->isDuplicate()) {
                std::filesystem::path outputPath = std::filesystem::path(outputDir) / meta->relativePath;
                std::filesystem::create_directories(outputPath.parent_path());
                
                try {
                    std::filesystem::copy_file(sourcePath, outputPath, 
                                              std::filesystem::copy_options::overwrite_existing);
                    
                    std::lock_guard<std::mutex> lock(outputMutex);
                    std::cout << "Extracted duplicate: " << meta->relativePath << std::endl;
                }
                catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(outputMutex);
                    std::cout << "Error copying to " << meta->relativePath << ": " << e.what() << std::endl;
                }
            }
        }
    });
}

void FileCompressor::decompress(const std::string& archiveFile, const std::string& outputDir) {
    std::ifstream archive(archiveFile, std::ios::binary);  // Open archive file
    io::checkOpen(archive, archiveFile, "Archive reading");

    CompressionType compType;
    auto metadata = io::readMetadata(archive, compType);  // Read file metadata and compression type
    auto decompressor = createCompressor(compType);  // Create appropriate decompressor
    decompressFiles(outputDir, archive, *decompressor, metadata);  // Extract files
    displayStats(metadata);  // Show statistics
}

}
