#ifndef DATABASE_CREDENTIAL_H
#define DATABASE_CREDENTIAL_H

#include <vector>
#include <string>
#include <tuple>
#include <optional>
#include <algorithm>
#include <fstream>

#include <boost/algorithm/string/trim.hpp>

#include "common/filesystemadditions.h"
#include "common/logger.h"

using namespace LoggerFramework;

namespace Database {

class Credential {

    std::vector<std::tuple<std::string, std::string>> credentialList;

public:
    Credential() = default;

    std::optional<std::string> passwordFind(const std::string& name);

    bool read();

};

}

#endif // DATABASE_CREDENTIAL_H
