#include <iostream>

#include "CompressorFactory.h"
#include "FileCompressor.h"

using namespace compression;


std::string print_default_compressions()
{
    #ifdef HAVE_BROTLI
    return "(default: brotli)";
    #elif defined(HAVE_ZLIB)
    return  "(default: zlib)";
    #elif defined(HAVE_ZSTD)
    return  "(default: zstd)";
    #endif
    return "";
}


std::string print_supported_compressions()
{
    bool first = true;
    std::string result = "";

    #ifdef HAVE_BROTLI
    result += "brotli";
    first = false;
    #endif
    
    #ifdef HAVE_ZLIB
    if (!first) {
        result += ", ";
    }
    result += "zlib";
    first = false;
    #endif
        
    #ifdef HAVE_ZSTD
    if (!first) {
        result += ", ";
    }
    result += "zstd";
    #endif
    return result + "";
}

void print_usage(const char* program_name) {
    std::cout << "LogRescuer - A time machine log compression and archival tool.\n"
              << "\n"
              << "Usage: " << program_name << " <command> <dir> <archive_file> [options]\n"
              << "\n"
              << "Commands:\n"
              << "  compress    - Create a compressed archive.\n"
              << "  decompress  - Extract an archive.\n"
              << "\n"
              << "Options:\n"
              << "  -c, --compression    Optionally specify a compression algorithm: [" << print_supported_compressions() << "] " << print_default_compressions() << "\n"
              << "  -h, --help           Print this help message.\n"
              << "\n"
              << "Example:\n"
              << "  " << program_name << " compress /var/logs logs_archive --compression=zlib\n\n";
}

compression::CompressionType parseCompressionType(const std::string& compression) {
    #ifdef HAVE_BROTLI
    if (compression == "--compression=brotli" || compression == "-c=brotli") {
        return compression::CompressionType::BROTLI;
    }
    #endif

    #ifdef HAVE_ZLIB
    if (compression == "--compression=zlib" || compression == "-c=zlib") {
        return compression::CompressionType::ZLIB;
    }    
    #endif
    
    #ifdef HAVE_ZSTD
    if (compression == "--compression=zstd" || compression == "-c=zstd") {
        return compression::CompressionType::ZSTD;
    }
    #endif

    throw std::runtime_error("Invalid compression type");
}

int main(int argc, char* argv[]) {
    if (argc < 2 || std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
        print_usage(argv[0]);
        return argc < 2 ? 1 : 0;
    }

    try {
        if (argc < 4) {
            throw std::invalid_argument("Insufficient arguments. Try '" + std::string(argv[0]) + " --help' for more information.");
        }

        std::string command = argv[1];
        if (command == "compress") {
            compression::CompressionType compType = compression::CompressionType::NONE;
            #ifdef HAVE_BROTLI
            compType = compression::CompressionType::BROTLI;
            #elif defined(HAVE_ZLIB)
            compType = compression::CompressionType::ZLIB;
            #elif defined(HAVE_ZSTD)
            compType = compression::CompressionType::ZSTD;
            #else
            #error "No compression method available"
            #endif

            if (argc > 4) {
                compType = parseCompressionType(argv[4]);
            }
            FileCompressor::compress(argv[2], argv[3], compType);
            std::cout << "Successfully compressed folder: " << argv[2] << " to archive file: " << argv[3] << "\n";
        } else if (command == "decompress") {
            FileCompressor::decompress(argv[3], argv[2]);
            std::cout << "Successfully decompressed archive file: " << argv[3] << " to folder: " << argv[2] << "\n";
        } else {
            throw std::invalid_argument("Unknown command '" + command + "'. Try '" + std::string(argv[0]) + " --help' for more information.");
        }
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
        return 1;
    }
}
