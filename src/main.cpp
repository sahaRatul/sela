#include "include/data/exception.hpp"
#include "include/file/wav_file.hpp"
#include "include/rice.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Provide input file" << std::endl;
    } else {
        std::cout << argv[1] << std::endl;
        std::ifstream inputFile(argv[1], std::ios::binary);

        file::WavFile wavFile;
        try {
            wavFile.readFromFile(inputFile);
        } catch (data::Exception exception) {
            std::cerr << exception.exceptionMessage << std::endl;
            inputFile.close();
            return 1;
        }
    }
    return 0;
}
