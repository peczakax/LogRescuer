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

void writeFooter(std::ofstream& stream, compression::CompressionType compType, uint64_t metaOffset, uint64_t metaCount) {
    write(stream, compType);
    write(stream, metaOffset);
    write(stream, metaCount);
}

void readFooter(std::ifstream& stream, compression::CompressionType& compType, uint64_t& metaOffset, uint64_t& metaCount) {
    // Seek to the end of the file, minus the size of the extended footer.
    size_t footerSize = sizeof(uint64_t) * 2 + sizeof(compression::CompressionType);
    stream.seekg(-footerSize, std::ios::end);
    read(stream, compType);
    read(stream, metaOffset);
    read(stream, metaCount);
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

void write(std::ofstream& archive, const meta::FileMeta& meta) {
    io::write(archive, meta.dataOffset);
    io::write(archive, meta.hash);
    io::write(archive, meta.relativePath);
}

meta::FileMeta read(std::ifstream& archive) {
    int64_t offset;
    std::string hash;
    std::string path;
    
    io::read(archive, offset);
    io::read(archive, hash);
    io::read(archive, path);
    
    return meta::FileMeta(offset, hash, path);
}

void writeMetadata(std::ofstream& archive, const std::vector<meta::FileMeta>& metadata, compression::CompressionType compType) {
    uint64_t metaOffset = archive.tellp();  // Get position for metadata section
    
    // Split metadata into unique and duplicate files
    std::vector<const meta::FileMeta*> originalFiles;
    std::vector<const meta::FileMeta*> duplicateFiles;
    
    for (const auto& meta : metadata) {
        if (meta.isOriginal()) {
            originalFiles.push_back(&meta);  // Original files have valid dataOffset
        } else if (meta.isDuplicate()) {
            duplicateFiles.push_back(&meta);  // Duplicate files have dataOffset = -1
        }
    }
    
    // Write number of original files
    uint64_t originalCount = originalFiles.size();
    io::write(archive, originalCount);
    
    // Write original files (full metadata)
    for (const auto* meta : originalFiles) {
        io::write(archive, *meta);
    }
    
    // Write number of duplicate files
    uint64_t duplicateCount = duplicateFiles.size();
    io::write(archive, duplicateCount);
    
    // Write duplicate files (only hash and relativePath for duplicates)
    for (const auto* meta : duplicateFiles) {
        io::write(archive, meta->hash);
        io::write(archive, meta->relativePath);
    }
    
    // Write footer with metadata position, count, and compression type
    io::writeFooter(archive, compType, metaOffset, metadata.size());
}

std::vector<meta::FileMeta> readMetadata(std::ifstream& archive, compression::CompressionType& compType) {
    uint64_t metaOffset, metaCount;
    
    io::readFooter(archive, compType, metaOffset, metaCount);
    archive.seekg(metaOffset);
    
    std::vector<meta::FileMeta> metadata;
    metadata.reserve(metaCount);
    
    uint64_t originalCount;
    io::read(archive, originalCount);
    
    // Read original files
    for (uint64_t i = 0; i < originalCount; i++) {
        int64_t offset;
        std::string hash;
        std::string path;
        
        io::read(archive, offset);
        io::read(archive, hash);
        io::read(archive, path);
        
        metadata.emplace_back(offset, hash, path);
    }
    
    // Read duplicate files count
    uint64_t duplicateCount;
    io::read(archive, duplicateCount);
    
    // Read duplicate files
    for (uint64_t i = 0; i < duplicateCount; i++) {
        std::string hash;
        std::string relativePath;
        
        io::read(archive, hash);
        io::read(archive, relativePath);
        
        metadata.emplace_back(-1, hash, relativePath);
    }
    
    return metadata;
}

std::vector<std::filesystem::path> scanDirectory(const std::string& rootDir) {
    std::vector<std::filesystem::path> filePaths;  // Container for all found file paths
    
    // Recursively iterate through all files in the directory
    for (const auto& entry : std::filesystem::recursive_directory_iterator(rootDir)) {
        if (entry.is_regular_file()) {
            filePaths.push_back(entry.path());  // Add each regular file to the list
        }
    }
    
    return filePaths;
}

}
