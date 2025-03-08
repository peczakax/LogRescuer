#include <iostream>
#include <memory>
#include <vector>
#include <cstring>
#include <iomanip>
#include <functional>

#include <zstd.h>

#include "ZStandardCompressor.h"

// using ZSTD v1.4.8

namespace compression {

void ZStandardCompressor::compressStream(std::istream& input, std::ostream& output) const {
    // Create compression context with automatic cleanup
    auto deleter = [](ZSTD_CStream* stream) { ZSTD_freeCStream(stream); };
    std::unique_ptr<ZSTD_CStream, decltype(deleter)> cstream(ZSTD_createCStream(), deleter);
    
    if (!cstream) {
        throw std::runtime_error("Failed to create ZSTD compression context");
    }
    // Initialize with default compression level
    ZSTD_initCStream(cstream.get(), ZSTD_CLEVEL_DEFAULT);

    // Create buffers for input and output
    std::vector<char> inputBuffer(ZSTD_CStreamInSize());
    std::vector<char> outputBuffer(ZSTD_CStreamOutSize());

    // Process input stream
    while (input) {
        input.read(inputBuffer.data(), inputBuffer.size());
        if (input.fail() && !input.eof()) {
            throw std::runtime_error("Error reading from input stream");
        }
        size_t bytesRead = input.gcount();
        if (bytesRead == 0) {
            break;
        }

        // Set up input buffer
        ZSTD_inBuffer inBuf = {inputBuffer.data(), bytesRead, 0};

        // Process current chunk
        while (inBuf.pos < inBuf.size) {
            // Set up output buffer
            ZSTD_outBuffer outBuf = {outputBuffer.data(), outputBuffer.size(), 0};
            
            // Compress
            size_t remaining = ZSTD_compressStream(cstream.get(), &outBuf, &inBuf);
            if (ZSTD_isError(remaining)) {
                throw std::runtime_error(std::string("ZSTD compression error: ") + 
                                       ZSTD_getErrorName(remaining));
            }

            // Write compressed data to output
            output.write(outputBuffer.data(), outBuf.pos);
            if (!output) {
                throw std::runtime_error("Failed to write compressed data");
            }
        }
    }

    // Flush remaining data
    ZSTD_outBuffer outBuf = {outputBuffer.data(), outputBuffer.size(), 0};
    if (ZSTD_endStream(cstream.get(), &outBuf) > 0) {
        throw std::runtime_error("ZSTD compression error: couldn't flush remaining data");
    }
    output.write(outputBuffer.data(), outBuf.pos);
    if (!output) {
        throw std::runtime_error("Failed to write final compressed data");
    }
}

size_t ZStandardCompressor::decompressStream(std::istream& input, std::ostream& output) const {
    // Create and initialize decompression context
    auto deleter = [](ZSTD_DStream* stream) { ZSTD_freeDStream(stream); };
    std::unique_ptr<ZSTD_DStream, decltype(deleter)> dstream(ZSTD_createDStream(), deleter);
    
    if (!dstream) {
        throw std::runtime_error("Failed to create ZSTD decompression context");
    }

    ZSTD_initDStream(dstream.get());

    // Prepare buffers
    std::vector<char> inputBuffer(ZSTD_DStreamInSize());
    std::vector<char> outputBuffer(ZSTD_DStreamOutSize());
    size_t totalDecompressedBytes = 0;

    // Process input stream
    while (input) {
        input.read(inputBuffer.data(), inputBuffer.size());
        if (input.fail() && !input.eof()) {
            throw std::runtime_error("Error reading from input stream");
        }
        size_t bytesRead = input.gcount();
        if (bytesRead == 0) {
            break;
        }

        ZSTD_inBuffer inBuf = {inputBuffer.data(), bytesRead, 0};

        while (inBuf.pos < inBuf.size) {
            ZSTD_outBuffer outBuf = {outputBuffer.data(), outputBuffer.size(), 0};
            
            size_t ret = ZSTD_decompressStream(dstream.get(), &outBuf, &inBuf);
            if (ZSTD_isError(ret)) {
                throw std::runtime_error(std::string("ZSTD decompression error: ") + 
                                      ZSTD_getErrorName(ret));
            }
            
            output.write(outputBuffer.data(), outBuf.pos);
            if (!output) {
                throw std::runtime_error("Failed to write decompressed data");
            }
            totalDecompressedBytes += outBuf.pos;
            
            if (ret == 0) {
                break;  // End of frame reached
            }
        }
    }

    return totalDecompressedBytes;
}

}  // End of compression namespace
