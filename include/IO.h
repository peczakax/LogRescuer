#ifndef IO_H
#define IO_H

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// Forward declarations
namespace meta {
class FileMeta;
}

namespace compression {
enum class CompressionType;
}

namespace io {

    // Checks for stream errors and throws exceptions when necessary
    void checkErrors(const std::ios& stream, const std::string& operation);

    // Verifies that the amount of data read matches the expected size
    void checkReadSize(size_t expected, size_t actual);

    // Template function to verify if a stream was successfully opened
    template<typename Stream>
    void checkOpen(const Stream& stream, const std::string& filepath,
                         const std::string& operation) {
        if (!stream.is_open()) {
            throw std::runtime_error(operation + " failed: Could not open file '" + filepath +
                               "' (errno: " + std::to_string(errno) + ")");
        }
    }

    // Safely reads a Plain Old Data type from an input stream
    template<typename T>
    typename std::enable_if<!std::is_const<T>::value>::type
    read(std::ifstream& stream, T& data) {
        stream.read(reinterpret_cast<char*>(&data), sizeof(T));
        checkErrors(stream, "Read");
        checkReadSize(sizeof(T), stream.gcount());
    }

    // Safely writes a POD type to an output stream
    template<typename T>
    void write(std::ofstream& stream, const T& data) {
        stream.write(reinterpret_cast<const char*>(&data), sizeof(T));
        checkErrors(stream, "Write");
    }

    // Reads a block of POD data into a pre-allocated buffer
    template<typename T>
    void readBuffer(std::ifstream& stream, T* data, uint64_t size) {
        stream.read(reinterpret_cast<char*>(data), size);
        checkErrors(stream, "Read");
        checkReadSize(size, stream.gcount());
    }

    // Writes a block of POD data from a buffer to an output stream
    template<typename T>
    void writeBuffer(std::ofstream& stream, const T* data, uint64_t size) {
        stream.write(reinterpret_cast<const char*>(data), size);
        checkErrors(stream, "Write");
    }

    // Writes a string to an output stream
    void write(std::ofstream &stream, const std::string &str);

    // Reads a string from an input stream
    void read(std::ifstream &stream, std::string &str);
    
    // Writes footer to the stream
    void writeFooter(std::ofstream& stream, compression::CompressionType compType, uint64_t uniqueCount, uint64_t duplicateCount, uint64_t metaOffset);

    // Reads footer from the stream
    void readFooter(std::ifstream& stream, compression::CompressionType& compType, uint64_t& uniqueCount, uint64_t& duplicateCount, uint64_t& metaOffset);

    // Reads file metadata from an input stream
    meta::FileMeta read(std::ifstream &archive);
    
    // Writes metadata to the stream
    void writeMetadata(std::ofstream& archive, const std::vector<meta::FileMeta>& metadata, compression::CompressionType compType);
    
    // Reads metadata from the stream
    std::vector<meta::FileMeta> readMetadata(std::ifstream& archive, compression::CompressionType& compType);

    // Recursively scan a directory and return all file paths
    std::vector<std::filesystem::path> scanDirectory(const std::string& rootDir, bool skipEmptyFiles = true);
};

#endif // IO_H
