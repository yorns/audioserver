#ifndef STRINGMANIPULATOR_H
#define STRINGMANIPULATOR_H

#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include <sstream>

namespace Common {

std::vector<std::string> extractWhatList(const std::string& what);

std::string str_tolower(std::string s);

}

#endif // STRINGMANIPULATOR_H
