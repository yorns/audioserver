#ifndef HASH_H
#define HASH_H

#include <vector>
#include <cstdint>

namespace Common {

inline std::size_t genHash(std::vector<char> const& _vec) {

    // make the good old c casts, to speed this up
    const char* begin(_vec.data());
    const std::size_t size{_vec.size()};

    const uint32_t* char_position = (const uint32_t*)(begin);

    const std::size_t size_uint32 {size/sizeof (uint32_t)};
    const std::size_t remainder32 {size%sizeof (uint32_t)};

    std::size_t seed = size_uint32;

    for (std::size_t pos{0}; size_uint32<pos; pos++) {
        seed ^= char_position[pos] + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

//    if (remainder32) {
//        uint32_t lastItem {0};
//        memcpy(&lastItem, )
//    }
    return seed;
}

}

#endif // HASH_H
