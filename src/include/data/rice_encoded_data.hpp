#ifndef _RICE_ENCODED_DATA_H_
#define _RICE_ENCODED_DATA_H_

#include <vector>

namespace data {
    class RiceEncodedData {
        public:
            uint32_t optimumRiceParam;
            uint32_t dataCount;
            std::vector<uint32_t> encodedData;
            RiceEncodedData(int32_t optimumRiceParam, int32_t dataCount, std::vector<uint32_t> encodedData) {
                this->optimumRiceParam = optimumRiceParam;
                this->dataCount = dataCount;
                this->encodedData = encodedData;
            }
    };
}

#endif