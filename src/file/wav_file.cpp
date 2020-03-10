#include "../include/file/wav_file.hpp"
#include "../include/data/exception.hpp"
#include <iostream>
#include <vector>

namespace file {
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
        throw data::Exception("File is too small, probably not a wav file.");
    }

    //Read RIFF marker
    offset += 4;
    wavChunk.chunkId = std::string(contents.begin(), contents.begin() + offset);
    if (wavChunk.chunkId != "RIFF") {
        throw data::Exception("chunkId is not RIFF, probably not a wav file.");
    }

    //Read chunkSize
    offset += 4;
    wavChunk.chunkSize = ((uint8_t)contents[7] << 24) | ((uint8_t)contents[6] << 16) | ((uint8_t)contents[5] << 8) | ((uint8_t)contents[4]);
    if ((size_t)wavChunk.chunkSize > contents.size()) {
        throw data::Exception("chunkSize exceeds file size, probably a corrupted file");
    }

    //Read format
    offset += 4;
    wavChunk.format = std::string(contents.begin() + (offset - 4), contents.begin() + offset);
    if (wavChunk.format != "WAVE") {
        throw data::Exception("format is not WAVE, probably not a wav file.");
    }

    bool isFmtSubChunkPresent = false;
    bool isDataSubChunkPresent = false;

    //Read subChunks
    while (offset < contents.size()) {
        //Validate subChunks
        uint8_t bitsPerSample = 0;
        uint8_t channels = 0;
        size_t dataSubChunkOffset = 0;

        //Read subChunkId
        std::string subChunkId = std::string(contents.begin() + offset, contents.begin() + (offset + 4));
        offset += 4;
        if (subChunkId == "fmt ") {
            isFmtSubChunkPresent = true; //Mark fmt subchunk as present
            dataSubChunkOffset++;

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
                throw data::Exception("Only 16bits per sample wav is supported.");
            }

            //Add to subChunk List
            wavChunk.wavSubChunks.push_back(wavFormatSubChunk);

            //Assign additional values which will be needed while processing data subChunk
            bitsPerSample = (uint8_t)wavFormatSubChunk.bitsPerSample;
            channels = (uint8_t)wavFormatSubChunk.numChannels;
        } else if (subChunkId == "data") {
            if (!isFmtSubChunkPresent) {
                throw data::Exception("Probably corrupt wav, data subChunk present without fmt subChunk.");
            }
            isDataSubChunkPresent = true; //Mark data subchunk as present
            dataSubChunkLocation = dataSubChunkOffset; //Set dataSubChunkLocation for later use

            //Assign subChunkId
            data::WavDataSubChunk wavDataSubChunk(bitsPerSample, channels);
            wavDataSubChunk.subChunkId = subChunkId;

            //Read subChunkSize
            wavDataSubChunk.subChunkSize = ((uint8_t)contents[offset + 3] << 24) | ((uint8_t)contents[offset + 2] << 16) | ((uint8_t)contents[offset + 1] << 8) | ((uint8_t)contents[offset]);
            offset += 4;

            //Read data
            wavDataSubChunk.subChunkData = std::vector<int8_t>(contents.begin() + offset, contents.begin() + offset + wavDataSubChunk.subChunkSize);
            offset += wavDataSubChunk.subChunkSize;

            wavChunk.wavSubChunks.push_back(wavDataSubChunk);

        } else {
            data::WavSubChunk wavSubChunk;
            dataSubChunkOffset++;

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
        throw data::Exception("fmt subChunk is missing from file");
    }
    if (!isDataSubChunkPresent) {
        throw data::Exception("data subChunk is missing from file");
    }
}

void WavFile::demuxSamples()
{
    data::WavFormatSubChunk* formatSubChunk = static_cast<data::WavFormatSubChunk*>(&wavChunk.wavSubChunks[formatSubChunkLocation]);
    data::WavDataSubChunk* dataSubChunk = static_cast<data::WavDataSubChunk*>(&wavChunk.wavSubChunks[dataSubChunkLocation]);

    size_t sampleCount = (dataSubChunk->subChunkData.size() * 8) / formatSubChunk->bitsPerSample;
    wavFrames.reserve((size_t)(sampleCount / (samplesPerChannelPerFrame * formatSubChunk->numChannels)));
    size_t offset = 0;

    std::vector<int32_t> intSamples;
    intSamples.reserve(sampleCount);

    for (size_t i = 0; i < sampleCount; i++) {
        intSamples.push_back(((uint8_t)dataSubChunk->subChunkData[offset + 1] << 8) | ((uint8_t)dataSubChunk->subChunkData[offset]));
        offset += 2;
    }

    for (size_t i = 0; i < wavFrames.capacity(); i++) {
        std::vector<std::vector<int32_t>> allSamples;
        allSamples.reserve((size_t)formatSubChunk->numChannels);
        
        for (size_t j = 0; j < (size_t)formatSubChunk->numChannels; j++) {
            //Assign buffer
            std::vector<int32_t> samples;
            samples.reserve(samplesPerChannelPerFrame);
            //Read into buffer
            for (size_t k = 0; k < samplesPerChannelPerFrame; k++) {
                samples.push_back(intSamples[sampleCount * k + j]);
            }
            allSamples.push_back(samples);
        }
        data::WavFrame wavFrame = data::WavFrame((uint8_t)formatSubChunk->bitsPerSample, std::move(allSamples));
        wavFrames.push_back(wavFrame);
    }
}

void WavFile::writeToFile(std::ofstream& outputFile)
{
    (void)outputFile;
}
}