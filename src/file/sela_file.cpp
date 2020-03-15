#include "../include/file/sela_file.hpp"
#include "../include/data/exception.hpp"
#include "../include/data/rice_encoded_data.hpp"
#include "../include/data/sela_frame.hpp"
#include "../include/data/sela_sub_frame.hpp"

#include <iostream>

namespace file {
SelaFile::SelaFile(uint32_t sampleRate, uint16_t bitsPerSample, uint8_t channels, std::vector<data::SelaFrame>&& selaFrames)
    : selaFrames(std::move(selaFrames))
{
    selaHeader.sampleRate = sampleRate;
    selaHeader.bitsPerSample = bitsPerSample;
    selaHeader.channels = channels;
    selaHeader.numFrames = (uint32_t)selaFrames.size();
}

void SelaFile::readFromFile(std::ifstream& inputFile)
{
    std::vector<char> contents;
    size_t offset = 0;
    inputFile.seekg(0, std::ios::end);
    contents.resize(inputFile.tellg());
    inputFile.seekg(0, std::ios::beg);
    inputFile.read(&contents[0], contents.size());

    //Check File size
    if (contents.size() < 15) {
        const std::string exceptionMessage = "File is too small, probably not a sela file.";
        throw data::Exception(exceptionMessage);
    }

    //Read Magic Number
    offset += 4;
    std::string header = std::string(contents.begin(), contents.begin() + offset);
    if (header != "SeLa") {
        const std::string exceptionMessage = "Magic number is incorrect, probably not a sela file.";
        throw data::Exception(exceptionMessage);
    }

    //Read other details
    offset += 11;
    selaHeader.sampleRate = ((uint8_t)contents[7] << 24) | ((uint8_t)contents[6] << 16) | ((uint8_t)contents[5] << 8) | ((uint8_t)contents[4]);
    selaHeader.bitsPerSample = ((uint8_t)contents[9] << 8) | ((uint8_t)contents[8]);
    selaHeader.channels = (uint8_t)contents[10];
    selaHeader.numFrames = ((uint8_t)contents[14] << 24) | ((uint8_t)contents[13] << 16) | ((uint8_t)contents[12] << 8) | ((uint8_t)contents[11]);

    //Read frames
    selaFrames.reserve(selaHeader.numFrames);
    for (size_t i = 0; i < (size_t)selaHeader.numFrames; i++) {
        //Read SyncWord
        uint32_t sync = ((uint8_t)contents[offset + 3] << 24) | ((uint8_t)contents[offset + 2] << 16) | ((uint8_t)contents[offset + 1] << 8) | ((uint8_t)contents[offset]);
        if (sync != 0xAA55FF00) {
            break;
        }
        offset += 4;

        data::SelaFrame frame = data::SelaFrame((uint8_t)selaHeader.bitsPerSample);
        frame.subFrames.reserve(selaHeader.channels);

        //Read subFrames
        for (size_t j = 0; j < (size_t)selaHeader.channels; j++) {
            // Get channel info
            uint8_t subFrameChannel = (uint8_t)contents[offset];
            uint8_t subFrameType = (uint8_t)contents[offset + 1];
            uint8_t parentChannelNumber = (uint8_t)contents[offset + 2];
            offset += 3;

            // Get Reflection data
            uint8_t reflectionCoefficientRiceParam = (uint8_t)contents[offset];
            uint16_t reflectionCoefficientRequiredInts = ((uint8_t)contents[offset + 2] << 8) | ((uint8_t)contents[offset + 1]);
            uint8_t optimumLpcOrder = (uint8_t)contents[offset + 3];
            std::vector<uint32_t> encodedReflectionCoefficients;
            encodedReflectionCoefficients.reserve((size_t)reflectionCoefficientRequiredInts);
            offset += 4;
            for (size_t k = 0; k < encodedReflectionCoefficients.capacity(); k++) {
                encodedReflectionCoefficients.push_back(((uint8_t)contents[offset + 3] << 24) | ((uint8_t)contents[offset + 2] << 16) | ((uint8_t)contents[offset + 1] << 8) | ((uint8_t)contents[offset]));
                offset += 4;
            }

            //Get Residue Data
            uint8_t residueRiceParam = (uint8_t)contents[offset];
            uint16_t residueRequiredInts = ((uint8_t)contents[offset + 2] << 8) | ((uint8_t)contents[offset + 1]);
            uint16_t samplesPerChannel = ((uint8_t)contents[offset + 4] << 8) | ((uint8_t)contents[offset + 3]);
            std::vector<uint32_t> encodedResidues;
            encodedResidues.reserve(residueRequiredInts);
            offset += 5;
            for (size_t k = 0; k < encodedResidues.capacity(); k++) {
                encodedResidues.push_back(((uint8_t)contents[offset + 3] << 24) | ((uint8_t)contents[offset + 2] << 16) | ((uint8_t)contents[offset + 1] << 8) | ((uint8_t)contents[offset]));
                offset += 4;
            }

            //Generate subFrames
            data::RiceEncodedData reflectionData = data::RiceEncodedData(reflectionCoefficientRiceParam, optimumLpcOrder, std::move(encodedReflectionCoefficients));
            data::RiceEncodedData residueData = data::RiceEncodedData(residueRiceParam, samplesPerChannel, std::move(encodedResidues));
            data::SelaSubFrame subFrame = data::SelaSubFrame(subFrameChannel, subFrameType, parentChannelNumber, reflectionData, residueData);
            frame.subFrames.push_back(subFrame);
        }

        selaFrames.push_back(frame);
    }
}

void SelaFile::writeToFile(std::ofstream& outputFile)
{
    //Write Header
    outputFile.write(reinterpret_cast<const char*>(selaHeader.magicNumber), (size_t)4);
    outputFile.write(reinterpret_cast<const char*>(&selaHeader.sampleRate), sizeof(selaHeader.sampleRate));
    outputFile.write(reinterpret_cast<const char*>(&selaHeader.bitsPerSample), sizeof(selaHeader.bitsPerSample));
    outputFile.write(reinterpret_cast<const char*>(&selaHeader.channels), sizeof(selaHeader.channels));
    outputFile.write(reinterpret_cast<const char*>(&selaHeader.numFrames), sizeof(selaHeader.numFrames));

    //Write frames
    for (data::SelaFrame selaFrame : selaFrames) {
        //Write Syncword
        outputFile.write(reinterpret_cast<const char*>(&selaFrame.syncWord), sizeof(selaFrame.syncWord));

        //Write subFrames
        for (data::SelaSubFrame subFrame : selaFrame.subFrames) {
            //Get byte count
            outputFile.write(reinterpret_cast<const char*>(&subFrame.channel), sizeof(subFrame.channel));
            outputFile.write(reinterpret_cast<const char*>(&subFrame.subFrameType), sizeof(subFrame.subFrameType));
            outputFile.write(reinterpret_cast<const char*>(&subFrame.parentChannelNumber), sizeof(subFrame.parentChannelNumber));

            outputFile.write(reinterpret_cast<const char*>(&subFrame.reflectionCoefficientRiceParam), sizeof(subFrame.reflectionCoefficientRiceParam));
            outputFile.write(reinterpret_cast<const char*>(&subFrame.reflectionCoefficientRequiredInts), sizeof(subFrame.reflectionCoefficientRequiredInts));
            outputFile.write(reinterpret_cast<const char*>(&subFrame.optimumLpcOrder), sizeof(subFrame.optimumLpcOrder));
            outputFile.write(reinterpret_cast<const char*>(&subFrame.encodedReflectionCoefficients[0]), subFrame.encodedReflectionCoefficients.size() * sizeof(uint32_t));

            outputFile.write(reinterpret_cast<const char*>(&subFrame.residueRiceParam), sizeof(subFrame.residueRiceParam));
            outputFile.write(reinterpret_cast<const char*>(&subFrame.residueRequiredInts), sizeof(subFrame.residueRequiredInts));
            outputFile.write(reinterpret_cast<const char*>(&subFrame.samplesPerChannel), sizeof(subFrame.samplesPerChannel));
            outputFile.write(reinterpret_cast<const char*>(&subFrame.encodedResidues[0]), subFrame.encodedResidues.size() * sizeof(uint32_t));
        }
    }
}
}
