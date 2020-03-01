#include "../include/frame.hpp"
#include "../include/lpc.hpp"
#include "../include/rice.hpp"

#include <vector>

namespace frame {
FrameEncoder::FrameEncoder(const data::WavFrame& wavFrame)
    : wavFrame(wavFrame)
{
}

data::SelaFrame FrameEncoder::process()
{
    std::vector<data::SelaSubFrame> subFrames;
    subFrames.reserve(wavFrame.samples.size());

    return data::SelaFrame(16);
}

}