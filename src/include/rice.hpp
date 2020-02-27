#ifndef _RICE_H_
#define _RICE_H_

#include <vector>
#include "./data/rice_decoded_data.hpp"
#include "./data/rice_encoded_data.hpp"

namespace rice {
    class RiceEncoder {
        private:
            std::vector<int32_t> input;
            std::vector<uint64_t> unsignedInput;
            std::vector<int32_t> output;
            std::vector<int32_t> bitOutput;
            std::vector<int64_t> bitSizes;
            int32_t optimumRiceParam;
            int64_t requiredBits;
            const int32_t maxRiceParam = 20;
            inline void convertSignedToUnsigned();
            inline void calculateOptimumRiceParam();
            inline void generateEncodedBits();
            inline void writeInts();
        public:
            RiceEncoder(data::RiceDecodedData decodedData);
            data::RiceEncodedData process();
    };

    class RiceDecoder {
        private:
            std::vector<int32_t> input;
            std::vector<int32_t> bitInput;
            std::vector<uint64_t> unsignedOutput;
            std::vector<int32_t> output;
            int32_t dataCount;
            int32_t optimumRiceParam;
            inline void generateEncodedBits();
            inline void generateDecodedUnsignedInts();
            inline void convertUnsignedToSigned();
        public:
            RiceDecoder(data::RiceEncodedData encodedData);
            data::RiceDecodedData process();
    };
}

#endif