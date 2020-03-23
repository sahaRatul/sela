#include <catch.hpp>
#include <cmath>

#include "../src/include/lpc.hpp"

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

TEST_CASE("LPC Encoder/Decoder combined test")
{
    // Generate a sine wave
    std::vector<int32_t> samples;
    size_t vecsize = 2048;
    samples.reserve(vecsize);
    for (size_t i = 0; i < vecsize; i++) {
        samples.push_back((int32_t)(32767 * sin((double)i * (M_PI / 180))));
    }

    data::LpcDecodedData input = data::LpcDecodedData((uint8_t)16, std::move(samples));

    // Generate residues
    lpc::ResidueGenerator resGen = lpc::ResidueGenerator(input);
    data::LpcEncodedData encoded = resGen.process();

    // Generate samples
    lpc::SampleGenerator sampleGen = lpc::SampleGenerator(encoded);
    data::LpcDecodedData decoded = sampleGen.process();

    REQUIRE(input.samples.size() == decoded.samples.size());
    REQUIRE(input.samples == decoded.samples);
}