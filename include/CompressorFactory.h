#ifndef COMPRESSORFACTORY_H
#define COMPRESSORFACTORY_H

#include <memory>
#include <string>

#include "Compressor.h"

namespace compression {
    
// Enumeration of supported compression algorithms
enum class CompressionType {    
    #ifdef HAVE_BROTLI  // Conditionally include Brotli if the library is available
    BROTLI,            // Google's Brotli compression algorithm
    #endif
    #ifdef HAVE_ZSTD   // Conditionally include ZStandard if the library is available
    ZSTD,              // Facebook's ZStandard compression algorithm
    #endif
    #ifdef HAVE_ZLIB   // Conditionally include zlib if the library is available
    ZLIB,              // DEFLATE algorithm implementation
    #endif
    NONE              // No compression option
};

// Factory function that creates and returns a compressor instance based on the specified type
std::unique_ptr<Compressor> createCompressor(CompressionType type);

// Convert CompressionType to string representation
std::string CompressionTypeToString(CompressionType type);

}  // End of compression namespace

#endif // COMPRESSORFACTORY_H
