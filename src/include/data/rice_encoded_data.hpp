#ifndef _RICE_ENCODED_DATA_H_
#define _RICE_ENCODED_DATA_H_

#include <cstdint>
#include <vector>

namespace data {
class RiceEncodedData {
public:
    uint32_t optimumRiceParam;
    uint32_t dataCount;
    const std::vector<uint32_t> encodedData;
    RiceEncodedData(int32_t optimumRiceParam,
        int32_t dataCount,
        const std::vector<uint32_t>&& encodedData) noexcept
        : optimumRiceParam(optimumRiceParam)
        , dataCount(dataCount)
        , encodedData(encodedData)
    {
    }
};
}

#endif