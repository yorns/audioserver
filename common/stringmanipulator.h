#ifndef STRINGMANIPULATOR_H
#define STRINGMANIPULATOR_H

#include <string>
#include <vector>

namespace Common {

std::vector<std::string> extractWhatList(const std::string& what);

std::string str_tolower(std::string s);

}

#endif // STRINGMANIPULATOR_H
