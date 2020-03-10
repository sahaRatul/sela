#ifndef _DECODER_H_
#define _DECODER_H_

#include "../file/sela_file.hpp"

namespace sela {
class Decoder {
private:
    void readFrames();
    void processFrames();
    std::ifstream& ifStream;
    file::SelaFile selaFile;

public:
    explicit Decoder(std::ifstream& ifStream)
        : ifStream(ifStream)
    {
    }
    void process();
};
}

#endif