#include <iostream>
#include "rice/rice.hpp"

int main(int, char**) {
    rice::RiceEncoder encoder;
    rice::RiceDecoder decoder;
    encoder.process();
    decoder.process(); 
    std::cout << "Hello, world!\n" << std::endl;
}
