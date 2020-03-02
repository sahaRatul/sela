#ifndef _WAV_SUB_CHUNK_H_
#define _WAV_SUB_CHUNK_H_

#include <cstdint>
#include <string>
#include <vector>

namespace data {
class WavSubChunk {
public:
    std::string subChunkId;
    uint32_t subChunkSize;
    std::vector<int8_t> subChunkData;
};

class WavFormatSubChunk : public WavSubChunk {
public:
    int16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
};

class WavDataSubChunk : public WavSubChunk {
public:
    std::vector<int32_t> samples;
    uint8_t bitsPerSample;
    WavDataSubChunk(uint8_t bitsPerSample)
        : bitsPerSample(bitsPerSample)
    {
    }
};
}

#endif