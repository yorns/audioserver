#ifndef BASE64_H
#define BASE64_H

#include <vector>
#include <string>

namespace utility {

typedef char BYTE;

std::string base64_encode(BYTE const* buf, unsigned int bufLen);
std::vector<BYTE> base64_decode(std::string const&);

}

#endif // BASE64_H
