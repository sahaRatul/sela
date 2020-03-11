#ifndef _WAV_FILE_H_
#define _WAV_FILE_H_

#include "../data/wav_chunk.hpp"
#include "../data/wav_frame.hpp"
#include <fstream>
#include <atomic>

namespace file {
class WavFile {
private:
    std::atomic<size_t> framesRead;
    size_t samplesPerChannelPerFrame = 2048;

public:
    data::WavChunk wavChunk;
    std::vector<data::WavFrame> wavFrames;
    void readFromFile(std::ifstream& inputFile);
    void demuxSamples();
    void writeToFile(std::ofstream& outputFile);
};
}

#endif