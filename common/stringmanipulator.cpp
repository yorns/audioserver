#include "stringmanipulator.h"

std::vector<std::string> Common::extractWhatList(const std::string &what)
{
    // cut what string:
    std::vector<std::string> whatList;
    std::stringstream tmp;
    tmp << what;
    while (tmp.good()) {
        std::string whatItem;
        tmp >> whatItem;
        if (!whatItem.empty())
            whatList.emplace_back(whatItem);
    }
    return whatList;
}

std::string Common::str_tolower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); }
    );
    return s;
}
