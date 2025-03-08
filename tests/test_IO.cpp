#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include "IO.h"
#include "FileMeta.h"

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

TEST_F(IOTest, MetadataReadWrite) {
    // Test reading and writing file metadata
    std::string testFilePath = (tempDir / "test_metadata.dat").string();
    
    // Create some test metadata
    std::vector<meta::FileMeta> testMetadata;
    for (int i = 0; i < 3; i++) {
        meta::FileMeta meta;
        meta.compressedSize = 1000 + i;
        meta.originalSize = 2000 + i;
        meta.dataOffset = 3000 + i;
        meta.hash = "hash" + std::to_string(i);
        meta.relativePath = "path/to/file" + std::to_string(i) + ".txt";
        testMetadata.push_back(meta);
    }
    
    // Write metadata
    {
        std::ofstream outFile(testFilePath, std::ios::binary);
        io::checkOpen(outFile, testFilePath, "Write metadata test");
        io::writeMetadata(outFile, testMetadata);
    }
    
    // Read metadata
    {
        std::ifstream inFile(testFilePath, std::ios::binary);
        io::checkOpen(inFile, testFilePath, "Read metadata test");
        auto readMetadata = io::readMetadata(inFile);
        
        EXPECT_EQ(readMetadata.size(), testMetadata.size());
        
        for (size_t i = 0; i < testMetadata.size(); i++) {
            EXPECT_EQ(readMetadata[i].compressedSize, testMetadata[i].compressedSize);
            EXPECT_EQ(readMetadata[i].originalSize, testMetadata[i].originalSize);
            EXPECT_EQ(readMetadata[i].dataOffset, testMetadata[i].dataOffset);
            EXPECT_EQ(readMetadata[i].hash, testMetadata[i].hash);
            EXPECT_EQ(readMetadata[i].relativePath, testMetadata[i].relativePath);
        }
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