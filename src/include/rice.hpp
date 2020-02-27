#ifndef _RICE_H_
#define _RICE_H_

#include <vector>

#include "./data/rice_decoded_data.hpp"
#include "./data/rice_encoded_data.hpp"

namespace rice {
class RiceEncoder {
private:
    std::vector<int32_t> input;
    std::vector<uint64_t> unsignedInput;
    std::vector<uint32_t> output;
    std::vector<bool> bitOutput;
    std::vector<size_t> bitSizes;
    uint32_t optimumRiceParam;
    size_t requiredBits;
    const int32_t maxRiceParam = 20;
    inline void convertSignedToUnsigned();
    inline void calculateOptimumRiceParam();
    inline void generateEncodedBits();
    inline void writeInts();

public:
    RiceEncoder(data::RiceDecodedData decodedData);
    data::RiceEncodedData process();
};

class RiceDecoder {
private:
    std::vector<uint32_t> input;
    std::vector<bool> bitInput;
    std::vector<uint64_t> unsignedOutput;
    std::vector<int32_t> output;
    uint32_t dataCount;
    uint32_t optimumRiceParam;
    inline void generateEncodedBits();
    inline void generateDecodedUnsignedInts();
    inline void convertUnsignedToSigned();

public:
    RiceDecoder(data::RiceEncodedData encodedData);
    data::RiceDecodedData process();
};
} // namespace rice

#endif