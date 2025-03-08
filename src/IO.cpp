#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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

void writeFooter(std::ofstream& stream, uint64_t metaOffset, uint64_t metaCount) {
    write(stream, metaOffset);
    write(stream, metaCount);
}


void readFooter(std::ifstream& stream, uint64_t& metaOffset, uint64_t& metaCount) {
    // Seek to the end of the file, minus the size of the footer.
    size_t footerSize = sizeof(uint64_t) * 2;
    stream.seekg(-footerSize, std::ios::end);
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
    io::write(archive, meta.compressedSize);
    io::write(archive, meta.originalSize);
    io::write(archive, meta.dataOffset);
    io::write(archive, meta.hash);
    io::write(archive, meta.relativePath);
}

meta::FileMeta read(std::ifstream& archive) {
    meta::FileMeta meta;        
    io::read(archive, meta.compressedSize);
    io::read(archive, meta.originalSize);
    io::read(archive, meta.dataOffset);
    io::read(archive, meta.hash);
    io::read(archive, meta.relativePath);
    return std::move(meta);
}

void writeMetadata(std::ofstream& archive, const std::vector<meta::FileMeta>& metadata) {
    uint64_t metaOffset = archive.tellp();  // Get position for metadata section
    for (const auto& meta : metadata) {
        io::write(archive, meta);  // Write each metadata entry
    }
    io::writeFooter(archive, metaOffset, metadata.size());  // Write footer with metadata position and count
}

std::vector<meta::FileMeta> readMetadata(std::ifstream& archive) {
    uint64_t metaOffset, metaCount;  // Position and count of metadata entries
    io::readFooter(archive, metaOffset, metaCount);  // Read archive footer

    archive.seekg(metaOffset);  // Seek to metadata section
    std::vector<meta::FileMeta> metadata;
    metadata.reserve(metaCount);  // Pre-allocate space
    
    for (uint64_t i = 0; i < metaCount; i++) {
        metadata.emplace_back(io::read(archive));  // Read each metadata entry
    }
    
    return metadata;
}

std::vector<std::filesystem::path> scanDirectory(const std::string& rootDir) {
    std::vector<std::filesystem::path> filePaths;  // Container for all found file paths
    std::cout << "Scanning directory: " << rootDir << std::endl;
    
    // Recursively iterate through all files in the directory
    for (const auto& entry : std::filesystem::recursive_directory_iterator(rootDir)) {
        if (entry.is_regular_file()) {
            filePaths.push_back(entry.path());  // Add each regular file to the list
        }
    }
    
    std::cout << "Found " << filePaths.size() << " files" << std::endl;
    return filePaths;
}

}
