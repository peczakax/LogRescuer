#ifndef FILEMETA_H
#define FILEMETA_H

#include <string>
#include <cstdint>

namespace meta {

    // Structure to store file metadata information
struct FileMeta {
    const int64_t dataOffset;      // Position in the archive where file data begins
    const std::string hash;         // Hash value for data integrity verification
    const std::string relativePath; // Path to the file relative to a base directory
    
    // Returns true if this file is original (has data stored in archive)
    bool isOriginal() const { 
        return dataOffset >= 0;
    }

    // Returns true if this file is duplicate (has no data stored in archive, just references another file)
    bool isDuplicate() const { 
        return dataOffset < 0;  // Must have a hash but no data offset
    }

    FileMeta() = delete;  // Deleted constructor
    explicit FileMeta(const uint64_t dataOffset, const std::string& hash, const std::string& path) : 
        dataOffset(dataOffset), hash(hash), relativePath(path) {}  // Constructor initializing all fields
};

}  // End of meta namespace

#endif // FILEMETA_H
