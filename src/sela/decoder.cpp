#include "../include/sela/decoder.hpp"
#include "../include/frame.hpp"
#include "../include/data/exception.hpp"

#include <iostream>
#include <thread>

namespace sela {

class LoopThrough {
    public:
    const size_t begin;
    const size_t end;
    const std::vector<data::SelaFrame>& selaFrames;
    LoopThrough(size_t begin, size_t end, const std::vector<data::SelaFrame>& selaFrames) : begin(begin), end(end), selaFrames(selaFrames){}
    std::vector<data::WavFrame> process() {
        if((begin >= end) || (end > selaFrames.size())) {
            data::Exception("Something is wrong");
        }
        std::vector<data::WavFrame> wavFrames;
        wavFrames.reserve(end - begin);

        for(size_t i = begin; i < end; i++) {
            frame::FrameDecoder decoder = frame::FrameDecoder(selaFrames[i]);
            data::WavFrame wavFrame = decoder.process();
            wavFrames.push_back(wavFrame);
        }
        return wavFrames;
    }
};

void Decoder::readFrames()
{
    selaFile.readFromFile(ifStream);
}

void Decoder::processFrames()
{
    size_t numThreads = std::thread::hardware_concurrency();

    std::vector<std::thread> threadPool;
    threadPool.reserve(numThreads);

    const size_t framesPerThread = selaFile.selaFrames.size() / numThreads;

    size_t begin = 0;
    size_t end = framesPerThread;
    for(size_t i = 0; i < numThreads; i++) {
        LoopThrough data = LoopThrough(begin, end, selaFile.selaFrames);
        threadPool.push_back(std::thread(&LoopThrough::process, data));
        begin += framesPerThread;
        end = ((i == numThreads - 2) ? (selaFile.selaFrames.size()) : (end + framesPerThread));
    }

    for(size_t i = 0; i < numThreads; i++) {
        threadPool[i].join();
    }
}

void Decoder::process()
{
    readFrames();
    processFrames();
}
}