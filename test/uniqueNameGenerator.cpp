#include <cstdlib>
#include <iostream>

#include "common/NameGenerator.h"

int main(int argc, char* argv[])
{
    uint32_t number{8};
    if (argc == 2)
        number = static_cast<uint32_t>(std::atoi(argv[1]));

    for(uint32_t i{0}; i<number; ++i) {
        std::cout << "generated id: " << Common::NameGenerator::create("","").filename <<"\n";
    }
    return EXIT_SUCCESS;
}
