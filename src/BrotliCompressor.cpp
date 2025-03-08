#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include <brotli/encode.h>
#include <brotli/decode.h>

#include "BrotliCompressor.h"

namespace compression {

void BrotliCompressor::compressStream(std::istream& input, std::ostream& output) const {
    // Create encoder with automatic cleanup
    auto deleter = [](BrotliEncoderState* state) { BrotliEncoderDestroyInstance(state); };
    std::unique_ptr<BrotliEncoderState, decltype(deleter)> encoder(
        BrotliEncoderCreateInstance(nullptr, nullptr, nullptr), deleter);
    
    if (!encoder) {
        throw std::runtime_error("Failed to create Brotli encoder");
    }
    
    // Set default compression parameters
    BrotliEncoderSetParameter(encoder.get(), BROTLI_PARAM_QUALITY, BROTLI_DEFAULT_QUALITY);
    BrotliEncoderSetParameter(encoder.get(), BROTLI_PARAM_LGWIN, BROTLI_DEFAULT_WINDOW);
    
    std::vector<uint8_t> inputBuffer(BUFFER_SIZE);
    std::vector<uint8_t> outputBuffer(BrotliEncoderMaxCompressedSize(BUFFER_SIZE));
    
    bool isEndOfStream = false;
    
    while (!isEndOfStream) {
        // Read input chunk
        input.read(reinterpret_cast<char*>(inputBuffer.data()), BUFFER_SIZE);
        size_t bytesRead = input.gcount();
        isEndOfStream = input.eof();
        
        if (input.fail() && !isEndOfStream) {
            throw std::runtime_error("Error reading from input stream");
        }
        
        // Compress chunk
        size_t availableIn = bytesRead;
        const uint8_t* nextIn = inputBuffer.data();
        BrotliEncoderOperation op = isEndOfStream ? BROTLI_OPERATION_FINISH : BROTLI_OPERATION_PROCESS;
        
        while (availableIn > 0 || BrotliEncoderHasMoreOutput(encoder.get())) {
            size_t availableOut = outputBuffer.size();
            uint8_t* nextOut = outputBuffer.data();
            
            if (!BrotliEncoderCompressStream(encoder.get(), op, &availableIn, &nextIn, 
                                            &availableOut, &nextOut, nullptr)) {
                throw std::runtime_error("Brotli compression failed");
            }

            // Write compressed data
            size_t bytesCompressed = nextOut - outputBuffer.data();
            if (bytesCompressed > 0) {
                output.write(reinterpret_cast<char*>(outputBuffer.data()), bytesCompressed);
                if (!output) {
                    throw std::runtime_error("Failed to write compressed data");
                }
            }
        }
    }
}

size_t BrotliCompressor::decompressStream(std::istream& input, std::ostream& output) const {   
    // Create decoder with automatic cleanup
    auto deleter = [](BrotliDecoderState* state) { BrotliDecoderDestroyInstance(state); };
    std::unique_ptr<BrotliDecoderState, decltype(deleter)> decoder(
        BrotliDecoderCreateInstance(nullptr, nullptr, nullptr), deleter);
    
    if (!decoder) {
        throw std::runtime_error("Failed to create Brotli decoder");
    }
    
    std::vector<uint8_t> inputBuffer(BUFFER_SIZE);
    std::vector<uint8_t> outputBuffer(BUFFER_SIZE);
    size_t totalBytesDecompressed = 0;
    
    BrotliDecoderResult result = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
    size_t availableIn = 0;
    const uint8_t* nextIn = nullptr;
    
    while (result != BROTLI_DECODER_RESULT_SUCCESS) {
        // Read more input if needed
        if (availableIn == 0 && result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
            input.read(reinterpret_cast<char*>(inputBuffer.data()), BUFFER_SIZE);
            if (input.fail() && !input.eof()) {
                throw std::runtime_error("Error reading from input stream");
            }
            availableIn = input.gcount();
            if (availableIn == 0 && input.eof()) {
                throw std::runtime_error("Unexpected end of compressed stream");
            }
            nextIn = inputBuffer.data();
        }
        
        // Prepare output buffer
        size_t availableOut = outputBuffer.size();
        uint8_t* nextOut = outputBuffer.data();
        
        // Decompress
        result = BrotliDecoderDecompressStream(
            decoder.get(), &availableIn, &nextIn, &availableOut, &nextOut, nullptr);
            
        // Write decompressed data
        size_t bytesDecompressed = nextOut - outputBuffer.data();
        if (bytesDecompressed > 0) {
            output.write(reinterpret_cast<char*>(outputBuffer.data()), bytesDecompressed);
            if (!output) {
                throw std::runtime_error("Failed to write decompressed data");
            }
            totalBytesDecompressed += bytesDecompressed;
        }
        
        // Handle errors
        if (result == BROTLI_DECODER_RESULT_ERROR) {
            throw std::runtime_error("Brotli decompression error");
        }
    }
    
    return totalBytesDecompressed;
}

}  // End of compression namespace