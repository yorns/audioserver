#include "Player.h"
#include <regex>

void Player::setPlayerEndCB(const std::function<void(const std::string &)> &endfunc) {
    m_endfunc = endfunc;
}

std::string Player::extractName(const std::string& fullName) {
    std::string fileName;
    const std::regex pattern1{"/([^/]*)\\..*$"};
    std::smatch match1{};
    if (std::regex_search(fullName, match1, pattern1)) {
        fileName = match1[1].str();
        logger(Level::info) << "filename is <" << fileName << ">\n";
    } else {
        fileName.clear();
        logger(Level::warning) << "cannot extract filename\n";
    }
    return fileName;
}

