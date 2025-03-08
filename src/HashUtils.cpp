#include <fstream>
#include <stdexcept>
#include <vector>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <array>

#include <openssl/evp.h>

#include "HashUtils.h"

namespace hashutils {

auto evpContextDeleter = [](EVP_MD_CTX* ctx) { if (ctx) EVP_MD_CTX_free(ctx); };

// Helper function to convert hash data to a hexadecimal string representation
std::string bytesToHexString(const uint8_t* data, size_t length) {
    std::stringstream ss;
    for (auto i = 0; i < length; i++) {
        // static_cast to int to get the numerical value for proper hex formatting
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return ss.str();
}

// Calculates SHA-256 hash of file contents specified by filePath
std::string computeSHA256FromFile(const std::string& filePath) {
    // Verify file existence before attempting operations
    if (!std::filesystem::exists(filePath)) {
        throw std::runtime_error("File not found: " + filePath);
    }
    
    // Open file in binary mode to handle all file types correctly
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to open file for hashing: " + filePath);
    }

    // Create digest context with RAII for automatic cleanup
    std::unique_ptr<EVP_MD_CTX, decltype(evpContextDeleter)> ctx(EVP_MD_CTX_new(), evpContextDeleter);
    if (!ctx) {
        throw std::runtime_error("Failed to create hash context");
    }

    // Initialize the digest context with SHA-256 algorithm
    if (EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) {
        throw std::runtime_error("Failed to initialize hash context with SHA-256 algorithm");
    }

    // Use 8KB buffer for reading file in chunks to handle large files efficiently
    const size_t bufferSize = 8192;
    std::vector<unsigned char> buffer(bufferSize);
    
    // Process the file in chunks until EOF is reached
    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), bufferSize);  // Read a chunk of data
        size_t bytesRead = file.gcount();      // Get actual number of bytes read
        if (bytesRead > 0) {
            // Update the hash computation with current chunk of data
            if (EVP_DigestUpdate(ctx.get(), buffer.data(), bytesRead) != 1) {
                throw std::runtime_error("Failed to update hash");
            }
        }
    }

    // Prepare buffer to store the resulting hash value
    std::array<unsigned char, EVP_MAX_MD_SIZE> hash;  // Use std::array for the hash buffer
    unsigned int hashLen;                 // Will store the actual length of the hash
    
    // Finalize the hash computation and get the digest value
    if (EVP_DigestFinal_ex(ctx.get(), hash.data(), &hashLen) != 1) {
        throw std::runtime_error("Failed to finalize hash");
    }

    // Convert binary hash to hexadecimal string representation
    return bytesToHexString(hash.data(), hashLen);  // Use the helper function
}

// Calculates SHA-256 hash of data buffer
std::string computeSHA256FromDataBuffer(const uint8_t* data, size_t size) {

    std::unique_ptr<EVP_MD_CTX, decltype(evpContextDeleter)> ctx(EVP_MD_CTX_new(), evpContextDeleter);
    if (!ctx) {
        throw std::runtime_error("Failed to create hash context");
    }

    if (EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) {  // Initialize with SHA-256 algorithm
        throw std::runtime_error("Failed to initialize hash context with SHA-256 algorithm");
    }

    if (EVP_DigestUpdate(ctx.get(), data, size) != 1) {  // Process all data at once
        throw std::runtime_error("Failed to update hash");
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> hash;  // Use std::array for the hash buffer
    unsigned int hashLen;  // Variable to receive the hash length
    
    if (EVP_DigestFinal_ex(ctx.get(), hash.data(), &hashLen) != 1) {  // Finalize the hash calculation
        throw std::runtime_error("Failed to finalize hash");
    }

    return bytesToHexString(hash.data(), hashLen);  // Use the helper function
}

}  // End of hashutils namespace