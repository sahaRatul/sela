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
}

#endif