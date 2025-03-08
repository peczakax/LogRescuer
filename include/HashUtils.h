#ifndef HASHUTILS_H
#define HASHUTILS_H

#include <string>
#include <vector>

namespace hashutils {

// Computes a SHA-256 hash for a file at the specified path
std::string computeSHA256FromFile(const std::string& filePath);

// Computes a SHA-256 hash for a buffer of bytes
std::string computeSHA256FromDataBuffer(const uint8_t* data, size_t size);

} // namespace hashutils

#endif // HASHUTILS_H
