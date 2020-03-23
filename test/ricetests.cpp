#include <catch.hpp>
#include <iostream>
#include <random>

#include "../src/include/rice.hpp"

TEST_CASE("Rice Encoder/Decoder combined test")
{
    std::vector<int32_t> input;
    input.reserve(100);
    for (size_t i = 0; i < 100; i++) {
        input.push_back(200 + (std::rand() % (201)));
    }
    data::RiceDecodedData data = data::RiceDecodedData(std::move(input));

    //Encode
    rice::RiceEncoder enc = rice::RiceEncoder(data);
    data::RiceEncodedData encodedData = enc.process();

    //Decode
    rice::RiceDecoder dec = rice::RiceDecoder(encodedData);
    data::RiceDecodedData decodedData = dec.process();
    REQUIRE(data.decodedData.size() == decodedData.decodedData.size());
    REQUIRE(data.decodedData == decodedData.decodedData);
}