#ifndef _SELA_HEADER_H_
#define _SELA_HEADER_H_

#include <cstdint>

namespace data {
class SelaHeader {
private:
    const uint8_t magicNumber[4] = { 0x53, 0x65, 0x4c, 0x61 }; // SeLa in ASCII
public:
    uint32_t sampleRate;
    uint16_t bitsPerSample;
    uint8_t channels;
    uint32_t numFrames;
};
}

#endif