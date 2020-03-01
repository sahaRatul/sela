#ifndef _WAV_FRAME_H_
#define _WAV_FRAME_H_

#include <cstdint>
#include <vector>

namespace data {
class WavFrame {
public:
    uint8_t bitsPerSample;
    const std::vector<const std::vector<int32_t> *>& samples;
    WavFrame(uint8_t bitsPerSample, const std::vector<const std::vector<int32_t> *>& samples)
        : bitsPerSample(bitsPerSample)
        , samples(samples)
    {
    }
};
}

#endif