#include "include/data/exception.hpp"
#include "include/sela/decoder.hpp"
#include "include/rice.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Provide input file" << std::endl;
    } else {
        std::ifstream inputFile(argv[1], std::ios::binary);
        try {
            sela::Decoder decoder = sela::Decoder(inputFile);
            decoder.process();
        } catch (data::Exception exception) {
            std::cerr << exception.exceptionMessage << std::endl;
            inputFile.close();
            return 1;
        }
    }
    return 0;
}
