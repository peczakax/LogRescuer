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
FileCompressor::compressFiles(const std::string& inputDir, std::ofstream& archive, CompressionType compType) {

    threading::ThreadPool& threadPool = threading::ThreadPool::getInstance();  // Get thread pool for parallel processing
    std::vector<meta::FileMeta> metadata;  // Container for file metadata
    
    // Scan directory and compute file hashes
    auto filePaths = io::scanDirectory(inputDir);  // Get all files in the input directory
    auto [hashToPathMap, pathToHashMap] = computeHashes(filePaths, std::filesystem::path(inputDir));  // Calculate file hashes
    
    auto compressor = createCompressor(compType);  // Create appropriate compressor based on compression type

    // Containers to separate unique files from duplicates
    std::vector<std::pair<std::filesystem::path, std::string>> uniqueFiles;  // Stores unique files with their relative paths
    std::vector<std::pair<std::filesystem::path, std::string>> duplicateFiles;  // Stores duplicate files with their relative paths

    std::filesystem::path rootPath(inputDir);  // Base path for calculating relative paths
    
    // Classify files as either unique or duplicates based on their hashes
    for (const auto& filePath : filePaths) {
        if (std::filesystem::file_size(filePath) == 0) {  // Skip empty files
            continue;
        }
        
        std::string relativePath = std::filesystem::relative(filePath, rootPath).string();  // Get path relative to root
        std::string hash = pathToHashMap.at(relativePath);  // Get file hash
        
        if (hashToPathMap.at(hash) == relativePath) {  // If this is the first occurrence of this hash
            uniqueFiles.push_back({filePath, relativePath});  // Add to unique files
        } else {
            duplicateFiles.push_back({filePath, relativePath});  // Add to duplicate files
        }
    }
    
    metadata.reserve(uniqueFiles.size() + duplicateFiles.size());  // Pre-allocate metadata storage
    
    // Mutexes for thread-safe operations
    std::mutex archiveMutex;  // Protects archive write operations
    std::mutex hashOffsetMutex;  // Protects hash-to-offset map access
    std::mutex metadataMutex;  // Protects metadata collection updates
    std::mutex streamMutex;  // Protects console output
    std::unordered_map<std::string, uint64_t> hashToOffsetMap;  // Maps file hash to its offset in the archive

    // Process unique files in parallel
    threadPool.parallelFor(uniqueFiles.begin(), uniqueFiles.end(),
        [&](auto fileIt, size_t) {
            const auto& [filePath, relativePath] = *fileIt;  // Extract file path and relative path
            uint64_t fileSize = std::filesystem::file_size(filePath);  // Get original file size            
            if (fileSize == 0) {  // Skip empty files
                return;
            }
            
            uint64_t dataOffset;  // Position in archive where file data begins
            uint64_t compressedSize;  // Size of compressed data
            {
                std::lock_guard<std::mutex> lock(archiveMutex);  // Thread-safe archive write
                dataOffset = archive.tellp();  // Get current position in archive
                
                // Open file for streaming
                std::ifstream inputFile(filePath.string(), std::ios::binary);  // Open file in binary mode
                io::checkOpen(inputFile, filePath.string(), "Compression");  // Verify file opened successfully
                
                // Get current archive position to calculate compressed size later
                uint64_t startPos = archive.tellp();  // Record starting position
                
                // Stream compress the file directly into the archive
                compressor->compressStream(inputFile, archive);  // Compress and write file to archive
                
                // Calculate the size of the compressed data
                compressedSize = archive.tellp() - startPos;  // Calculate bytes written
            }
            
            {
                std::scoped_lock lock(hashOffsetMutex, metadataMutex);  // Thread-safe update to multiple resources
                std::string hash = pathToHashMap.at(relativePath);  // Get file hash
                hashToOffsetMap[hash] = dataOffset;  // Store data location by hash
                meta::FileMeta meta(dataOffset, hash, relativePath);  // Create metadata for file
                metadata.push_back(std::move(meta));  // Add to metadata collection
            }
            
            {
                std::lock_guard<std::mutex> lock(streamMutex);  // Thread-safe console output
                std::cout << "Compressed file: " << relativePath 
                      << " (" << fileSize << " -> " << compressedSize << " bytes)" << std::endl;  // Log compression results
            }
        });

    // Process duplicate files in parallel
    threadPool.parallelFor(duplicateFiles.begin(), duplicateFiles.end(),
        [&](auto fileIt, size_t) {
            const auto& [filePath, relativePath] = *fileIt;  // Extract file path and relative path
            
            {
                std::lock_guard<std::mutex> lock(metadataMutex);  // Thread-safe metadata update
                std::string hash = pathToHashMap.at(relativePath);  // Get file hash
                uint64_t dataOffset = hashToOffsetMap.at(hash);  // Get data location from original file
                meta::FileMeta meta(dataOffset, "", relativePath);  // Create metadata for duplicate file
                metadata.push_back(std::move(meta));  // Add to metadata collection
            }
            
            {
                std::lock_guard<std::mutex> lock(streamMutex);  // Thread-safe console output
                std::cout << "Duplicate file: " << relativePath << std::endl;  // Log duplicate file
            }
        });

    io::writeMetadata(archive, metadata, compType);  // Write metadata and compression type to archive
    
    return std::move(metadata);  // Return metadata for statistics
}

void FileCompressor::compress(const std::string& rootDir, const std::string& outputFile, CompressionType compType) {
    std::ofstream archive(outputFile, std::ios::binary);  // Create binary output stream for the archive
    io::checkOpen(archive, outputFile, "Archive creation");  // Verify archive file was opened successfully
    auto metadata = compressFiles(rootDir, archive, compType);  // Compress files and get metadata
    displayStats(metadata);  // Output compression statistics
}

void FileCompressor::displayStats(const std::vector<meta::FileMeta>& metadata) {
    uint32_t uniqueCount = 0;  // Counter for unique files
    uint32_t duplicateCount = 0;  // Counter for duplicate files
    
    for (const auto& meta : metadata) {
        if (meta.isDuplicate()) {
            duplicateCount++;  // Files that are duplicates
        } else {
            uniqueCount++;  // Files with unique contents
        }
    }
    
    std::cout << "Total files in archive: " << metadata.size() << std::endl;
    std::cout << "Unique files: " << uniqueCount << ", Duplicate files: " << duplicateCount << std::endl;
}

std::vector<meta::FileMeta>
FileCompressor::decompressFiles(const std::string& outputDir, std::ifstream& archive) {
    CompressionType compType;
    auto metadata = io::readMetadata(archive, compType);  // Read file metadata and compression type from archive
    auto decompressor = createCompressor(compType);  // Create appropriate decompressor based on compression type

    std::filesystem::create_directories(outputDir);  // Create output directory if it doesn't exist
    threading::ThreadPool& threadPool = threading::ThreadPool::getInstance();  // Get thread pool for parallel processing
    std::mutex archiveMutex, outputMutex;  // Mutexes for thread-safe archive reading and output operations
    
    // Maps data offset to the corresponding file path in the output directory for linking duplicates
    std::unordered_map<int64_t, std::string> extractedPaths;
    
    // Separate containers to process unique and duplicate files
    std::vector<const meta::FileMeta*> uniqueFiles;
    std::vector<const meta::FileMeta*> duplicateFiles;
    
    // Classify files as either unique or duplicates
    for (const auto& meta : metadata) {
        if (meta.isDuplicate()) {
            duplicateFiles.push_back(&meta);  // Add duplicate files to their container
        } else {
            uniqueFiles.push_back(&meta);  // Add unique files to their containerer
        }
    }
    
    // Process unique files in parallel using the thread pool
    threadPool.parallelFor(uniqueFiles.begin(), uniqueFiles.end(), [&](auto it, size_t) {
        const auto& meta = **it;  // Dereference to get the actual metadata
        
        std::filesystem::path outputPath = std::filesystem::path(outputDir) / meta.relativePath;  // Build output file path
        std::filesystem::create_directories(outputPath.parent_path());  // Create parent directories if needed
        
        {
            std::lock_guard<std::mutex> lock(archiveMutex);  // Thread-safe archive access
            archive.clear();  // Clear any error flags on the stream
            archive.seekg(meta.dataOffset);  // Move to file data position in archive
            
            std::ofstream outputFile(outputPath, std::ios::binary);  // Create output file
            io::checkOpen(outputFile, outputPath.string(), "Output file creation");  // Verify file opened successfully
            decompressor->decompressStream(archive, outputFile);  // Decompress file data from archive to output
        }
        
        {
            std::lock_guard<std::mutex> lock(outputMutex);  // Thread-safe output operations
            extractedPaths[meta.dataOffset] = outputPath.string();  // Record extracted file path for duplicates
            std::cout << "Extracted: " << meta.relativePath << std::endl;  // Log extraction
        }
    });
    
    // Process duplicate files in parallel after originals are extracted
    threadPool.parallelFor(duplicateFiles.begin(), duplicateFiles.end(), [&](auto it, size_t) {
        const auto& meta = **it;  // Dereference to get the actual metadata
        
        std::filesystem::path outputPath = std::filesystem::path(outputDir) / meta.relativePath;  // Build output file path
        std::filesystem::create_directories(outputPath.parent_path());  // Create parent directories if needed
        
        std::string sourcePath;
        {
            std::lock_guard<std::mutex> lock(outputMutex);  // Thread-safe output operations
            if (extractedPaths.find(meta.dataOffset) == extractedPaths.end()) {  // Check if original file was extracted
                std::cout << "Error: No original file found for " << meta.relativePath << std::endl;  // Log error
                return;
            }
            sourcePath = extractedPaths[meta.dataOffset];  // Get path of original file
        }
        
        try {
            std::filesystem::copy_file(sourcePath, outputPath,  // Copy from original to duplicate location
                                      std::filesystem::copy_options::overwrite_existing);  // Overwrite if file exists
            
            std::lock_guard<std::mutex> lock(outputMutex);  // Thread-safe console output
            std::cout << "Extracted duplicate: " << meta.relativePath << std::endl;  // Log successful duplication
        }
        catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(outputMutex);  // Thread-safe console output
            std::cout << "Error copying to " << meta.relativePath << ": " << e.what() << std::endl;  // Log copy error
        }
    });

    return std::move(metadata);  // Return metadata for statistics
}

void FileCompressor::decompress(const std::string& archiveFile, const std::string& outputDir) {
    std::ifstream archive(archiveFile, std::ios::binary);  // Open archive file in binary mode for reading
    io::checkOpen(archive, archiveFile, "Archive reading");  // Verify the archive file opened successfully
    auto metadata = decompressFiles(outputDir, archive);  // Extract files to output directory and get metadata
    displayStats(metadata);  // Show statistics about decompressed files
}

}
