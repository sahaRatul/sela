#ifndef _WAV_CHUNK_H_
#define _WAV_CHUNK_H_

#include "wav_sub_chunk.hpp"
#include <cstdint>

namespace data {
class WavChunk {
public:
    std::string chunkId;
    uint32_t chunkSize;
    std::string format;
    WavFormatSubChunk formatSubChunk;
    WavDataSubChunk dataSubChunk;
    std::vector<WavSubChunk> wavSubChunks;
};
}

#endif