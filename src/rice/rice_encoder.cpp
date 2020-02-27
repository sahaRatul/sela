#include <iostream>
#include "../include/rice.hpp"

namespace rice
{
    RiceEncoder::RiceEncoder(data::RiceDecodedData decodedData) {
        input = decodedData.decodedData;
    }

    void RiceEncoder::convertSignedToUnsigned() {
        unsignedInput.reserve(input.size());
        size_t position = 0;
        for(int32_t x: input) {
            unsignedInput.push_back(x < 0 ? (-(x << 1)) - 1 : (x << 1));
        }
    }

    void RiceEncoder::process() {
        convertSignedToUnsigned();
    }
}