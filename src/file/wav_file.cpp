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

            //Add to subChunk List
            wavChunk.wavSubChunks.push_back(wavFormatSubChunk);

            //Assign additional values which will be needed while processing data subChunk
            bitsPerSample = (uint8_t)wavFormatSubChunk.bitsPerSample;
            channels = (uint8_t)wavFormatSubChunk.numChannels;

            std::cout << wavFormatSubChunk.subChunkId << std::endl;
        } else if (subChunkId == "data") {
            if (!isFmtSubChunkPresent) {
                throw data::Exception("Probably corrupt wav, data subChunk present without fmt subChunk.");
            }
            isDataSubChunkPresent = true; //Mark data subchunk as present

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
            std::cout << wavDataSubChunk.subChunkId << std::endl;

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
            std::cout << wavSubChunk.subChunkId << std::endl;
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

void WavFile::writeToFile(std::ofstream& outputFile)
{
    (void)outputFile;
}
}