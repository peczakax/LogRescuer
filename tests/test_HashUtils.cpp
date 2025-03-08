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
        
        // Create a file with known content
        knownContentPath = tempDir / "known_content.txt";
        std::ofstream knownFile(knownContentPath);
        knownFile.write(knownData.c_str(), knownData.size());
    }
    
    void TearDown() override {
        // Clean up temporary files and directory
        std::filesystem::remove_all(tempDir);
    }
    
    std::filesystem::path tempDir;
    std::filesystem::path emptyFilePath;
    std::filesystem::path knownContentPath;

    const std::string knownData = "Hello, World!";
    const std::string knownDataHash = "dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f";
    const std::string emptyDataHash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
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
    std::vector<uint8_t> buffer(knownData.begin(), knownData.end());
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
    
    // Compute hashes
    std::string fileHash = hashutils::computeSHA256FromFile(knownContentPath.string());
    std::string bufferHash = hashutils::computeSHA256FromDataBuffer(buffer.data(), buffer.size());
    
    // Verify they match
    EXPECT_EQ(fileHash, bufferHash);
}

// Test hashing a large file (to test chunking functionality)
TEST_F(HashUtilsTest, LargeFileHash) {
    // Create a test file of 100KB with random data
    std::vector<uint8_t> largeBuffer(100 * 1024);
    RAND_bytes(largeBuffer.data(), largeBuffer.size());
    
    // Write the data to file
    std::filesystem::path largeFilePath = tempDir / "large_file.bin";
    std::ofstream largeFile(largeFilePath, std::ios::binary);
    largeFile.write(reinterpret_cast<const char*>(largeBuffer.data()), largeBuffer.size());
    
    // Compare hash from file with hash from buffer
    std::string fileHash = hashutils::computeSHA256FromFile(largeFilePath.string());
    std::string bufferHash = hashutils::computeSHA256FromDataBuffer(largeBuffer.data(), largeBuffer.size());
    
    EXPECT_EQ(fileHash, bufferHash);
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