#include "../include/frame.hpp"
#include "../include/lpc.hpp"
#include "../include/rice.hpp"

#include <vector>

namespace frame {
FrameEncoder::FrameEncoder(const data::WavFrame& wavFrame)
    : wavFrame(wavFrame)
{
}

data::SelaFrame FrameEncoder::process()
{
    data::SelaFrame selaFrame = data::SelaFrame(wavFrame.bitsPerSample);
    selaFrame.subFrames.reserve(wavFrame.samples.size());

    // Foreach channel
    for (size_t i = 0; i < wavFrame.samples.size(); i++) {
        if (i == 1 && ((i + 1) == wavFrame.samples.size())) {
            // Stage 1 - Generate difference signal
            std::vector<int32_t> differenceSignal;
            differenceSignal.reserve(wavFrame.samples[i]->size());
            for (size_t j = 0; j < wavFrame.samples[i]->size(); j++) {
                differenceSignal.push_back(wavFrame.samples[i - 1]->at(j) - wavFrame.samples[i]->at(j));
            }

            // Stage 2 - Generate residues and reflection coefficients for difference as
            // well as actual signal
            lpc::ResidueGenerator residueGeneratorDifference = lpc::ResidueGenerator(data::LpcDecodedData(wavFrame.bitsPerSample, std::move(differenceSignal)));
            data::LpcEncodedData residuesDifference = residueGeneratorDifference.process();

            lpc::ResidueGenerator residueGeneratorActual = lpc::ResidueGenerator(data::LpcDecodedData(wavFrame.bitsPerSample, std::move(*wavFrame.samples[i])));
            data::LpcEncodedData residuesActual = residueGeneratorActual.process();

            // Stage 3 - Compress residues and reflection coefficients for difference and
            // actual signal
            rice::RiceEncoder reflectionRiceEncoderDifference = rice::RiceEncoder(data::RiceDecodedData(std::move(residuesDifference.quantizedReflectionCoefficients)));
            rice::RiceEncoder residueRiceEncoderDifference = rice::RiceEncoder(data::RiceDecodedData(std::move(residuesDifference.residues)));
            data::RiceEncodedData reflectionDataDifference = reflectionRiceEncoderDifference.process();
            data::RiceEncodedData residueDataDifference = residueRiceEncoderDifference.process();

            rice::RiceEncoder reflectionRiceEncoderActual = rice::RiceEncoder(data::RiceDecodedData(std::move(residuesActual.quantizedReflectionCoefficients)));
            rice::RiceEncoder residueRiceEncoderActual = rice::RiceEncoder(data::RiceDecodedData(std::move(residuesActual.residues)));
            data::RiceEncodedData reflectionDataActual = reflectionRiceEncoderActual.process();
            data::RiceEncodedData residueDataActual = residueRiceEncoderActual.process();

            // Stage 4 - Compare sizes of both types and generate subFrame
            size_t differenceDataSize = reflectionDataDifference.encodedData.size() + residueDataDifference.encodedData.size();
            size_t actualDataSize = reflectionDataActual.encodedData.size() + residueDataActual.encodedData.size();
            if (differenceDataSize < actualDataSize) {
                data::SelaSubFrame selaSubFrame = data::SelaSubFrame((uint8_t)i, (uint8_t)1, (uint8_t)(i - 1), reflectionDataDifference, residueDataDifference);
                selaFrame.subFrames.push_back(selaSubFrame);
            } else {
                data::SelaSubFrame selaSubFrame = data::SelaSubFrame((uint8_t)i, (uint8_t)0, (uint8_t)i, reflectionDataActual, residueDataActual);
                selaFrame.subFrames.push_back(selaSubFrame);
            }
        } else {
            // Stage 1 - Generate residues and reflection coefficients
            lpc::ResidueGenerator residueGenerator = lpc::ResidueGenerator(data::LpcDecodedData(wavFrame.bitsPerSample, std::move(*wavFrame.samples[i])));
            data::LpcEncodedData residues = residueGenerator.process();

            // Stage 2 - Compress residues and reflection coefficients
            rice::RiceEncoder reflectionRiceEncoder = rice::RiceEncoder(data::RiceDecodedData(std::move(residues.quantizedReflectionCoefficients)));
            rice::RiceEncoder residueRiceEncoder = rice::RiceEncoder(data::RiceDecodedData(std::move(residues.residues)));
            data::RiceEncodedData reflectionData = reflectionRiceEncoder.process();
            data::RiceEncodedData residueData = residueRiceEncoder.process();

            // Stage 3 - Generate Subframes
            data::SelaSubFrame selaSubFrame = data::SelaSubFrame((uint8_t)i, (uint8_t)0, (uint8_t)i, reflectionData, residueData);
            selaFrame.subFrames.push_back(selaSubFrame);
        }
    }

    return selaFrame;
}

}