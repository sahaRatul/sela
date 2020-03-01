#ifndef _RICE_H_
#define _RICE_H_

#include <cstdint>
#include <vector>

#include "./data/rice_decoded_data.hpp"
#include "./data/rice_encoded_data.hpp"

constexpr uint8_t MAX_RICE_PARAM = 20;

namespace rice {
class RiceEncoder {
private:
    const std::vector<int32_t>& input;
    std::vector<uint64_t> unsignedInput;
    std::vector<uint32_t> output;
    std::vector<bool> bitOutput;
    std::vector<size_t> bitSizes;
    uint32_t optimumRiceParam;
    size_t requiredBits;
    inline void convertSignedToUnsigned();
    inline void calculateOptimumRiceParam();
    inline void generateEncodedBits();
    inline void writeInts();

public:
    explicit RiceEncoder(const data::RiceDecodedData& decodedData);
    data::RiceEncodedData process();
};

class RiceDecoder {
private:
    const std::vector<uint32_t>& input;
    std::vector<bool> bitInput;
    std::vector<uint64_t> unsignedOutput;
    std::vector<int32_t> output;
    uint32_t dataCount;
    uint32_t optimumRiceParam;
    inline void generateEncodedBits();
    inline void generateDecodedUnsignedInts();
    inline void convertUnsignedToSigned();

public:
    explicit RiceDecoder(const data::RiceEncodedData& encodedData);
    data::RiceDecodedData process();
};
}

#endif