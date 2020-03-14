#ifndef _WAV_FILE_H_
#define _WAV_FILE_H_

#include "../data/wav_chunk.hpp"
#include "../data/wav_frame.hpp"
#include <atomic>
#include <fstream>

namespace file {
class WavFile {
public:
    size_t samplesPerChannelPerFrame = 2048;
    void demuxSamples();
    data::WavChunk wavChunk;
    WavFile() {}
    WavFile(uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels, std::vector<data::WavFrame>&& wavFrames);
    void readFromFile(std::ifstream& inputFile);
    void writeToFile(const std::ofstream& outputFile);
};
}

#endif