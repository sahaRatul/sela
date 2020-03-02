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
    if (wavChunk.chunkSize > contents.size()) {
        throw data::Exception("chunkSize exceeds file size, probably a corrupted file");
    }

    //Read format
    offset += 4;
    wavChunk.format = std::string(contents.begin() + (offset - 4), contents.begin() + offset);
    if (wavChunk.format != "WAVE") {
        throw data::Exception("format is not WAVE, probably not a wav file.");
    }

    //Read subChunks
    while (offset < contents.size()) {
        data::WavSubChunk wavSubChunk;

        //Read subChunkId
        wavSubChunk.subChunkId = std::string(contents.begin() + offset, contents.begin() + (offset + 4));
        offset += 4;

        //Read subChunkSize
        wavSubChunk.subChunkSize = ((uint8_t)contents[offset + 3] << 24) | ((uint8_t)contents[offset + 2] << 16) | ((uint8_t)contents[offset + 1] << 8) | ((uint8_t)contents[offset]);
        offset += 4;

        //Read data
        wavSubChunk.subChunkData = std::vector<int8_t>(contents.begin() + offset, contents.begin() + offset + wavSubChunk.subChunkSize);
        offset += wavSubChunk.subChunkSize;

        wavChunk.wavSubChunks.push_back(wavSubChunk);
    }

    //Validate subChunks
    bool isFmtSubChunkPresent = false;
    bool isDataSubChunkPresent = false;
    for (data::WavSubChunk wavSubChunk : wavChunk.wavSubChunks) {
        if(wavSubChunk.subChunkId == "fmt ") {
            isFmtSubChunkPresent = true;
        }
        if(wavSubChunk.subChunkId == "data") {
            isDataSubChunkPresent = true;
        }
    }

    if(!isFmtSubChunkPresent) {
        throw data::Exception("fmt subChunk is missing from file");
    }
    else if(!isDataSubChunkPresent) {
        throw data::Exception("data subChunk is missing from file");
    }
}

void WavFile::writeToFile(std::ofstream& outputFile)
{
    (void)outputFile;
}
}