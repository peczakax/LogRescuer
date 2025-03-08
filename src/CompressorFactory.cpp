#include <stdexcept>

// Conditionally include compressor implementations based on available libraries
#ifdef HAVE_ZLIB
#include "ZlibCompressor.h"
#endif

#ifdef HAVE_BROTLI
#include "BrotliCompressor.h"
#endif

#ifdef HAVE_ZSTD
#include "ZStandardCompressor.h"
#endif

#include "CompressorFactory.h"

namespace compression {

std::unique_ptr<Compressor> createCompressor(CompressionType type) {
    switch (type) {  // Select compressor implementation based on requested type
        #ifdef HAVE_ZLIB
        case CompressionType::ZLIB:
            return std::make_unique<ZlibCompressor>();  // Create and return a Zlib compressor
        #endif
        #ifdef HAVE_BROTLI
        case CompressionType::BROTLI:
            return std::make_unique<BrotliCompressor>();  // Create and return a Brotli compressor
        #endif
        #ifdef HAVE_ZSTD
        case CompressionType::ZSTD:
            return std::make_unique<ZStandardCompressor>();  // Create and return a ZStandard compressor
        #endif
        default:
            throw std::runtime_error("No supported compression method available");  // Throw if no suitable compressor found
    }
}

std::string CompressionTypeToString(CompressionType type) {
    switch (type) {
        #ifdef HAVE_ZLIB
        case CompressionType::ZLIB: return "ZLIB";
        #endif
        #ifdef HAVE_BROTLI
        case CompressionType::BROTLI: return "BROTLI";
        #endif
        #ifdef HAVE_ZSTD
        case CompressionType::ZSTD: return "ZSTD";
        #endif
        default: return "UNKNOWN";
    }
}

}  // End of compression namespace