#include "include/data/exception.hpp"
#include "include/sela/decoder.hpp"
#include "include/sela/encoder.hpp"
#include "include/file/wav_file.hpp"
#include "include/rice.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    std::cout << argv[0] << std::endl;
    if (argc < 1) {
        std::cout << "Provide input file" << std::endl;
    } else {
        std::ifstream inputFile("C:/Users/Ratul Saha/Desktop/Source/sela/build/knights.wav", std::ios::binary);
        try {
            //sela::Decoder decoder = sela::Decoder(inputFile);
            //decoder.process();

            sela::Encoder encoder = sela::Encoder(inputFile);
            encoder.process();

            //file::WavFile wavFile;
            //wavFile.readFromFile(inputFile);
        } catch (data::Exception exception) {
            std::cerr << exception.exceptionMessage << std::endl;
            inputFile.close();
            return 1;
        }
    }
    return 0;
}
