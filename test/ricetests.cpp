#define CATCH_CONFIG_MAIN

#include "../src/include/rice.hpp"
#include "include/catch.hpp"

TEST_CASE("Rice Encoder/Decoder combined test") {
    data::RiceDecodedData* data = new data::RiceDecodedData(std::vector<int32_t> { 10, -20, 30 });
    //Encode
    rice::RiceEncoder* enc = new rice::RiceEncoder(*data);
    data::RiceEncodedData encodedData = enc->process();

    //Decode
    rice::RiceDecoder* dec = new rice::RiceDecoder(encodedData);
    data::RiceDecodedData decodedData = dec->process();
    REQUIRE(data->decodedData.size() == decodedData.decodedData.size());
}