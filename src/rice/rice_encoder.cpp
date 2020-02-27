#include <iostream>
#include "../include/rice.hpp"

namespace rice
{
    RiceEncoder::RiceEncoder(data::RiceDecodedData decodedData) {
        input = decodedData.decodedData;
    }

    inline void RiceEncoder::convertSignedToUnsigned() {
        unsignedInput.reserve(input.size());
        size_t position = 0;
        for(int32_t x: input) {
            unsignedInput.push_back(x < 0 ? (-(x << 1)) - 1 : (x << 1));
        }
    }

    inline void RiceEncoder::calculateOptimumRiceParam() {

    }

    inline void RiceEncoder::generateEncodedBits() {

    }

    inline void RiceEncoder::writeInts() {

    }

    data::RiceEncodedData RiceEncoder::process() {
        convertSignedToUnsigned();
        calculateOptimumRiceParam();
        generateEncodedBits();
        writeInts();
        return *(new data::RiceEncodedData(optimumRiceParam, input.size(), output));
    }
}