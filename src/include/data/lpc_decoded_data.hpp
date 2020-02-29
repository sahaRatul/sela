#ifndef _LPC_DECODED_DATA_H_
#define _LPC_DECODED_DATA_H_

#include <cstdint>
#include <vector>

namespace data {
class LpcDecodedData {
public:
    std::vector<int32_t> samples;
    uint8_t bitsPerSample;
    LpcDecodedData(std::vector<int32_t> samples, uint8_t bitsPerSample)
        : samples(samples)
        , bitsPerSample(bitsPerSample)
    {
    }
};
}

#endif