#ifndef _RICE_DECODED_DATA_H_
#define _RICE_DECODED_DATA_H_

#include <cstdint>
#include <vector>

namespace data {
class RiceDecodedData {
public:
    const std::vector<int32_t> decodedData;
    explicit RiceDecodedData(const std::vector<int32_t>&& decodedData)
        : decodedData(decodedData)
    {
    }
};
}

#endif