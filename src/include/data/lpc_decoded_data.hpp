#ifndef _LPC_DECODED_DATA_H_
#define _LPC_DECODED_DATA_H_

#include <cstdint>
#include <vector>

namespace data {
class LpcDecodedData {
public:
    const std::vector<int32_t>& samples;
    uint8_t bitsPerSample;
    LpcDecodedData(uint8_t bitsPerSample, std::vector<int32_t>& samples)
        : bitsPerSample(bitsPerSample)
        , samples(samples)
    {
    }
};
}

#endif