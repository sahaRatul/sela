#include "../include/frame.hpp"
#include "../include/lpc.hpp"
#include "../include/rice.hpp"

#include <vector>
#include <list>

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
            //-------------------------------------Stage 1 - Generate difference signal----------------------------------------------------
            std::vector<int32_t> differenceSignal;
            differenceSignal.reserve(wavFrame.samples[i].size());
            for (size_t j = 0; j < wavFrame.samples[i].size(); j++) {
                differenceSignal.push_back(wavFrame.samples[i - 1][j] - wavFrame.samples[i][j]);
            }
            //-----------------------------------------------------------------------------------------------------------------------------

            //----------- Stage 2 - Generate residues and reflection coefficients for difference as well as actual signal------------------
            data::LpcDecodedData residueGeneratorDifferenceData = data::LpcDecodedData(wavFrame.bitsPerSample, std::move(differenceSignal));
            lpc::ResidueGenerator residueGeneratorDifference = lpc::ResidueGenerator(residueGeneratorDifferenceData);
            data::LpcEncodedData residuesDifference = residueGeneratorDifference.process();

            data::LpcDecodedData residueGeneratorActualData = data::LpcDecodedData(wavFrame.bitsPerSample, std::move(wavFrame.samples[i]));
            lpc::ResidueGenerator residueGeneratorActual = lpc::ResidueGenerator(residueGeneratorActualData);
            data::LpcEncodedData residuesActual = residueGeneratorActual.process();
            //-----------------------------------------------------------------------------------------------------------------------------

            //-----------------Stage 3 - Compress residues and reflection coefficients for difference and actual signal--------------------
            //Assign data
            data::RiceDecodedData reflectionRiceEncoderDifferenceData = data::RiceDecodedData(std::move(residuesDifference.quantizedReflectionCoefficients));
            data::RiceDecodedData residueRiceEncoderDifferenceData = data::RiceDecodedData(std::move(residuesDifference.residues));
            
            //Create Encoders
            rice::RiceEncoder reflectionRiceEncoderDifference = rice::RiceEncoder(reflectionRiceEncoderDifferenceData);
            rice::RiceEncoder residueRiceEncoderDifference = rice::RiceEncoder(residueRiceEncoderDifferenceData);

            //Process data in encoders
            data::RiceEncodedData reflectionDataDifference = reflectionRiceEncoderDifference.process();
            data::RiceEncodedData residueDataDifference = residueRiceEncoderDifference.process();

            //Assign data
            data::RiceDecodedData reflectionRiceEncoderActualData = data::RiceDecodedData(std::move(residuesActual.quantizedReflectionCoefficients));
            data::RiceDecodedData residueRiceEncoderActualData = data::RiceDecodedData(std::move(residuesActual.residues));
            
            //Create Encoders
            rice::RiceEncoder reflectionRiceEncoderActual = rice::RiceEncoder(reflectionRiceEncoderActualData);
            rice::RiceEncoder residueRiceEncoderActual = rice::RiceEncoder(residueRiceEncoderActualData);

            //Process data in encoders
            data::RiceEncodedData reflectionDataActual = reflectionRiceEncoderActual.process();
            data::RiceEncodedData residueDataActual = residueRiceEncoderActual.process();
            //-----------------------------------------------------------------------------------------------------------------------------

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
            //---------------------Stage 1 - Generate residues and reflection coefficients----------------------
            data::LpcDecodedData samples = data::LpcDecodedData(wavFrame.bitsPerSample, std::move(wavFrame.samples[i]));
            lpc::ResidueGenerator residueGenerator = lpc::ResidueGenerator(samples);
            data::LpcEncodedData residues = residueGenerator.process();
            //--------------------------------------------------------------------------------------------------

            //---------------------Stage 2 - Compress residues and reflection coefficients----------------------
            //Assign data
            data::RiceDecodedData reflectionRiceInputData = data::RiceDecodedData(std::move(residues.quantizedReflectionCoefficients));
            data::RiceDecodedData residueRiceInputData = data::RiceDecodedData(std::move(residues.residues));
            
            //Create Encoders
            rice::RiceEncoder reflectionRiceEncoder = rice::RiceEncoder(reflectionRiceInputData);
            rice::RiceEncoder residueRiceEncoder = rice::RiceEncoder(residueRiceInputData);

            //Process data in encoders
            data::RiceEncodedData reflectionRiceOutputData = reflectionRiceEncoder.process();
            data::RiceEncodedData residueRiceOutputData = residueRiceEncoder.process();
            //-------------------------------------------------------------------------------------------------

            //-----------------------------------Stage 3 - Generate Subframes----------------------------------
            data::SelaSubFrame selaSubFrame = data::SelaSubFrame((uint8_t)i, (uint8_t)0, (uint8_t)i, reflectionRiceOutputData, residueRiceOutputData);
            selaFrame.subFrames.push_back(selaSubFrame);
            //-------------------------------------------------------------------------------------------------
        }
    }

    return selaFrame;
}

}