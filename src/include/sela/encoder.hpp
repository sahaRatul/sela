#ifndef _ENCODER_H_
#define _ENCODER_H_

#include "../data/sela_frame.hpp"
#include "../file/sela_file.hpp"
#include "../file/wav_file.hpp"

namespace sela {
class Encoder {
private:
    void readFrames();
    void processFrames(std::vector<data::SelaFrame>& encodedSelaFrames);
    std::ifstream& ifStream;
    file::WavFile wavFile;

public:
    explicit Encoder(std::ifstream& ifStream)
        : ifStream(ifStream)
    {
    }
    file::SelaFile process();
};
}

#endif