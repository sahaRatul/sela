#include "include/rice.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    data::RiceDecodedData* data = new data::RiceDecodedData(std::vector<int32_t> { 10, -20, 30 });

    //Encode
    rice::RiceEncoder* enc = new rice::RiceEncoder(*data);
    data::RiceEncodedData encodedData = enc->process();

    //Decode
    rice::RiceDecoder* dec = new rice::RiceDecoder(encodedData);
    data::RiceDecodedData decodedData = dec->process();
    return 0;
}
