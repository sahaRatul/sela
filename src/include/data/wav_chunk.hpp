#ifndef _WAV_CHUNK_H_
#define _WAV_CHUNK_H_

#include "wav_sub_chunk.hpp"
#include <cstdint>

namespace data {
class WavChunk {
public:
    std::string chunkId;
    int32_t chunkSize;
    std::string format;
    std::vector<WavSubChunk> wavSubChunks;
};
}

#endif