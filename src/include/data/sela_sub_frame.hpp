#ifndef _SELA_SUB_FRAME_H_
#define _SELA_SUB_FRAME_H_

#include <cstdint>

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
    std::vector<uint32_t> encodedReflectionCoefficients;

    // Residue data
    uint8_t residueRiceParam;
    uint16_t residueRequiredInts;
    uint16_t samplesPerChannel;
    std::vector<uint32_t> encodedResidues;

    uint32_t getByteCount();
    void write();
};
}

#endif