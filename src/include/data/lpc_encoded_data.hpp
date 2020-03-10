#ifndef _LPC_ENCODED_DATA_H_
#define _LPC_ENCODED_DATA_H_

#include <cstdint>
#include <vector>

namespace data {
class LpcEncodedData {
public:
    uint8_t optimalLpcOrder;
    uint8_t bitsPerSample;
    const std::vector<int32_t> quantizedReflectionCoefficients;
    const std::vector<int32_t> residues;
    LpcEncodedData(uint8_t optimalLpcOrder, uint8_t bitsPerSample, const std::vector<int32_t>&& quantizedReflectionCoefficients, const std::vector<int32_t>&& residues) noexcept
        : optimalLpcOrder(optimalLpcOrder)
        , bitsPerSample(bitsPerSample)
        , quantizedReflectionCoefficients(quantizedReflectionCoefficients)
        , residues(residues)
    {
    }
};
}

#endif