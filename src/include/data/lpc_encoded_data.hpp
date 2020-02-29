#ifndef _LPC_ENCODED_DATA_H_
#define _LPC_ENCODED_DATA_H_

#include <cstdint>
#include <vector>

namespace data {
class LpcEncodedData {
public:
    uint8_t optimalLpcOrder;
    uint8_t bitsPerSample;
    std::vector<int32_t> quantizedReflectionCoefficients;
    std::vector<int32_t> residues;
    LpcEncodedData(uint8_t optimalLpcOrder, uint8_t bitsPerSample, std::vector<int32_t> quantizedReflectionCoefficients, std::vector<int32_t> residues)
    {
        this->optimalLpcOrder = optimalLpcOrder;
        this->bitsPerSample = bitsPerSample;
        this->quantizedReflectionCoefficients = quantizedReflectionCoefficients;
        this->residues = residues;
    }
};
}

#endif