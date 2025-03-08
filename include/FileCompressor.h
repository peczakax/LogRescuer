#ifndef FILECOMPRESSOR_H
#define FILECOMPRESSOR_H

#include <string>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>

// forward declarations
namespace meta {
class FileMeta;
}

namespace compression {
    
enum class CompressionType;
class Compressor;

// Class responsible for compressing and decompressing files
class FileCompressor {
public:
    // Compress files from a directory into a single archive file
    static void compress(const std::string& rootDir, const std::string& outputFile, CompressionType compType);
    
    // Extract files from an archive to the specified output directory
    static void decompress(const std::string& archiveFile, const std::string& outputDir);

    // Calculate hashes for all files and return maps for lookup
    static std::pair<std::unordered_map<std::string, std::string>, std::unordered_map<std::string, std::string>> 
    computeHashes(const std::vector<std::filesystem::path>& filePaths, const std::filesystem::path& rootPath);
    

    static std::vector<meta::FileMeta> compressFiles(const std::string& inputDir, std::ofstream& archive,
                                                     CompressionType compType);
    
    // Extract files from the archive to the output directory
    static std::vector<meta::FileMeta>  decompressFiles(const std::string& outputDir, std::ifstream& archive);
                        
    // Display statistics about the compressed files
    static void displayStats(const std::vector<meta::FileMeta>& metadata);
};

}
#endif // FILECOMPRESSOR_H
