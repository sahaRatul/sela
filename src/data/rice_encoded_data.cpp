#include <cstdint>
#include <vector>

namespace data {
    class RiceEncodedData {
        public:
            int32_t optimumRiceParam;
            int32_t dataCount;
            std::vector<int32_t> encodedData;
    };
}