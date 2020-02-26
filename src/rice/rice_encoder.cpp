#include <iostream>
#include "../include/rice.hpp"

namespace rice
{
    RiceEncoder::RiceEncoder(data::RiceDecodedData decodedData) {
        this->input = decodedData.decodedData;
    }

    void RiceEncoder::process() {
        
    }
}