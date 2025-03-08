#ifndef FILEMETA_H
#define FILEMETA_H

#include <string>
#include <cstdint>

namespace meta {

    // Structure to store file metadata information
struct FileMeta {
    uint32_t compressedSize;  // Size of the file after compression in bytes
    uint32_t originalSize;    // Original file size in bytes before compression
    uint64_t dataOffset;      // Position in the archive where file data begins
    std::string hash;         // Hash value for data integrity verification
    std::string relativePath; // Path to the file relative to a base directory

    FileMeta() = default;  // Default constructor with no initialization
    explicit FileMeta(const std::string& path) : relativePath(path) {}  // Constructor initializing only relativePath
};

}  // End of meta namespace

#endif // FILEMETA_H 
