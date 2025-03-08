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
    
    // Explicitly disable skipping empty files since our test files are empty
    auto files = io::scanDirectory(tempDir.string(), false);
    EXPECT_EQ(files.size(), 3);
}

TEST_F(IOTest, ScanDirectoryWithNestedFolders) {
    // Create a nested directory structure
    auto nestedDir = tempDir / "nested";
    std::filesystem::create_directory(nestedDir);
    auto nestedSubDir = nestedDir / "subdir";
    std::filesystem::create_directory(nestedSubDir);
    
    // Create files in different levels
    std::ofstream file1(tempDir / "root.txt");
    file1 << "root content";
    file1.close();
    
    std::ofstream file2(nestedDir / "level1.txt");
    file2 << "level1 content";
    file2.close();
    
    std::ofstream file3(nestedSubDir / "level2.txt");
    file3 << "level2 content";
    file3.close();
    
    // Test recursive scanning
    auto files = io::scanDirectory(tempDir.string(), false);
    EXPECT_EQ(files.size(), 3);
}

TEST_F(IOTest, ScanDirectorySkipEmpty) {
    // Create both empty and non-empty files
    std::ofstream emptyFile(tempDir / "empty.txt");
    emptyFile.close();
    
    std::ofstream nonEmptyFile(tempDir / "nonempty.txt");
    nonEmptyFile << "This file has content";
    nonEmptyFile.close();
    
    // Test with skipEmptyFiles=true (default)
    auto filesWithSkip = io::scanDirectory(tempDir.string(), true);
    EXPECT_EQ(filesWithSkip.size(), 1);
    
    // Test with skipEmptyFiles=false
    auto filesNoSkip = io::scanDirectory(tempDir.string(), false);
    EXPECT_EQ(filesNoSkip.size(), 2);
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

TEST_F(IOTest, MetadataWithDuplicates) {
    std::string testFilePath = (tempDir / "test_metadata_dupes.dat").string();
    
    // Create metadata with both original and duplicate files
    std::vector<meta::FileMeta> testMetadata;
    
    // Add original files
    testMetadata.emplace_back(3000, "hash1", "path/to/file1.txt");
    testMetadata.emplace_back(4000, "hash2", "path/to/file2.txt");
    
    // Add duplicate files (with dataOffset = -1 and empty hash)
    testMetadata.emplace_back(-1, "", "path/to/duplicate1.txt");
    testMetadata.emplace_back(-1, "", "path/to/duplicate2.txt");
    
    // Write metadata
    {
        std::ofstream outFile(testFilePath, std::ios::binary);
        io::checkOpen(outFile, testFilePath, "Write metadata with duplicates test");
        io::writeMetadata(outFile, testMetadata, compression::CompressionType::ZSTD);
    }
    
    // Read metadata
    {
        std::ifstream inFile(testFilePath, std::ios::binary);
        io::checkOpen(inFile, testFilePath, "Read metadata with duplicates test");
        compression::CompressionType compType;
        auto readMetadata = io::readMetadata(inFile, compType);
        
        EXPECT_EQ(readMetadata.size(), testMetadata.size());
        
        // Check originals
        EXPECT_EQ(readMetadata[0].dataOffset, 3000);
        EXPECT_EQ(readMetadata[0].hash, "hash1");
        EXPECT_EQ(readMetadata[0].relativePath, "path/to/file1.txt");
        
        EXPECT_EQ(readMetadata[1].dataOffset, 4000);
        EXPECT_EQ(readMetadata[1].hash, "hash2");
        EXPECT_EQ(readMetadata[1].relativePath, "path/to/file2.txt");
        
        // Check duplicates
        EXPECT_EQ(readMetadata[2].dataOffset, -1);
        EXPECT_TRUE(readMetadata[2].hash.empty());
        EXPECT_EQ(readMetadata[2].relativePath, "path/to/duplicate1.txt");
        
        EXPECT_EQ(readMetadata[3].dataOffset, -1);
        EXPECT_TRUE(readMetadata[3].hash.empty());
        EXPECT_EQ(readMetadata[3].relativePath, "path/to/duplicate2.txt");
    }
}

TEST_F(IOTest, FooterReadWrite) {
    std::string testFilePath = (tempDir / "test_footer.dat").string();
    
    // Test data
    compression::CompressionType writeCompType = compression::CompressionType::ZSTD;
    uint64_t writeUniqueCount = 50;
    uint64_t writeDuplicateCount = 30;
    uint64_t writeMetaOffset = 12345;
    
    // Write test
    {
        std::ofstream outFile(testFilePath, std::ios::binary);
        io::checkOpen(outFile, testFilePath, "Write footer test");
        
        // Write some dummy data first
        std::string dummyData = "Dummy data to simulate file content";
        outFile.write(dummyData.c_str(), dummyData.size());
        
        io::writeFooter(outFile, writeCompType, writeUniqueCount, writeDuplicateCount, writeMetaOffset);
    }
    
    // Read test
    {
        std::ifstream inFile(testFilePath, std::ios::binary);
        io::checkOpen(inFile, testFilePath, "Read footer test");
        
        compression::CompressionType readCompType;
        uint64_t readUniqueCount;
        uint64_t readDuplicateCount;
        uint64_t readMetaOffset;
        
        io::readFooter(inFile, readCompType, readUniqueCount, readDuplicateCount, readMetaOffset);
        
        EXPECT_EQ(readCompType, writeCompType);
        EXPECT_EQ(readUniqueCount, writeUniqueCount);
        EXPECT_EQ(readDuplicateCount, writeDuplicateCount);
        EXPECT_EQ(readMetaOffset, writeMetaOffset);
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

TEST_F(IOTest, ReadSizeMismatch) {
    std::string testFilePath = (tempDir / "test_size_mismatch.dat").string();
    
    // Create a file with some content
    {
        std::ofstream outFile(testFilePath, std::ios::binary);
        io::checkOpen(outFile, testFilePath, "Write test");
        
        // Write less data than we'll try to read
        uint32_t value = 12345;
        outFile.write(reinterpret_cast<const char*>(&value), sizeof(uint32_t));
    }
    
    // Try to read more than what's in the file
    {
        std::ifstream inFile(testFilePath, std::ios::binary);
        io::checkOpen(inFile, testFilePath, "Read test");
        
        // Try to read more bytes than available
        char buffer[sizeof(uint32_t) * 2];
        
        EXPECT_THROW({
            io::readBuffer(inFile, buffer, sizeof(buffer));
        }, std::runtime_error);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}