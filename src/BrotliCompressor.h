#ifndef BROTLI_COMPRESSOR_H
#define BROTLI_COMPRESSOR_H

#include <iostream>

#include "Compressor.h"

namespace compression {

class BrotliCompressor : public Compressor {
public:
    void compressStream(std::istream& input, std::ostream& output) const override;
    size_t decompressStream(std::istream& input, std::ostream& output) const override;
private:
    static constexpr size_t BUFFER_SIZE = 65536; // 64KB buffer size
};

} // namespace compression

#endif // BROTLI_COMPRESSOR_H
