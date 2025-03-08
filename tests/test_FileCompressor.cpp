#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "CompressorFactory.h"
#include "FileCompressor.h"
#include "FileMeta.h"
#include "HashUtils.h"
#include "IO.h"
#include "ThreadPool.h"

namespace compression {

class FileCompressorTest : public ::testing::Test {
protected:
    void SetUp() override {
        tempDir = std::filesystem::temp_directory_path() / "filecompressor_test";
        std::filesystem::create_directories(tempDir);

        // Create test files
        createTestFile("file1.txt", "Hello, World!");
        createTestFile("file2.txt", "Hello, World!");
        createTestFile("file3.txt", "Different content");
        createTestFile("file4.txt", "");
    }

    void TearDown() override {
        std::filesystem::remove_all(tempDir);
    }

    void createTestFile(const std::string& filename, const std::string& content) {
        std::ofstream file(tempDir / filename);
        file << content;
        file.close();
    }

    std::filesystem::path tempDir;
};

class FileCompressorParameterizedTest : public FileCompressorTest, 
                                        public ::testing::WithParamInterface<CompressionType> {
protected:
    CompressionType GetCompressionType() const {
        return GetParam();
    }
};

TEST_F(FileCompressorTest, ComputeHashes) {
    auto files = io::scanDirectory(tempDir.string());
    auto [hashToPathMap, pathToHashMap] = FileCompressor::computeHashes(files, tempDir);

    EXPECT_EQ(hashToPathMap.size(), 3);  // Three unique contents
    EXPECT_EQ(pathToHashMap.size(), 4);  // Four files
}

TEST_F(FileCompressorTest, GroupFiles) {
    auto files = io::scanDirectory(tempDir.string());
    auto [hashToPathMap, pathToHashMap] = FileCompressor::computeHashes(files, tempDir);
    auto fileGroup = FileCompressor::groupFiles(files, tempDir, pathToHashMap, hashToPathMap);

    EXPECT_EQ(fileGroup.uniqueFiles.size(), 3);  // Three unique files
    EXPECT_EQ(fileGroup.duplicateFiles.size(), 1);  // One duplicate file
}

TEST_P(FileCompressorParameterizedTest, ProcessFiles) {
    auto files = io::scanDirectory(tempDir.string());
    auto [hashToPathMap, pathToHashMap] = FileCompressor::computeHashes(files, tempDir);
    auto fileGroup = FileCompressor::groupFiles(files, tempDir, pathToHashMap, hashToPathMap);

    std::ofstream archive(tempDir / "archive.bin", std::ios::binary);
    auto metadata = FileCompressor::processFiles(fileGroup, archive, pathToHashMap, GetCompressionType());

    EXPECT_EQ(metadata.size(), 4);  // Four files in metadata
}

TEST_P(FileCompressorParameterizedTest, CompressionDecompression) {
    FileCompressor::compress(tempDir.string(), (tempDir / "archive.bin").string(), GetCompressionType());

    std::ifstream archive(tempDir / "archive.bin", std::ios::binary);
    EXPECT_TRUE(archive.good());

    std::filesystem::path outputDir = tempDir / "output";
    FileCompressor::decompress((tempDir / "archive.bin").string(), outputDir.string());

    EXPECT_TRUE(std::filesystem::exists(outputDir / "file1.txt"));
    EXPECT_TRUE(std::filesystem::exists(outputDir / "file2.txt"));
    EXPECT_TRUE(std::filesystem::exists(outputDir / "file3.txt"));
    
    // Verify file contents are correct
    std::ifstream file1(outputDir / "file1.txt");
    std::string content1((std::istreambuf_iterator<char>(file1)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content1, "Hello, World!");
    
    std::ifstream file3(outputDir / "file3.txt");
    std::string content3((std::istreambuf_iterator<char>(file3)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content3, "Different content");
}

TEST_P(FileCompressorParameterizedTest, EmptyFileHandling) {
    // Create an empty file
    std::string emptyFileName = "empty_file.txt";
    createTestFile(emptyFileName, "");
    
    // Compress the directory
    FileCompressor::compress(tempDir.string(), (tempDir / "archive.bin").string(), GetCompressionType());
    
    // Decompress to output dir
    std::filesystem::path outputDir = tempDir / "output";
    FileCompressor::decompress((tempDir / "archive.bin").string(), outputDir.string());
    
    // Check that the empty file exists in the output
    EXPECT_TRUE(std::filesystem::exists(outputDir / emptyFileName));
    
    // Verify it's actually empty
    std::ifstream emptyFile(outputDir / emptyFileName);
    std::string content((std::istreambuf_iterator<char>(emptyFile)), std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.empty());
}

// Instantiate the parameterized tests for each compression type
INSTANTIATE_TEST_SUITE_P(AllCompressionTypes,
                         FileCompressorParameterizedTest,
                         ::testing::Values(CompressionType::ZLIB,
                                           CompressionType::BROTLI,
                                           CompressionType::ZSTD),
                        [](const ::testing::TestParamInfo<CompressionType>& info) {
                            return CompressionTypeToString(info.param); // Now using the function from compression namespace
                        }
);

}  // namespace compression

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}