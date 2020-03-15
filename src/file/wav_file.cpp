#include "../include/file/wav_file.hpp"
#include "../include/data/exception.hpp"
#include <iostream>
#include <vector>

namespace file {
WavFile::WavFile(uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels, std::vector<data::WavFrame>&& wavFrames)
{
    //Calculate File Size
    size_t fileSize = 44;
    const size_t bytesPerSample = bitsPerSample / 8;
    for (data::WavFrame wavFrame : wavFrames) {
        fileSize += (wavFrame.samples.size() * wavFrame.samples[0].size() * bytesPerSample);
    }
    
    //Set chunk data
    wavChunk.chunkId = "RIFF";
    wavChunk.chunkSize = (uint32_t)(fileSize - 8);;
    wavChunk.format = "WAVE";

    //Set format subchunk data
    wavChunk.formatSubChunk.subChunkId = "fmt ";
    wavChunk.formatSubChunk.subChunkSize = 16;
    wavChunk.formatSubChunk.audioFormat = 1;
    wavChunk.formatSubChunk.numChannels = numChannels;
    wavChunk.formatSubChunk.sampleRate = sampleRate;
    wavChunk.formatSubChunk.byteRate = (sampleRate * numChannels * bitsPerSample) / 8;
    wavChunk.formatSubChunk.blockAlign = (numChannels * bitsPerSample) / 8;
    wavChunk.formatSubChunk.bitsPerSample = bitsPerSample;

    //Set dataSubChunk
    wavChunk.dataSubChunk.subChunkId = "data";
    wavChunk.dataSubChunk.subChunkSize = (uint32_t)(fileSize - 44);
    wavChunk.dataSubChunk.bitsPerSample = (uint8_t)bitsPerSample;
    wavChunk.dataSubChunk.channels = (uint8_t)numChannels;
    wavChunk.dataSubChunk.wavFrames = wavFrames;
}

void WavFile::readFromFile(std::ifstream& inputFile)
{
    std::vector<char> contents;
    size_t offset = 0;
    inputFile.seekg(0, std::ios::end);
    contents.resize(inputFile.tellg());
    inputFile.seekg(0, std::ios::beg);
    inputFile.read(&contents[0], contents.size());

    //Check File size
    if (contents.size() < 44) {
        const std::string exceptionMessage = "File is too small, probably not a wav file.";
        throw data::Exception(exceptionMessage);
    }

    //Read RIFF marker
    offset += 4;
    wavChunk.chunkId = std::string(contents.begin(), contents.begin() + offset);
    if (wavChunk.chunkId != "RIFF") {
        const std::string exceptionMessage = "chunkId is not RIFF, probably not a wav file.";
        throw data::Exception(exceptionMessage);
    }

    //Read chunkSize
    offset += 4;
    wavChunk.chunkSize = ((uint8_t)contents[7] << 24) | ((uint8_t)contents[6] << 16) | ((uint8_t)contents[5] << 8) | ((uint8_t)contents[4]);
    if ((size_t)wavChunk.chunkSize > contents.size()) {
        const std::string exceptionMessage = "chunkSize exceeds file size, probably a corrupted file";
        throw data::Exception(exceptionMessage);
    }

    //Read format
    offset += 4;
    wavChunk.format = std::string(contents.begin() + (offset - 4), contents.begin() + offset);
    if (wavChunk.format != "WAVE") {
        const std::string exceptionMessage = "format is not WAVE, probably not a wav file.";
        throw data::Exception(exceptionMessage);
    }

    bool isFmtSubChunkPresent = false;
    bool isDataSubChunkPresent = false;

    //Read subChunks
    while (offset < contents.size()) {
        //Validate subChunks
        uint8_t bitsPerSample = 0;
        uint8_t channels = 0;

        //Read subChunkId
        std::string subChunkId = std::string(contents.begin() + offset, contents.begin() + (offset + 4));
        offset += 4;
        if (subChunkId == "fmt ") {
            isFmtSubChunkPresent = true; //Mark fmt subchunk as present

            //Assign subChunkId
            data::WavFormatSubChunk wavFormatSubChunk;
            wavFormatSubChunk.subChunkId = subChunkId;

            //Read subChunkSize
            wavFormatSubChunk.subChunkSize = ((uint8_t)contents[offset + 3] << 24) | ((uint8_t)contents[offset + 2] << 16) | ((uint8_t)contents[offset + 1] << 8) | ((uint8_t)contents[offset]);
            offset += 4;

            //Read data
            wavFormatSubChunk.subChunkData = std::vector<int8_t>(contents.begin() + offset, contents.begin() + offset + wavFormatSubChunk.subChunkSize);
            offset += wavFormatSubChunk.subChunkSize;

            //Assign values
            wavFormatSubChunk.audioFormat = ((int8_t)wavFormatSubChunk.subChunkData[1] << 8) | ((int8_t)wavFormatSubChunk.subChunkData[0]);
            wavFormatSubChunk.numChannels = ((uint8_t)wavFormatSubChunk.subChunkData[3] << 8) | ((uint8_t)wavFormatSubChunk.subChunkData[2]);
            wavFormatSubChunk.sampleRate = ((uint8_t)wavFormatSubChunk.subChunkData[7] << 24) | ((uint8_t)wavFormatSubChunk.subChunkData[6] << 16) | ((uint8_t)wavFormatSubChunk.subChunkData[5] << 8) | ((uint8_t)wavFormatSubChunk.subChunkData[4]);
            wavFormatSubChunk.byteRate = ((uint8_t)wavFormatSubChunk.subChunkData[11] << 24) | ((uint8_t)wavFormatSubChunk.subChunkData[10] << 16) | ((uint8_t)wavFormatSubChunk.subChunkData[9] << 8) | ((uint8_t)wavFormatSubChunk.subChunkData[8]);
            wavFormatSubChunk.blockAlign = ((uint8_t)wavFormatSubChunk.subChunkData[13] << 8) | ((uint8_t)wavFormatSubChunk.subChunkData[12]);
            wavFormatSubChunk.bitsPerSample = ((uint8_t)wavFormatSubChunk.subChunkData[15] << 8) | ((uint8_t)wavFormatSubChunk.subChunkData[14]);

            if (wavFormatSubChunk.bitsPerSample != 16) {
                const std::string exceptionMessage = "Only 16bits per sample wav is supported.";
                throw data::Exception(exceptionMessage);
            }

            //Assign formatSubChunk
            wavChunk.formatSubChunk = wavFormatSubChunk;

            //Assign additional values which will be needed while processing data subChunk
            bitsPerSample = (uint8_t)wavFormatSubChunk.bitsPerSample;
            channels = (uint8_t)wavFormatSubChunk.numChannels;
            (void)bitsPerSample;
            (void)channels;
        } else if (subChunkId == "data") {
            if (!isFmtSubChunkPresent) {
                const std::string exceptionMessage = "Probably corrupt wav, data subChunk present without fmt subChunk.";
                throw data::Exception(exceptionMessage);
            }
            isDataSubChunkPresent = true; //Mark data subchunk as present

            //Assign subChunkId
            data::WavDataSubChunk wavDataSubChunk;
            wavDataSubChunk.bitsPerSample = bitsPerSample;
            wavDataSubChunk.channels = channels;
            wavDataSubChunk.subChunkId = subChunkId;

            //Read subChunkSize
            wavDataSubChunk.subChunkSize = ((uint8_t)contents[offset + 3] << 24) | ((uint8_t)contents[offset + 2] << 16) | ((uint8_t)contents[offset + 1] << 8) | ((uint8_t)contents[offset]);
            offset += 4;

            //Read data
            wavDataSubChunk.subChunkData = std::vector<int8_t>(contents.begin() + offset, contents.begin() + offset + wavDataSubChunk.subChunkSize);
            offset += wavDataSubChunk.subChunkSize;

            //Assign formatSubChunk
            wavChunk.dataSubChunk = wavDataSubChunk;
        } else {
            data::WavSubChunk wavSubChunk;

            //Assign subChunkId
            wavSubChunk.subChunkId = subChunkId;

            //Read subChunkSize
            wavSubChunk.subChunkSize = ((uint8_t)contents[offset + 3] << 24) | ((uint8_t)contents[offset + 2] << 16) | ((uint8_t)contents[offset + 1] << 8) | ((uint8_t)contents[offset]);
            offset += 4;

            //Read data
            wavSubChunk.subChunkData = std::vector<int8_t>(contents.begin() + offset, contents.begin() + offset + wavSubChunk.subChunkSize);
            offset += wavSubChunk.subChunkSize;

            wavChunk.wavSubChunks.push_back(wavSubChunk);
        }
    }

    //Validate subChunks
    if (!isFmtSubChunkPresent) {
        const std::string exceptionMessage = "fmt subChunk is missing from file";
        throw data::Exception(exceptionMessage);
    }
    if (!isDataSubChunkPresent) {
        const std::string exceptionMessage = "data subChunk is missing from file";
        throw data::Exception(exceptionMessage);
    }

    //Demux Samples
    demuxSamples();
}

void WavFile::demuxSamples()
{
    size_t sampleCount = (wavChunk.dataSubChunk.subChunkData.size() * 8) / wavChunk.formatSubChunk.bitsPerSample;
    wavChunk.dataSubChunk.wavFrames.reserve((size_t)(sampleCount / (samplesPerChannelPerFrame * wavChunk.formatSubChunk.numChannels)));
    size_t offset = 0;

    std::vector<std::vector<int32_t>> demuxedIntSamples;
    demuxedIntSamples.reserve((size_t)wavChunk.formatSubChunk.numChannels);
    for (size_t i = 0; i < (size_t)wavChunk.formatSubChunk.numChannels; i++) {
        demuxedIntSamples.push_back(std::vector<int32_t>());
        demuxedIntSamples[i].reserve((size_t)(sampleCount / wavChunk.formatSubChunk.numChannels));
    }

    for (size_t i = 0; i < sampleCount;) {
        for (size_t j = 0; j < (size_t)wavChunk.formatSubChunk.numChannels; j++) {
            demuxedIntSamples[j].push_back(((int8_t)wavChunk.dataSubChunk.subChunkData[offset + 1] << 8) | ((int8_t)wavChunk.dataSubChunk.subChunkData[offset]));
            offset += 2;
            i++;
        }
    }

    offset = 0;
    for (size_t i = 0; i < wavChunk.dataSubChunk.wavFrames.capacity(); i++) {
        std::vector<std::vector<int32_t>> allSamples;
        allSamples.reserve((size_t)wavChunk.formatSubChunk.numChannels);

        for (size_t j = 0; j < (size_t)wavChunk.formatSubChunk.numChannels; j++) {
            //Assign buffer
            std::vector<int32_t> samples;
            samples.reserve(samplesPerChannelPerFrame);

            samples.assign((demuxedIntSamples[j].begin() + offset), (demuxedIntSamples[j].begin() + offset + samplesPerChannelPerFrame));
            allSamples.push_back(samples);
        }
        offset += samplesPerChannelPerFrame;

        data::WavFrame wavFrame = data::WavFrame((uint8_t)wavChunk.formatSubChunk.bitsPerSample, std::move(allSamples));
        wavChunk.dataSubChunk.wavFrames.push_back(wavFrame);
    }
}

void WavFile::writeToFile(std::ofstream& outputFile)
{
    const size_t bytesPerSample = wavChunk.formatSubChunk.bitsPerSample / 8;

    //Write chunk
    outputFile << wavChunk.chunkId;
    outputFile.write(reinterpret_cast<const char*>(&wavChunk.chunkSize), sizeof(wavChunk.chunkSize));
    outputFile << wavChunk.format;

    //Write format subchunk
    outputFile << wavChunk.formatSubChunk.subChunkId;
    outputFile.write(reinterpret_cast<const char*>(&wavChunk.formatSubChunk.subChunkSize), sizeof(wavChunk.formatSubChunk.subChunkSize));
    outputFile.write(reinterpret_cast<const char*>(&wavChunk.formatSubChunk.audioFormat), sizeof(wavChunk.formatSubChunk.audioFormat));
    outputFile.write(reinterpret_cast<const char*>(&wavChunk.formatSubChunk.numChannels), sizeof(wavChunk.formatSubChunk.numChannels));
    outputFile.write(reinterpret_cast<const char*>(&wavChunk.formatSubChunk.sampleRate), sizeof(wavChunk.formatSubChunk.sampleRate));
    outputFile.write(reinterpret_cast<const char*>(&wavChunk.formatSubChunk.byteRate), sizeof(wavChunk.formatSubChunk.byteRate));
    outputFile.write(reinterpret_cast<const char*>(&wavChunk.formatSubChunk.blockAlign), sizeof(wavChunk.formatSubChunk.blockAlign));
    outputFile.write(reinterpret_cast<const char*>(&wavChunk.formatSubChunk.bitsPerSample), sizeof(wavChunk.formatSubChunk.bitsPerSample));

    //Write data subChunk
    outputFile << wavChunk.dataSubChunk.subChunkId;
    outputFile.write(reinterpret_cast<const char*>(&wavChunk.dataSubChunk.subChunkSize), sizeof(wavChunk.dataSubChunk.subChunkSize));
    if (wavChunk.formatSubChunk.numChannels == 2 && wavChunk.formatSubChunk.bitsPerSample == 16) { //Faster version of 16bit and 2channel files
        int16_t* samples = new int16_t[10000];
        for (data::WavFrame wavFrame : wavChunk.dataSubChunk.wavFrames) {
            size_t offset = 0;
            const size_t bufferSize = wavFrame.samples[0].size() * wavFrame.samples.size() * bytesPerSample;
            for (size_t i = 0; i < wavFrame.samples[0].size(); i++) {
                samples[offset] = (uint16_t)wavFrame.samples[0][i];
                offset++;
                samples[offset] = (uint16_t)wavFrame.samples[1][i];
                offset++;
            }
            outputFile.write(reinterpret_cast<const char*>(samples), (bufferSize));
        }
        delete[] samples;
    } else { //Slow as molasses version
        for (data::WavFrame wavFrame : wavChunk.dataSubChunk.wavFrames) {
            for (size_t i = 0; i < wavFrame.samples[0].size(); i++) {
                for (std::vector<int32_t> samples : wavFrame.samples) {
                    outputFile.write(reinterpret_cast<const char*>(&samples[i]), bytesPerSample);
                }
            }
        }
    }
}
}