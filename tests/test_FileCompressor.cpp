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

struct TestFile {
    std::string filename;
    std::string content;
};

const std::vector<TestFile> baseTestSet = {
    {"file1.txt", "Hello, World!"},
    {"file2.txt", "Hello, World!"},
    {"file3.txt", "Different content"},
    {"file4.txt", ""}
};

const std::vector<TestFile> extendedTestSet = {
    {"subset2_file1.log", "Error: System failure"},
    {"subset2_file2.log", "Error: System failure"},
    {"subset2_file3.log", "Warning: Low memory"},
    {"subset2_file4.log", "Critical: Service stopped"}
};

class FileCompressorTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
        std::filesystem::remove_all(tempDir);
    }

    void createTestFile(const std::string& filename, const std::string& content) {
        std::ofstream file(tempDir / filename);
        file << content;
    }

    std::filesystem::path tempDir;
};

class FileCompressorParameterizedTest 
    : public FileCompressorTest,
      public ::testing::WithParamInterface<std::tuple<CompressionType, std::vector<TestFile>>> {
public:
    using ParamType = std::tuple<CompressionType, std::vector<TestFile>>;

protected:
    CompressionType GetCompressionType() const {
        return std::get<0>(GetParam());
    }

    void SetUp() override {
        tempDir = std::filesystem::temp_directory_path() / "filecompressor_test";
        std::filesystem::create_directories(tempDir);

        const auto& testFiles = std::get<1>(GetParam());
        for (const auto& file : testFiles) {
            createTestFile(file.filename, file.content);
        }
    }
};

TEST_P(FileCompressorParameterizedTest, ComputeHashes) {
    auto files = io::scanDirectory(tempDir.string());
    auto [hashToPathMap, pathToHashMap] = FileCompressor::computeHashes(files, tempDir);

    const auto& testFiles = std::get<1>(GetParam());
    bool isSubset2 = testFiles[0].filename.find("subset2_") != std::string::npos;

    // Subset2 has different expectations:
    // - 3 unique contents (2 identical files + 2 different files)
    // - 4 non-empty files
    if (isSubset2) {
        EXPECT_EQ(hashToPathMap.size(), 3);
        EXPECT_EQ(pathToHashMap.size(), 4);
    } else {
        EXPECT_EQ(hashToPathMap.size(), 2);
        EXPECT_EQ(pathToHashMap.size(), 3);
    }
}

TEST_P(FileCompressorParameterizedTest, CompressFiles) {
    std::ofstream archive(tempDir / "archive.bin", std::ios::binary);
    auto metadata = FileCompressor::compressFiles(tempDir.string(), archive, GetCompressionType());

    // Get test set to determine expected metadata size
    const auto& testFiles = std::get<1>(GetParam());
    bool isSubset2 = testFiles[0].filename.find("subset2_") != std::string::npos;
    EXPECT_EQ(metadata.size(), isSubset2 ? 4 : 3);
    
    // Verify metadata has correct structure
    std::unordered_map<std::string, bool> foundPaths;
    int uniqueCount = 0;
    int duplicateCount = 0;
    
    for (const auto& meta : metadata) {
        // Verify each relative path appears exactly once in metadata
        EXPECT_FALSE(foundPaths[meta.relativePath]) << "Duplicate entry for " << meta.relativePath;
        foundPaths[meta.relativePath] = true;
        
        // Count uniques vs duplicates
        if (meta.isDuplicate()) {
            duplicateCount++;
        } else {
            uniqueCount++;
        }
    }
    
    // Verify expected count of unique and duplicate files
    if (isSubset2) {
        // Subset2: 3 unique files, 1 duplicate
        EXPECT_EQ(uniqueCount, 3);
        EXPECT_EQ(duplicateCount, 1);
    } else {
        // Subset1: 2 unique files, 1 duplicate
        EXPECT_EQ(uniqueCount, 2);
        EXPECT_EQ(duplicateCount, 1);
    }
}

TEST_P(FileCompressorParameterizedTest, CompressionDecompression) {
    FileCompressor::compress(tempDir.string(), (tempDir / "archive.bin").string(), GetCompressionType());

    std::ifstream archive(tempDir / "archive.bin", std::ios::binary);
    EXPECT_TRUE(archive.good());

    std::filesystem::path outputDir = tempDir / "output";
    FileCompressor::decompress((tempDir / "archive.bin").string(), outputDir.string());

    // Get test set to verify correct files
    const auto& testFiles = std::get<1>(GetParam());
    bool isSubset2 = testFiles[0].filename.find("subset2_") != std::string::npos;

    if (isSubset2) {
        EXPECT_TRUE(std::filesystem::exists(outputDir / "subset2_file1.log"));
        EXPECT_TRUE(std::filesystem::exists(outputDir / "subset2_file2.log"));
        EXPECT_TRUE(std::filesystem::exists(outputDir / "subset2_file3.log"));
        EXPECT_TRUE(std::filesystem::exists(outputDir / "subset2_file4.log"));
        
        std::ifstream file1(outputDir / "subset2_file1.log");
        std::string content1((std::istreambuf_iterator<char>(file1)), std::istreambuf_iterator<char>());
        EXPECT_EQ(content1, "Error: System failure");
        
        std::ifstream file3(outputDir / "subset2_file3.log");
        std::string content3((std::istreambuf_iterator<char>(file3)), std::istreambuf_iterator<char>());
        EXPECT_EQ(content3, "Warning: Low memory");
    } else {
        EXPECT_TRUE(std::filesystem::exists(outputDir / "file1.txt"));
        EXPECT_TRUE(std::filesystem::exists(outputDir / "file2.txt"));
        EXPECT_TRUE(std::filesystem::exists(outputDir / "file3.txt"));
        
        std::ifstream file1(outputDir / "file1.txt");
        std::string content1((std::istreambuf_iterator<char>(file1)), std::istreambuf_iterator<char>());
        EXPECT_EQ(content1, "Hello, World!");
        
        std::ifstream file3(outputDir / "file3.txt");
        std::string content3((std::istreambuf_iterator<char>(file3)), std::istreambuf_iterator<char>());
        EXPECT_EQ(content3, "Different content");
    }
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
    
    // Check that the empty file does not exist in the output
    EXPECT_FALSE(std::filesystem::exists(outputDir / emptyFileName));
}

// Instantiate the parameterized tests for each compression type
INSTANTIATE_TEST_SUITE_P(
    AllCompressionTypes,
    FileCompressorParameterizedTest,
    ::testing::Values(
        std::make_tuple(CompressionType::ZLIB, baseTestSet),
        std::make_tuple(CompressionType::ZLIB, extendedTestSet),
        std::make_tuple(CompressionType::BROTLI, baseTestSet),
        std::make_tuple(CompressionType::BROTLI, extendedTestSet),
        std::make_tuple(CompressionType::ZSTD, baseTestSet),
        std::make_tuple(CompressionType::ZSTD, extendedTestSet)
    ),
    [](const ::testing::TestParamInfo<FileCompressorParameterizedTest::ParamType>& info) {
        const auto& compression = std::get<0>(info.param);
        const auto& files = std::get<1>(info.param);
        return CompressionTypeToString(compression) + "_" + 
               (files[0].filename.find("subset2_") != std::string::npos ? "Subset2" : "Subset1");
    }
);

}  // namespace compression

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}