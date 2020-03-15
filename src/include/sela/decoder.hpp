#ifndef _DECODER_H_
#define _DECODER_H_

#include "../file/sela_file.hpp"
#include "../file/wav_file.hpp"
#include "../data/wav_frame.hpp"

namespace sela {
class Decoder {
private:
    void readFrames();
    void processFrames(std::vector<data::WavFrame>& decodedWavFrames);
    std::ifstream& ifStream;
    file::SelaFile selaFile;

public:
    explicit Decoder(std::ifstream& ifStream)
        : ifStream(ifStream)
    {
    }
    file::WavFile process();
};
}

#endif