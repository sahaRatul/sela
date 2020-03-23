#include "../include/sela/decoder.hpp"
#include "../include/data/exception.hpp"
#include "../include/frame.hpp"

#include <thread>

namespace sela {

class LoopThrough {
public:
    const size_t begin;
    const size_t end;
    const std::vector<data::SelaFrame>& selaFrames;
    LoopThrough(size_t begin, size_t end, const std::vector<data::SelaFrame>& selaFrames)
        : begin(begin)
        , end(end)
        , selaFrames(selaFrames)
    {
    }
    void process(std::vector<data::WavFrame>& wavFrames)
    {
        if ((begin >= end) || (end > selaFrames.size())) {
            const std::string exceptionMessage = "Something is wrong";
            data::Exception(std::move(exceptionMessage));
        }
        wavFrames.reserve(end - begin);

        for (size_t i = begin; i < end; i++) {
            frame::FrameDecoder decoder = frame::FrameDecoder(selaFrames[i]);
            data::WavFrame wavFrame = decoder.process();
            wavFrames.push_back(wavFrame);
        }
    }
};

void Decoder::readFrames()
{
    selaFile.readFromFile(ifStream);
}

void Decoder::processFrames(std::vector<data::WavFrame>& decodedWavFrames)
{
    // Get number of threads supported by hardware
    size_t numThreads = std::thread::hardware_concurrency();
    decodedWavFrames.reserve(selaFile.selaFrames.size());

    if (numThreads > 0) {
        //Initialize ThreadPool
        std::vector<std::thread> threadPool;
        threadPool.reserve(numThreads);

        //Initialize output wavFrames
        std::vector<std::vector<data::WavFrame>> wavFrameSegments;
        wavFrameSegments.reserve(numThreads);
        for (size_t i = 0; i < numThreads; i++) {
            wavFrameSegments.push_back(std::vector<data::WavFrame>());
        }

        const size_t framesPerThread = selaFile.selaFrames.size() / numThreads;

        size_t begin = 0;
        size_t end = framesPerThread;
        for (size_t i = 0; i < numThreads; i++) {
            //Create worker object
            LoopThrough data = LoopThrough(begin, end, selaFile.selaFrames);

            //Start thread object and push thread to threadpool
            threadPool.push_back(std::thread(&LoopThrough::process, data, std::ref(wavFrameSegments[i])));

            //Increment beginning and end
            begin += framesPerThread;
            end = ((i == numThreads - 2) ? (selaFile.selaFrames.size()) : (end + framesPerThread));
        }

        for (size_t i = 0; i < numThreads; i++) {
            // Wait for worker threads to finish execution
            threadPool[i].join();
        }

        for (std::vector<data::WavFrame> segment : wavFrameSegments) {
            for (data::WavFrame wavFrame : segment) {
                decodedWavFrames.push_back(wavFrame);
            }
        }
    } else { //Single loop execution
        for (data::SelaFrame selaFrame : selaFile.selaFrames) {
            frame::FrameDecoder decoder = frame::FrameDecoder(selaFrame);
            data::WavFrame wavFrame = decoder.process();
            decodedWavFrames.push_back(wavFrame);
        }
    }
}

file::WavFile Decoder::process()
{
    std::vector<data::WavFrame> wavFrames;
    readFrames();
    processFrames(wavFrames);
    return file::WavFile(selaFile.selaHeader.sampleRate, selaFile.selaHeader.bitsPerSample, selaFile.selaHeader.channels, std::move(wavFrames));
}
}