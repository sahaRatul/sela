#include "../include/sela/decoder.hpp"
#include "../include/frame.hpp"

#include <iostream>
#include <algorithm>
#include <execution>

namespace sela {
void Decoder::readFrames()
{
    selaFile.readFromFile(ifStream);
}

void Decoder::processFrames()
{
    std::for_each(std::execution::par_unseq, selaFile.selaFrames.begin(), selaFile.selaFrames.end(), [](data::SelaFrame selaFrame) {
        frame::FrameDecoder decoder = frame::FrameDecoder(selaFrame);
        data::WavFrame wavFrame = decoder.process();
    });
}

void Decoder::process()
{
    readFrames();
    processFrames();
}
}