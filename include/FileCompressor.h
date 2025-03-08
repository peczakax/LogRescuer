#ifndef FILECOMPRESSOR_H
#define FILECOMPRESSOR_H

#include <string>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>

// forward declarations
namespace meta {
enum class CompressionType;
class Compressor;
class FileMeta;
}

namespace compression {

// Class responsible for compressing and decompressing files
class FileCompressor {
public:
    // Compress files from a directory into a single archive file
    static void compress(const std::string& rootDir, const std::string& outputFile, CompressionType compType);
    
    // Extract files from an archive to the specified output directory
    static void decompress(const std::string& archiveFile, const std::string& outputDir);

    // Container for grouping unique files and their duplicates
    struct FileGroup {
        std::vector<std::pair<std::filesystem::path, std::string>> uniqueFiles;    // Files with unique content (full path, relative path)
        std::vector<std::pair<std::filesystem::path, std::string>> duplicateFiles; // Files with duplicate content
    };
    
    // Calculate hashes for all files and return maps for lookup
    static std::pair<std::unordered_map<std::string, std::string>, std::unordered_map<std::string, std::string>> 
    computeHashes(const std::vector<std::filesystem::path>& filePaths, const std::filesystem::path& rootPath);
    
    // Separate files into unique and duplicate groups based on their hashes
    static FileGroup groupFiles(const std::vector<std::filesystem::path>& filePaths,
                        const std::filesystem::path& rootPath,
                        const std::unordered_map<std::string, std::string>& pathToHashMap,
                        const std::unordered_map<std::string, std::string>& hashToPathMap);

    // Compress unique files and track duplicates, returning metadata for all files
    static std::vector<meta::FileMeta> processFiles(const FileGroup& fileGroup,
                                     std::ofstream& archive,
                                     const std::unordered_map<std::string, std::string>& pathToHashMap,
                                     CompressionType compType);
    
    // Group metadata entries by hash to identify originals and duplicates
    static std::unordered_map<std::string, std::vector<std::reference_wrapper<const meta::FileMeta>>>
    groupMetadata(const std::vector<meta::FileMeta>& metadata);
    
    // Extract files from the archive to the output directory
    static void decompressFiles(std::ifstream& archive,
                        const std::string& outputDir,
                        const std::unordered_map<std::string, std::vector<std::reference_wrapper<const meta::FileMeta>>>& groupedMeta,
                        std::unique_ptr<Compressor>& decompressor);
                        
    // Display statistics about the compressed files
    static void displayStats(const std::vector<meta::FileMeta>& metadata);
};

}
#endif // FILECOMPRESSOR_H
