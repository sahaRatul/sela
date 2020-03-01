#include "../src/include/frame.hpp"
#include "include/catch.hpp"

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

TEST_CASE("Frame Encoder/Decoder combined test")
{
    std::vector<const std::vector<int32_t>*> samples = std::vector<const std::vector<int32_t>*>(2, nullptr);

    // Generate a sine wave for channel 1
    std::vector<int32_t> sinWave = std::vector<int32_t>(2048, 0);
    for (int i = 0; i < sinWave.size(); i++) {
        sinWave[i] = (int32_t)(32767 * sin((double)i * (M_PI / 180)));
    }
    samples[0] = &sinWave;

    // Generate a cosine wave for channel 2
    std::vector<int32_t> cosWave = std::vector<int32_t>(2048, 0);
    for (int i = 0; i < sinWave.size(); i++) {
        cosWave[i] = (int32_t)(32767 * cos((double)i * (M_PI / 180)));
    }
    samples[1] = &cosWave;

    data::WavFrame* input = new data::WavFrame((uint8_t)16, samples);
    
    frame::FrameEncoder* encoder = new frame::FrameEncoder(*input);
    data::SelaFrame frame = encoder->process();

    frame::FrameDecoder* decoder = new frame::FrameDecoder(frame);
    data::WavFrame output = decoder->process();

    REQUIRE(input->samples.size() == output.samples.size());
    for(size_t i = 0; i < samples.size(); i++) {
        REQUIRE(*input->samples.at(i) == *output.samples.at(i));
    }
}