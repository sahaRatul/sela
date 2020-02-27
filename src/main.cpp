#include <iostream>
#include "include/rice.hpp"

int main(int argc, char** argv) {
    data::RiceDecodedData* data = new data::RiceDecodedData(std::vector<int32_t> { 10, 20, 30 });
    rice::RiceEncoder* enc = new rice::RiceEncoder(*data);
    
    enc->process();
    return 0;
}
