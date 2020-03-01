#include "../include/frame.hpp"
#include "../include/lpc.hpp"
#include "../include/rice.hpp"

#include <vector>

namespace frame {
FrameDecoder::FrameDecoder(data::SelaFrame& selaFrame)
    : selaFrame(selaFrame)
{
}

data::WavFrame FrameDecoder::process()
{
    std::vector<std::vector<int32_t>*> samples;
    samples.reserve(selaFrame.subFrames.size());

    // Foreach independent subFrame
    for (data::SelaSubFrame selaSubFrame : selaFrame.subFrames) {
        if (selaSubFrame.subFrameType == 0) {
            // Stage 1 - Extract data from subFrame
            uint8_t channel = selaSubFrame.channel;
            uint8_t optimumLpcOrder = selaSubFrame.optimumLpcOrder;
            data::RiceEncodedData* reflectionData = new data::RiceEncodedData(selaSubFrame.reflectionCoefficientRiceParam,
                selaSubFrame.optimumLpcOrder, selaSubFrame.encodedReflectionCoefficients);
            data::RiceEncodedData* residueData = new data::RiceEncodedData(selaSubFrame.residueRiceParam,
                selaSubFrame.samplesPerChannel, selaSubFrame.encodedResidues);

            // Stage 2 - Decompress data
            data::RiceDecodedData decodedReflectionData = (new rice::RiceDecoder(*reflectionData))->process();
            data::RiceDecodedData decodedResidueData = (new rice::RiceDecoder(*residueData))->process();

            // Stage 3 - Generate Samples
            data::LpcEncodedData* encodedData = new data::LpcEncodedData(optimumLpcOrder, selaFrame.bitsPerSample,
                decodedReflectionData.decodedData, decodedResidueData.decodedData);
            data::LpcDecodedData decoded = (new lpc::SampleGenerator(*encodedData))->process();
            samples[channel] = &decoded.samples;
        }
    }

    // Foreach dependent subFrame
    for (data::SelaSubFrame subFrame : selaFrame.subFrames) {
        if (subFrame.subFrameType == 1) {
            // Stage 1 - Extract data from subFrame
            uint8_t channel = subFrame.channel;
            uint8_t parentChannelNumber = subFrame.parentChannelNumber;
            uint8_t optimumLpcOrder = subFrame.optimumLpcOrder;
            data::RiceEncodedData* reflectionData = new data::RiceEncodedData(subFrame.reflectionCoefficientRiceParam,
                subFrame.optimumLpcOrder, subFrame.encodedReflectionCoefficients);
            data::RiceEncodedData* residueData = new data::RiceEncodedData(subFrame.residueRiceParam,
                subFrame.samplesPerChannel, subFrame.encodedResidues);

            // Stage 2 - Decompress data
            data::RiceDecodedData decodedReflectionData = (new rice::RiceDecoder(*reflectionData))->process();
            data::RiceDecodedData decodedResidueData = (new rice::RiceDecoder(*residueData))->process();

            // Stage 3 - Generate Difference signal
            data::LpcEncodedData* encodedData = new data::LpcEncodedData(optimumLpcOrder, selaFrame.bitsPerSample,
                decodedReflectionData.decodedData, decodedResidueData.decodedData);
            data::LpcDecodedData difference = (new lpc::SampleGenerator(*encodedData))->process();

            std::vector<int32_t> decoded;
            decoded.reserve(difference.samples.size());

            // Stage 4 Generate samples
            for (int i = 0; i < decoded.capacity(); i++) {
                decoded[i] = (*(samples[parentChannelNumber]))[i] - difference.samples[i];
            }
            samples[channel] = &decoded;
        }
    }

    return data::WavFrame(selaFrame.bitsPerSample, samples);
}
}