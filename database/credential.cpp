#include "credential.h"

std::optional<std::string> Database::Credential::passwordFind(const std::string &name) {

    std::vector<std::tuple<std::string, std::string>>::iterator pw =  std::find_if(
                begin(credentialList),
                end(credentialList),
                [&name](const std::tuple<std::string, std::string>& item) {
        return std::get<0>(item) == name;
    }
    );

    if (pw == std::end(credentialList))
        return std::nullopt;

    std::optional<std::string> ret(std::get<1>(*pw));

    if (ret)
        logger(Level::debug) << "password found\n";

    return ret;
}

bool Database::Credential::read() {
    std::fstream inStream;
    auto tagFile = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Credential) +
            "/" + std::string(ServerConstant::credentialFile);
    inStream.open(tagFile, std::ios_base::in);

    if (!inStream.good()) {
        logger(Level::warning) << "cannot open credential file\n";
        return false;
    }

    credentialList.clear();

    for (std::string line; std::getline(inStream, line); ) {

        boost::trim(line);

        if ((line.length() > 0 && line[0] == '#') ||
                line.find(':') == std::string::npos)
            continue;

        std::string name = line.substr(0, line.find(':'));
        std::string pw = line.substr(line.find(':')+1);

        if (!name.empty() && !pw.empty())
            credentialList.push_back(std::make_tuple(name, pw));
    }

    logger(Level::info) << "read <"<<credentialList.size()<<"> credentials\n";

    return true;
}
