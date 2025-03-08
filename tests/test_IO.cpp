#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "IO.h"
#include "FileMeta.h"
#include "CompressorFactory.h"

class IOTest : public ::testing::Test {
protected:
    std::filesystem::path tempDir;
    
    void SetUp() override {
        // Create a temporary directory for tests
        tempDir = std::filesystem::temp_directory_path() / "logrescuer_test";
        std::filesystem::create_directory(tempDir);
    }
    
    void TearDown() override {
        // Clean up temporary directory
        std::filesystem::remove_all(tempDir);
    }
};

TEST_F(IOTest, ScanDirectory) {
    // Create test files in the temporary directory
    std::ofstream file1(tempDir / "file1.txt");
    file1.close();
    std::ofstream file2(tempDir / "file2.txt");
    file2.close();
    std::ofstream file3(tempDir / "file3.txt");
    file3.close();
    
    auto files = io::scanDirectory(tempDir.string());
    EXPECT_EQ(files.size(), 3);
}

TEST_F(IOTest, ReadWrite) {
    // Test basic read/write operations
    std::string testFilePath = (tempDir / "test_read_write.dat").string();
    
    // Test data
    uint32_t testInt = 12345;
    std::string testStr = "Hello, world!";
    
    // Write test
    {
        std::ofstream outFile(testFilePath, std::ios::binary);
        io::checkOpen(outFile, testFilePath, "Write test");
        
        io::write(outFile, testInt);
        io::write(outFile, testStr);
    }
    
    // Read test
    {
        std::ifstream inFile(testFilePath, std::ios::binary);
        io::checkOpen(inFile, testFilePath, "Read test");
        
        uint32_t readInt;
        std::string readStr;
        
        io::read(inFile, readInt);
        io::read(inFile, readStr);
        
        EXPECT_EQ(readInt, testInt);
        EXPECT_EQ(readStr, testStr);
    }
}

TEST_F(IOTest, ReadWriteBuffer) {
    // Test buffer read/write operations
    std::string testFilePath = (tempDir / "test_buffer.dat").string();
    
    // Test data
    const size_t bufferSize = 1024;
    char writeBuffer[bufferSize];
    char readBuffer[bufferSize];
    
    // Initialize the write buffer with some pattern
    for (size_t i = 0; i < bufferSize; i++) {
        writeBuffer[i] = static_cast<char>(i % 256);
    }
    
    // Write test
    {
        std::ofstream outFile(testFilePath, std::ios::binary);
        io::checkOpen(outFile, testFilePath, "Write buffer test");
        io::writeBuffer(outFile, writeBuffer, bufferSize);
    }
    
    // Read test
    {
        std::ifstream inFile(testFilePath, std::ios::binary);
        io::checkOpen(inFile, testFilePath, "Read buffer test");
        io::readBuffer(inFile, readBuffer, bufferSize);
        
        // Verify buffer contents
        for (size_t i = 0; i < bufferSize; i++) {
            EXPECT_EQ(readBuffer[i], writeBuffer[i]);
        }
    }
}

TEST_F(IOTest, EmptyBufferHandling) {
    // Test handling of empty buffers
    std::string testFilePath = (tempDir / "test_empty_buffer.dat").string();
    
    // Write test with empty buffer
    {
        std::ofstream outFile(testFilePath, std::ios::binary);
        io::checkOpen(outFile, testFilePath, "Write empty buffer test");
        // Use a properly typed buffer but with size 0
        char * dummyBuffer = nullptr; // We need a typed buffer even if we don't use it
        io::writeBuffer(outFile, dummyBuffer, 0);
    }
    
    // Read test with empty buffer
    {
        std::ifstream inFile(testFilePath, std::ios::binary);
        io::checkOpen(inFile, testFilePath, "Read empty buffer test");
        // Use a properly typed buffer but with size 0
        char * dummyBuffer = nullptr; // We need a typed buffer even if we don't use it
        io::readBuffer(inFile, dummyBuffer, 0);
        EXPECT_TRUE(inFile.good());
    }
}

TEST_F(IOTest, MetadataReadWrite) {
    std::string testFilePath = (tempDir / "test_metadata.dat").string();
    
    // Create some test metadata
    std::vector<meta::FileMeta> testMetadata;
    for (int i = 0; i < 3; i++) {
        testMetadata.emplace_back(
            3000 + i,
            "hash" + std::to_string(i),
            "path/to/file" + std::to_string(i) + ".txt"
        );
    }
    
    // Write metadata with default compression type
    {
        std::ofstream outFile(testFilePath, std::ios::binary);
        io::checkOpen(outFile, testFilePath, "Write metadata test");
        io::writeMetadata(outFile, testMetadata, compression::CompressionType::ZSTD);
    }
    
    // Read metadata
    {
        std::ifstream inFile(testFilePath, std::ios::binary);
        io::checkOpen(inFile, testFilePath, "Read metadata test");
        compression::CompressionType compType;
        auto readMetadata = io::readMetadata(inFile, compType);
        
        EXPECT_EQ(readMetadata.size(), testMetadata.size());
        
        for (size_t i = 0; i < testMetadata.size(); i++) {
            EXPECT_EQ(readMetadata[i].dataOffset, testMetadata[i].dataOffset);
            EXPECT_EQ(readMetadata[i].hash, testMetadata[i].hash);
            EXPECT_EQ(readMetadata[i].relativePath, testMetadata[i].relativePath);
        }
        
        EXPECT_EQ(compType, compression::CompressionType::ZSTD);
    }
}

TEST_F(IOTest, ErrorChecking) {
    // Test error handling
    std::string nonExistentFile = (tempDir / "non_existent.dat").string();
    
    // Test failing to open a non-existent file
    std::ifstream inFile(nonExistentFile);
    EXPECT_THROW({
        io::checkOpen(inFile, nonExistentFile, "Open");
    }, std::runtime_error);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}