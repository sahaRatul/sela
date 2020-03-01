#ifndef _FRAME_H_
#define _FRAME_H_

#include "data/sela_frame.hpp"
#include "data/wav_frame.hpp"

namespace frame {
class FrameEncoder {
private:
    const data::WavFrame& wavFrame;

public:
    explicit FrameEncoder(const data::WavFrame& wavFrame);
    data::SelaFrame process();
};

class FrameDecoder {
private:
    const data::SelaFrame& selaFrame;

public:
    explicit FrameDecoder(const data::SelaFrame& selaFrame);
    data::WavFrame process();
};
}

#endif