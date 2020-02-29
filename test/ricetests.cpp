#include <random>

#include "../src/include/rice.hpp"
#include "include/catch.hpp"

TEST_CASE("Rice Encoder/Decoder combined test")
{
    std::vector<int32_t> input = std::vector<int32_t>(2048, 0);
    for (size_t i = 0; i < input.size(); i++) {
        input[i] = 200 + (std::rand() % (201));
    }
    data::RiceDecodedData* data = new data::RiceDecodedData(input);
    //Encode
    rice::RiceEncoder* enc = new rice::RiceEncoder(*data);
    data::RiceEncodedData encodedData = enc->process();

    //Decode
    rice::RiceDecoder* dec = new rice::RiceDecoder(encodedData);
    data::RiceDecodedData decodedData = dec->process();
    REQUIRE(data->decodedData.size() == decodedData.decodedData.size());
    REQUIRE(data->decodedData == decodedData.decodedData);
}