#include "../include/rice.hpp"

namespace rice {
RiceDecoder::RiceDecoder(const data::RiceEncodedData& encodedData)
    : input(encodedData.encodedData)
    , dataCount(encodedData.dataCount)
    , optimumRiceParam(encodedData.optimumRiceParam)
{
}

inline void RiceDecoder::generateEncodedBits()
{
    bitInput.reserve(input.size() * 32);
    for (uint64_t x : input) {
        for (uint64_t j = 0; j < 32; j++) {
            bitInput.push_back((bool)(x >> j & 0x1));
        }
    }
}

inline void RiceDecoder::generateDecodedUnsignedInts()
{
    uint32_t count = 0;
    uint32_t temp = 0;
    uint32_t i = 0;
    uint32_t bitReadCounter = 0;
    unsignedOutput.reserve(dataCount);
    while (count < dataCount) {
        // Count 1s until a zero is encountered
        temp = 0;
        while (bitInput[bitReadCounter] == 1) {
            temp++;
            bitReadCounter++;
        }
        bitReadCounter++;
        unsignedOutput.push_back(temp << optimumRiceParam);
        // Read the last 'optimumRiceParam' number of bits and add them to output
        for (i = 1; i < (optimumRiceParam + 1); i++) {
            unsignedOutput[count] = unsignedOutput[count] | ((long)bitInput[bitReadCounter] << (optimumRiceParam - i));
            bitReadCounter++;
        }
        count++;
    }
}

inline void RiceDecoder::convertUnsignedToSigned(std::vector<int32_t>& output)
{
    output.reserve(dataCount);
    for (uint64_t x : unsignedOutput) {
        output.push_back(
            (int32_t)(((x & 0x01) == 0x01) ? -(int64_t)((x + 1) >> 1) : (x >> 1)));
    }
}

data::RiceDecodedData RiceDecoder::process()
{
    std::vector<int32_t> output;
    generateEncodedBits();
    generateDecodedUnsignedInts();
    convertUnsignedToSigned(output);
    return data::RiceDecodedData(std::move(output));
}
}
