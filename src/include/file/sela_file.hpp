#ifndef _SELA_FILE_H_
#define _SELA_FILE_H_

#include "../data/sela_frame.hpp"
#include "../data/sela_header.hpp"

#include <fstream>
#include <vector>

namespace file {
class SelaFile {
public:
    data::SelaHeader selaHeader;
    std::vector<data::SelaFrame> selaFrames;
    void readFromFile(std::ifstream inputFile);
    void writeToFile(std::ofstream outputFile);
};
}

#endif