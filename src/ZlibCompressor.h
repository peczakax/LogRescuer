#ifndef ZLIB_COMPRESSOR_H
#define ZLIB_COMPRESSOR_H

#include <vector>
#include <cstdint>
#include <iostream>

#include "Compressor.h"

namespace compression {

class ZlibCompressor : public Compressor {
public:
    void compressStream(std::istream& input, std::ostream& output) override;
    size_t decompressStream(std::istream& input, std::ostream& output) override;

private:
    static constexpr size_t BUFFER_SIZE = 65536; // 64KB buffer size
};

} // namespace compression

#endif // ZLIB_COMPRESSOR_H
