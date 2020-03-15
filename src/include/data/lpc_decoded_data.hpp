#ifndef _LPC_DECODED_DATA_H_
#define _LPC_DECODED_DATA_H_

#include <cstdint>
#include <vector>

namespace data {
class LpcDecodedData {
public:
    uint8_t bitsPerSample;
    const std::vector<int32_t> samples;
    LpcDecodedData(uint8_t bitsPerSample, const std::vector<int32_t>&& samples) noexcept
        : bitsPerSample(bitsPerSample)
        , samples(samples)
    {
    }
};
}

#endif