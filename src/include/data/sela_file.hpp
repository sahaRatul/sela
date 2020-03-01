#ifndef _SELA_FILE_H_
#define _SELA_FILE_H_

#include "sela_frame.hpp"
#include "sela_header.hpp"

#include <vector>

namespace data {
class SelaFile {
public:
    SelaHeader selaHeader;
    std::vector<SelaFrame> selaFrames;
};
}

#endif