#include "../src/include/frame.hpp"
#include <catch.hpp>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

TEST_CASE("Frame Encoder/Decoder combined test ")
{
    std::vector<std::vector<int32_t>> samples = std::vector<std::vector<int32_t>>(2, std::vector<int32_t>());

    // Generate a sine wave for channel 1
    std::vector<int32_t> sinWave = std::vector<int32_t>(2048, 0);
    for (int i = 0; i < sinWave.size(); i++) {
        sinWave[i] = (int32_t)(INT16_MAX * sin((double)i * (M_PI / 180)));
    }
    samples[0] = sinWave;

    // Generate a cosine wave for channel 2
    std::vector<int32_t> cosineWave = std::vector<int32_t>(2048, 0);
    for (int i = 0; i < sinWave.size(); i++) {
        cosineWave[i] = (int32_t)(INT16_MAX * sin((double)i * (M_PI / 180)));
    }
    samples[1] = cosineWave;

    data::WavFrame input = data::WavFrame((uint8_t)16, std::move(samples));

    frame::FrameEncoder encoder = frame::FrameEncoder(std::move(input));
    data::SelaFrame frame = encoder.process();

    frame::FrameDecoder decoder = frame::FrameDecoder(frame);
    data::WavFrame output = decoder.process();

    REQUIRE(input.samples.size() == output.samples.size());
    for (size_t i = 0; i < samples.size(); i++) {
        REQUIRE(input.samples[i] == output.samples[i]);
    }
}

TEST_CASE("Frame Encoder/Decoder + difference coding combined test ")
{
    std::vector<std::vector<int32_t>> samples = std::vector<std::vector<int32_t>>(2, std::vector<int32_t>());

    // Generate a sine wave for channel 1
    std::vector<int32_t> sinWave = std::vector<int32_t>(2048, 0);
    for (int i = 0; i < sinWave.size(); i++) {
        sinWave[i] = (int32_t)(INT16_MAX * sin((double)i * (M_PI / 180)));
    }
    samples[0] = sinWave;

    // Generate a sine wave for channel 2
    std::vector<int32_t> sinWave2 = std::vector<int32_t>(2048, 0);
    for (int i = 0; i < sinWave.size(); i++) {
        sinWave2[i] = (int32_t)(INT16_MAX * sin((double)i * (M_PI / 180)));
    }
    samples[1] = sinWave2;

    data::WavFrame input = data::WavFrame((uint8_t)16, std::move(samples));

    frame::FrameEncoder encoder = frame::FrameEncoder(std::move(input));
    data::SelaFrame frame = encoder.process();

    frame::FrameDecoder decoder = frame::FrameDecoder(frame);
    data::WavFrame output = decoder.process();

    REQUIRE(input.samples.size() == output.samples.size());
    for (size_t i = 0; i < samples.size(); i++) {
        REQUIRE(input.samples[i] == output.samples[i]);
    }
}