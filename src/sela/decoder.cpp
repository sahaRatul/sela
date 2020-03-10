#include "../include/sela/decoder.hpp"
#include "../include/frame.hpp"

#include <iostream>

namespace sela {
    void Decoder::readFrames() {
        selaFile.readFromFile(ifStream);
    }

    void Decoder::processFrames() {
        for(size_t i = 0; i < selaFile.selaFrames.size(); i++) {
            frame::FrameDecoder decoder = frame::FrameDecoder(selaFile.selaFrames[i]);
            data::WavFrame wavFrame = decoder.process();
        }
    }

    void Decoder::process() {
        readFrames();
        processFrames();
    }
}