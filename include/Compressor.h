#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <iostream>

namespace compression {

// Interface defining compression operations that concrete compressors must implement
class Compressor {
public:
    virtual ~Compressor() = default;
    
    // Stream-based compression methods
    virtual void compressStream(std::istream& input, std::ostream& output) const = 0;            
    virtual size_t decompressStream(std::istream& input, std::ostream& output) const = 0;
};

} // namespace compression

#endif // COMPRESSOR_H
