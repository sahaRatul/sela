#include "../include/sela/decoder.hpp"
#include "../include/frame.hpp"

#include <iostream>

namespace sela {
    void Decoder::readFrames() {
        selaFile.readFromFile(ifStream);
    }

    void Decoder::processFrames() {
        std::cout << selaFile.selaFrames.size() << std::endl;
        for(size_t i = 0; i < selaFile.selaFrames.size(); i++) {
            std::cout << "Decoded" << std::endl;
            frame::FrameDecoder decoder(selaFile.selaFrames[i]);
        }
    }

    void Decoder::process() {
        processFrames();
    }
}