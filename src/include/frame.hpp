#ifndef _FRAME_H_
#define _FRAME_H_

#include "data/sela_frame.hpp"
#include "data/wav_frame.hpp"

namespace frame {
class FrameEncoder {
private:
    data::WavFrame& wavFrame;

public:
    explicit FrameEncoder(data::WavFrame& wavFrame);
    data::SelaFrame process();
};

class FrameDecoder {
private:
    data::SelaFrame& selaFrame;

public:
    explicit FrameDecoder(data::SelaFrame& selaFrame);
    data::WavFrame process();
};
}

#endif