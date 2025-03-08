#include <gtest/gtest.h>
#include <vector>
#include <fstream>
#include <string>
#include <filesystem>

#include <openssl/evp.h>
#include <openssl/rand.h>

#include "HashUtils.h"

class HashUtilsTest : public ::testing::Test {
protected:
    // Setup temporary files for testing
    void SetUp() override {
        // Create a temporary directory for test files
        tempDir = std::filesystem::temp_directory_path() / "hashutils_test";
        std::filesystem::create_directories(tempDir);
        
        // Create an empty file
        emptyFilePath = tempDir / "empty_file.txt";
        std::ofstream emptyFile(emptyFilePath);
        emptyFile.close();
        
        // Create a file with known content
        knownContentPath = tempDir / "known_content.txt";
        std::ofstream knownFile(knownContentPath);
        std::string knownData = "Hello, World!"; // Simple test data
        knownFile.write(knownData.c_str(), knownData.size());
        knownFile.close();
        
        // Create a larger test file
        largeFilePath = tempDir / "large_file.bin";
        std::ofstream largeFile(largeFilePath, std::ios::binary);
        // Generate 100 KB of random data
        for (int i = 0; i < 100 * 1024; i++) {
            largeFile.put(static_cast<char>(i % 256));
        }
        largeFile.close();
        
        // Known hash values for test data
        knownDataHash = "dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f";
        emptyDataHash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    }
    
    void TearDown() override {
        // Clean up temporary files and directory
        std::filesystem::remove_all(tempDir);
    }
    
    std::filesystem::path tempDir;
    std::filesystem::path emptyFilePath;
    std::filesystem::path knownContentPath;
    std::filesystem::path largeFilePath;
    std::string knownDataHash;
    std::string emptyDataHash;
};

// Test hashing an empty buffer
TEST_F(HashUtilsTest, EmptyBufferHash) {
    std::vector<uint8_t> emptyBuffer;
    std::string hash = hashutils::computeSHA256FromDataBuffer(emptyBuffer.data(), emptyBuffer.size());
    EXPECT_EQ(hash, emptyDataHash);
}

// Test hashing a buffer with known content
TEST_F(HashUtilsTest, KnownContentBufferHash) {
    std::string data = "Hello, World!";
    std::vector<uint8_t> buffer(data.begin(), data.end());
    std::string hash = hashutils::computeSHA256FromDataBuffer(buffer.data(), buffer.size());
    EXPECT_EQ(hash, knownDataHash);
}

// Test hashing an empty file
TEST_F(HashUtilsTest, EmptyFileHash) {
    std::string hash = hashutils::computeSHA256FromFile(emptyFilePath.string());
    EXPECT_EQ(hash, emptyDataHash);
}

// Test hashing a file with known content
TEST_F(HashUtilsTest, KnownContentFileHash) {
    std::string hash = hashutils::computeSHA256FromFile(knownContentPath.string());
    EXPECT_EQ(hash, knownDataHash);
}

// Test that file and buffer hashing produce the same result for the same content
TEST_F(HashUtilsTest, FileAndBufferHashConsistency) {
    // Read file content into buffer
    std::ifstream file(knownContentPath, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
    file.close();
    
    // Compute hashes
    std::string fileHash = hashutils::computeSHA256FromFile(knownContentPath.string());
    std::string bufferHash = hashutils::computeSHA256FromDataBuffer(buffer.data(), buffer.size());
    
    // Verify they match
    EXPECT_EQ(fileHash, bufferHash);
}

// Test hashing a large file (to test chunking functionality)
TEST_F(HashUtilsTest, LargeFileHash) {
    std::string largeFileHash = hashutils::computeSHA256FromFile(largeFilePath.string());
    
    // Read the large file into buffer and compute hash for verification
    std::ifstream file(largeFilePath, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
    file.close();
    std::string bufferHash = hashutils::computeSHA256FromDataBuffer(buffer.data(), buffer.size());
    
    // Both methods should produce the same hash
    EXPECT_EQ(largeFileHash, bufferHash);
}

// Test error handling when trying to hash a non-existent file
TEST_F(HashUtilsTest, NonExistentFileError) {
    std::filesystem::path nonExistentFile = tempDir / "does_not_exist.txt";
    EXPECT_THROW(hashutils::computeSHA256FromFile(nonExistentFile.string()), std::runtime_error);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}