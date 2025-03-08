#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <zlib.h>

#include "ZlibCompressor.h"

namespace compression {
    
void ZlibCompressor::compressStream(std::istream& input, std::ostream& output) const {
    // Create compressor with automatic cleanup
    z_stream zs = {};    
    if (deflateInit(&zs, Z_DEFAULT_COMPRESSION) != Z_OK) {
        throw std::runtime_error("Failed to initialize zlib compressor");
    }
    struct ZStreamCleanup { 
        z_stream* z; 
        ~ZStreamCleanup() { 
            deflateEnd(z); 
        } 
    } cleanup{&zs};
    
    std::vector<Bytef> inBuffer(BUFFER_SIZE);
    std::vector<Bytef> outBuffer(BUFFER_SIZE);
    
    // Process all input data
    do {
        // Read input chunk
        input.read(reinterpret_cast<char*>(inBuffer.data()), BUFFER_SIZE);
        if (input.fail() && !input.eof()) {
            throw std::runtime_error("Error reading from input stream");
        }
        zs.avail_in = static_cast<uInt>(input.gcount());
        zs.next_in = inBuffer.data();
        
        int flush = input.eof() ? Z_FINISH : Z_NO_FLUSH;
        
        // Compress input until all processed or output buffer full
        do {
            zs.avail_out = BUFFER_SIZE;
            zs.next_out = outBuffer.data();
            
            int ret = deflate(&zs, flush);
            if (ret == Z_STREAM_ERROR) {
                throw std::runtime_error("Zlib compression failed");
            }
            
            // Write compressed data
            size_t have = BUFFER_SIZE - zs.avail_out;
            output.write(reinterpret_cast<char*>(outBuffer.data()), have);
            if (!output) {
                throw std::runtime_error("Failed to write compressed data");
            }
            
        } while (zs.avail_out == 0);
        
    } while (!input.eof());
}

size_t ZlibCompressor::decompressStream(std::istream& input, std::ostream& output) const {
    // Create decompressor with automatic cleanup
    z_stream zs = {};
    if (inflateInit(&zs) != Z_OK) {
        throw std::runtime_error("Failed to initialize zlib decompressor");
    }
    struct ZStreamCleanup { 
        z_stream* z;
        ~ZStreamCleanup() { 
            inflateEnd(z); 
        }
    } cleanup{&zs};
    
    std::vector<Bytef> inBuffer(BUFFER_SIZE);
    std::vector<Bytef> outBuffer(BUFFER_SIZE);
    size_t totalDecompressedBytes = 0;
    int ret;
    
    do {
        // Read compressed data chunk
        input.read(reinterpret_cast<char*>(inBuffer.data()), BUFFER_SIZE);
        if (input.fail() && !input.eof()) {
            throw std::runtime_error("Error reading from input stream");
        }
        zs.avail_in = static_cast<uInt>(input.gcount());
        zs.next_in = inBuffer.data();
        
        // Process until no more input or error
        while (zs.avail_in > 0) {
            zs.avail_out = BUFFER_SIZE;
            zs.next_out = outBuffer.data();
            
            ret = inflate(&zs, Z_NO_FLUSH);
            if (ret < 0) {
                throw std::runtime_error("Zlib decompression failed: error code " + std::to_string(ret));
            }
            
            // Write decompressed data
            size_t have = BUFFER_SIZE - zs.avail_out;
            output.write(reinterpret_cast<char*>(outBuffer.data()), have);
            if (!output) {
                throw std::runtime_error("Failed to write decompressed data");
            }
            totalDecompressedBytes += have;
            
            if (ret == Z_STREAM_END) {
                break;
            }
        }
    } while (ret != Z_STREAM_END && input.gcount() > 0);

    return totalDecompressedBytes;
}

}  // End of compression namespace
