#include <iostream>
#include "../include/rice.hpp"

namespace rice
{
    RiceDecoder::RiceDecoder(data::RiceEncodedData encodedData) {
        input = encodedData.encodedData;
        dataCount = encodedData.dataCount;
        optimumRiceParam = encodedData.optimumRiceParam;
        unsignedOutput = std::vector<uint64_t>(dataCount);
        output = std::vector<int32_t>(dataCount);
    }

    inline void RiceDecoder::generateEncodedBits() {

    }

    inline void RiceDecoder::generateDecodedUnsignedInts() {

    }

    inline void RiceDecoder::convertUnsignedToSigned() {

    }

    data::RiceDecodedData RiceDecoder::process() {
        generateEncodedBits();
        generateDecodedUnsignedInts();
        convertUnsignedToSigned();
        return *(new data::RiceDecodedData(output));
    }
}
