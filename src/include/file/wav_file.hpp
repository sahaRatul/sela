#ifndef _WAV_FILE_H_
#define _WAV_FILE_H_

#include "../data/wav_chunk.hpp"
#include <fstream>

namespace file {
class WavFile {
public:
    data::WavChunk wavChunk;
    void readFromFile(std::ifstream inputFile);
    void writeToFile(std::ofstream outputFile);
};
}

#endif