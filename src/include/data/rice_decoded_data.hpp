#ifndef _RICE_DECODED_DATA_H_
#define _RICE_DECODED_DATA_H_

#include <cstdint>
#include <vector>

namespace data {
class RiceDecodedData {
public:
    std::vector<int32_t> decodedData;
    RiceDecodedData(std::vector<int32_t> decodedData)
    {
        this->decodedData = decodedData;
    }
};
} // namespace data

#endif