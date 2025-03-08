#ifndef ZSTANDARD_COMPRESSOR_H
#define ZSTANDARD_COMPRESSOR_H

#include "Compressor.h"

namespace compression {

class ZStandardCompressor : public Compressor {
public:
    void compressStream(std::istream& input, std::ostream& output) const override;
    size_t decompressStream(std::istream& input, std::ostream& output) const override;
};

} // namespace compression

#endif // ZSTANDARD_COMPRESSOR_H
