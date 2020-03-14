#include "include/data/exception.hpp"
#include "include/sela/decoder.hpp"
#include "include/sela/encoder.hpp"
#include "include/file/wav_file.hpp"
#include "include/rice.hpp"
#include <fstream>
#include <iostream>

void printUsage(const std::string& programName) {
    std::string usageString = "";
    usageString += "Usage: \n\n";
    usageString += "Encoding a file:\n";
    usageString += programName + " -e path/to/input.wav path/to/output.sela\n\n";
    usageString += "Decoding a file:\n";
    usageString += programName + " -d path/to/input.sela path/to/output.wav\n";
    std::cout << usageString << std::endl;
}

void encodeFile(std::ifstream& inputFile, std::ofstream& outputFile) {
    sela::Encoder encoder = sela::Encoder(inputFile);
    encoder.process();
}

void decodeFile(std::ifstream& inputFile, std::ofstream& outputFile) {
    sela::Decoder decoder = sela::Decoder(inputFile);
    decoder.process();
}

int main(int argc, char** argv)
{
    std::cout << "SimplE Lossless Audio. Released under MIT license" << std::endl;
    std::string programName = std::string(argv[0]);
    if (argc < 2) {
        printUsage(programName);
    } else {
        try {
            std::string secondArg = std::string(argv[1]);
            if(secondArg == "-e" && argc == 4) {
                std::ifstream inputFile(argv[2], std::ios::binary);
                std::ofstream outputFile(argv[3], std::ios::binary);
                encodeFile(inputFile, outputFile);
            }
            else if(secondArg == "-d" && argc == 4) {
                std::ifstream inputFile(argv[2], std::ios::binary);
                std::ofstream outputFile(argv[3], std::ios::binary);
                decodeFile(inputFile, outputFile);
            }
            else {
                printUsage(programName);
            }
        } catch(data::Exception exception) {
            std::cerr << exception.exceptionMessage << std::endl;
            return 1;
        }
        
    }
    return 0;
}