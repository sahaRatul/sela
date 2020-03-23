#include <algorithm>
#include <cmath>

#include "../include/rice.hpp"

namespace rice {
RiceEncoder::RiceEncoder(const data::RiceDecodedData& decodedData)
    : input(decodedData.decodedData)
{
}

inline void RiceEncoder::convertSignedToUnsigned()
{
    unsignedInput.reserve(input.size());
    for (int32_t x : input) {
        unsignedInput.push_back(x < 0 ? (-(x << 1)) - 1 : (x << 1));
    }
}

inline void RiceEncoder::calculateOptimumRiceParam()
{
    for (uint64_t i = 0; i < MAX_RICE_PARAM; i++) {
        size_t temp = 0;
        for (uint64_t x : unsignedInput) {
            temp += x >> i;
            temp += 1;
            temp += i;
        }
        bitSizes.push_back(temp);
    }
    requiredBits = *std::min_element(bitSizes.begin(), bitSizes.end());
    optimumRiceParam = (int32_t)(std::find(bitSizes.begin(), bitSizes.end(), requiredBits) - bitSizes.begin());
}

inline void RiceEncoder::generateEncodedBits()
{
    uint64_t bitReservoirSize = (uint64_t)(ceil((float)requiredBits / 32) * 32);
    bitOutput.reserve(bitReservoirSize);
    uint64_t temp = 0;

    for (uint64_t x : unsignedInput) {
        temp = x >> optimumRiceParam;
        // Write out 'temp' number of ones
        for (uint64_t j = 0; j < temp; j++) {
            bitOutput.push_back(true);
        }
        // Write out a zero
        bitOutput.push_back(false);
        // Write out last param bits of input
        for (int64_t j = (int32_t)optimumRiceParam - 1; j >= 0; j--) {
            bitOutput.push_back((bool)(x >> j & 0x1));
        }
    }

    // Clear out rest of bits
    for (uint64_t i = requiredBits; i < bitReservoirSize; i++) {
        bitOutput.push_back(false);
    }
}

inline void RiceEncoder::writeInts(std::vector<uint32_t>& output)
{
    uint64_t requiredInts = (uint64_t)ceil((float)requiredBits / 32);
    output.reserve((size_t)requiredInts);
    for (uint64_t i = 0; i < requiredInts; i++) {
        output.push_back(0);
        for (uint64_t j = 0; j < 32; j++) {
            output[i] |= ((uint64_t)bitOutput[i * 32 + j]) << j;
        }
    }
}

data::RiceEncodedData RiceEncoder::process()
{
    std::vector<uint32_t> output;
    convertSignedToUnsigned();
    calculateOptimumRiceParam();
    generateEncodedBits();
    writeInts(output);
    return data::RiceEncodedData(optimumRiceParam, (uint32_t)input.size(), std::move(output));
}
}