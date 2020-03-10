#ifndef _SELA_SUB_FRAME_H_
#define _SELA_SUB_FRAME_H_

#include <cstdint>

#include "rice_encoded_data.hpp"

namespace data {
class SelaSubFrame {
public:
    // Audio Channel number - 1, 2, etc
    uint8_t channel;
    // 0 - Subframe is independent, 1 - SubFrame uses difference coding and is
    // dependent on another channel
    uint8_t subFrameType;
    // Incase subFrame uses difference coding, the channel from which difference is
    // generated will be stored here
    uint8_t parentChannelNumber;

    // Reflection coefficient data
    uint8_t reflectionCoefficientRiceParam;
    uint16_t reflectionCoefficientRequiredInts;
    uint8_t optimumLpcOrder;
    const std::vector<uint32_t> encodedReflectionCoefficients;

    // Residue data
    uint8_t residueRiceParam;
    uint16_t residueRequiredInts;
    uint16_t samplesPerChannel;
    const std::vector<uint32_t> encodedResidues;

    uint32_t getByteCount();
    void write();
    SelaSubFrame(uint8_t channel, uint8_t subFrameType, uint8_t parentChannelNumber,
        const data::RiceEncodedData& reflectionData, const data::RiceEncodedData& residueData)
        : channel(channel)
        , subFrameType(subFrameType)
        , parentChannelNumber(parentChannelNumber)
        , reflectionCoefficientRiceParam((uint8_t)reflectionData.optimumRiceParam)
        , reflectionCoefficientRequiredInts((uint16_t)reflectionData.encodedData.size())
        , optimumLpcOrder((uint8_t)reflectionData.dataCount)
        , encodedReflectionCoefficients(std::move(reflectionData.encodedData))
        , residueRiceParam((uint8_t)residueData.optimumRiceParam)
        , residueRequiredInts((uint16_t)residueData.encodedData.size())
        , samplesPerChannel((uint16_t)residueData.dataCount)
        , encodedResidues(std::move(residueData.encodedData))
    {
    }
};
}

#endif