#include <iostream>
#include "include/rice.hpp"

int main(int argc, char** argv) {
    data::RiceDecodedData data;
    data.decodedData = { 10, -20, 30 };

    rice::RiceEncoder* enc = new rice::RiceEncoder(data);
    enc->process();
    return 0;
}
