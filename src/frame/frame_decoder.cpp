#include "../include/frame.hpp"
#include "../include/lpc.hpp"
#include "../include/rice.hpp"

namespace frame {
FrameDecoder::FrameDecoder(const data::SelaFrame& selaFrame)
    : selaFrame(selaFrame)
{
}

data::WavFrame FrameDecoder::process()
{
    std::vector<std::vector<int32_t>> allSamples = std::vector<std::vector<int32_t>>(selaFrame.subFrames.size(), std::vector<int32_t>());
    allSamples.reserve(selaFrame.subFrames.size());

    // Foreach independent subFrame
    for (data::SelaSubFrame selaSubFrame : selaFrame.subFrames) {
        if (selaSubFrame.subFrameType == 0) {
            // Stage 1 - Extract data from subFrame
            uint8_t channel = selaSubFrame.channel;
            uint8_t optimumLpcOrder = selaSubFrame.optimumLpcOrder;
            const data::RiceEncodedData reflectionData = data::RiceEncodedData(selaSubFrame.reflectionCoefficientRiceParam,
                selaSubFrame.optimumLpcOrder, std::move(selaSubFrame.encodedReflectionCoefficients));
            const data::RiceEncodedData residueData = data::RiceEncodedData(selaSubFrame.residueRiceParam,
                selaSubFrame.samplesPerChannel, std::move(selaSubFrame.encodedResidues));

            // Stage 2 - Decompress data
            data::RiceDecodedData decodedReflectionData = rice::RiceDecoder(reflectionData).process();
            data::RiceDecodedData decodedResidueData = rice::RiceDecoder(residueData).process();

            // Stage 3 - Generate Samples
            const data::LpcEncodedData encodedData = data::LpcEncodedData(optimumLpcOrder, selaFrame.bitsPerSample,
                std::move(decodedReflectionData.decodedData), std::move(decodedResidueData.decodedData));
            const data::LpcDecodedData decoded = lpc::SampleGenerator(encodedData).process();
            allSamples[channel] = decoded.samples;
        }
    }

    // Foreach dependent subFrame
    for (data::SelaSubFrame subFrame : selaFrame.subFrames) {
        if (subFrame.subFrameType == 1) {
            // Stage 1 - Extract data from subFrame
            uint8_t channel = subFrame.channel;
            uint8_t parentChannelNumber = subFrame.parentChannelNumber;
            uint8_t optimumLpcOrder = subFrame.optimumLpcOrder;
            const data::RiceEncodedData reflectionData = data::RiceEncodedData(subFrame.reflectionCoefficientRiceParam,
                subFrame.optimumLpcOrder, std::move(subFrame.encodedReflectionCoefficients));
            const data::RiceEncodedData residueData = data::RiceEncodedData(subFrame.residueRiceParam,
                subFrame.samplesPerChannel, std::move(subFrame.encodedResidues));

            // Stage 2 - Decompress data
            data::RiceDecodedData decodedReflectionData = rice::RiceDecoder(reflectionData).process();
            data::RiceDecodedData decodedResidueData = rice::RiceDecoder(residueData).process();

            // Stage 3 - Generate Difference signal
            const data::LpcEncodedData encodedData = data::LpcEncodedData(optimumLpcOrder, selaFrame.bitsPerSample,
                std::move(decodedReflectionData.decodedData), std::move(decodedResidueData.decodedData));
            data::LpcDecodedData difference = lpc::SampleGenerator(encodedData).process();

            std::vector<int32_t> decoded;
            decoded.reserve(difference.samples.size());

            // Stage 4 Generate samples
            for (size_t i = 0; i < decoded.capacity(); i++) {
                decoded.push_back(allSamples[parentChannelNumber][i] - difference.samples[i]);
            }
            allSamples[channel] = decoded;
        }
    }

    return data::WavFrame(selaFrame.bitsPerSample, std::move(allSamples));
}
}