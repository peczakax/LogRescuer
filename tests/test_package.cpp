#include <iostream>
#include <string>

#include "CompressorFactory.h"

int main() {
    try {
        std::cout << "Testing LogRescuer package" << std::endl;
        
        // Create a compressor and verify it works
        #ifdef HAVE_ZLIB
        auto compressor = compression::createCompressor(compression::CompressionType::ZLIB);
        std::cout << "Successfully created a ZLIB compressor" << std::endl;
        #elif defined(HAVE_BROTLI)
        auto compressor = compression::createCompressor(compression::CompressionType::BROTLI);
        std::cout << "Successfully created a BROTLI compressor" << std::endl;
        #elif defined(HAVE_ZSTD)
        auto compressor = compression::createCompressor(compression::CompressionType::ZSTD);
        std::cout << "Successfully created a ZSTD compressor" << std::endl;
        #else
        #error "No compression method available"
        #endif

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
