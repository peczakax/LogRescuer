#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>

#include "CompressorFactory.h"
#include "IO.h"
#include "FileMeta.h"

namespace io {

void checkErrors(const std::ios& stream, const std::string& operation) {
    if (stream.fail()) {
        throw std::runtime_error(operation + " failed: formatting or extraction error");
    }
    if (stream.bad()) {
        throw std::runtime_error(operation + " failed: unrecoverable stream error");
    }
}

void checkReadSize(size_t expected, size_t actual) {
    if (actual != expected) {
        throw std::runtime_error("Read operation failed: incomplete read (" + 
                            std::to_string(actual) + " of " + 
                            std::to_string(expected) + " bytes)");
    }
}

void writeFooter(std::ofstream& stream, compression::CompressionType compType, uint64_t uniqueCount, uint64_t duplicateCount, uint64_t metaOffset) {
    write(stream, compType);
    write(stream, uniqueCount);
    write(stream, duplicateCount);
    write(stream, metaOffset);
}

void readFooter(std::ifstream& stream, compression::CompressionType& compType, uint64_t& uniqueCount, uint64_t& duplicateCount, uint64_t& metaOffset) {
    // Seek to the end of the file, minus the size of the extended footer
    // Footer contains: compression type (4 bytes) + 3 uint64_t values (8 bytes each)
    size_t footerSize = sizeof(compression::CompressionType) + 3 * sizeof(uint64_t);
    stream.seekg(-static_cast<std::streamoff>(footerSize), std::ios::end);
    
    read(stream, compType);
    read(stream, uniqueCount);
    read(stream, duplicateCount);    
    read(stream, metaOffset);
}

void write(std::ofstream& stream, const std::string& str) {
    uint64_t length = str.length();
    write(stream, length);
    writeBuffer(stream, str.c_str(), length);
}

void read(std::ifstream& stream, std::string& str) {
    uint64_t length;
    read(stream, length);
    str.resize(length);
    readBuffer(stream, &str[0], length);
}

void write(std::ofstream& stream, const meta::FileMeta& meta) {
    io::write(stream, meta.dataOffset);
    io::write(stream, meta.hash);
    io::write(stream, meta.relativePath);
}

void writeMetadata(std::ofstream& stream, const std::vector<meta::FileMeta>& metadata, compression::CompressionType compType) {
    uint64_t metaOffset = stream.tellp();  // Get position for metadata section

    // Split metadata into unique and duplicate files
    std::vector<const meta::FileMeta*> uniqueFiles;
    std::vector<const meta::FileMeta*> duplicateFiles;
    
    for (const auto& meta : metadata) {
        if (meta.isDuplicate()) {
            duplicateFiles.push_back(&meta);
        } else {
            uniqueFiles.push_back(&meta);
        } 
    }

    // Write unique files (full metadata)
    for (const auto* meta : uniqueFiles) {
        io::write(stream, *meta);
    }
    
    // Write duplicate files (only dataOffset and relativePath for duplicates)
    for (const auto* meta : duplicateFiles) {
        io::write(stream, meta->dataOffset);
        io::write(stream, meta->relativePath);
    }
    
    // Write footer with metadata position, count, and compression type
    io::writeFooter(stream, compType, uniqueFiles.size(), duplicateFiles.size(), metaOffset);
}

std::vector<meta::FileMeta> readMetadata(std::ifstream& stream, compression::CompressionType& compType) {
    uint64_t uniqueCount;
    size_t duplicateCount;
    size_t metaOffset;
    
    io::readFooter(stream, compType, uniqueCount, duplicateCount, metaOffset);
    stream.seekg(metaOffset);

    std::vector<meta::FileMeta> metadata;
    metadata.reserve(uniqueCount + duplicateCount);

    // Read unique files
    for (size_t i = 0; i < uniqueCount; i++) {
        int64_t offset;
        std::string hash;
        std::string path;
        
        io::read(stream, offset);
        io::read(stream, hash);
        io::read(stream, path);
        
        metadata.emplace_back(offset, hash, path);
    }
        
    // Read duplicate files
    for (size_t i = 0; i < duplicateCount; i++) {
        std::int64_t dataOffset;
        std::string path;
        
        io::read(stream, dataOffset);
        io::read(stream, path);
        
        metadata.emplace_back(dataOffset, "", path);
    }
    
    return std::move(metadata);
}

std::vector<std::filesystem::path> scanDirectory(const std::string& rootDir, bool skipEmptyFiles) {
    std::vector<std::filesystem::path> filePaths;  // Container for all found file paths
    
    // Recursively iterate through all files in the directory
    for (const auto& entry : std::filesystem::recursive_directory_iterator(rootDir)) {
        if (entry.is_regular_file()) {
            // Check if file is non-empty
            if (std::filesystem::file_size(entry.path()) == 0) {
                if (skipEmptyFiles) {
                    std::cout << "Skipping empty file: " << entry.path() << std::endl;
                    continue;
                }
            }
            filePaths.push_back(entry.path());  // Add each non-empty regular file to the list
        }
    }
    
    return std::move(filePaths);
}

}
