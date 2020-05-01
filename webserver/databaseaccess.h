#ifndef DATABASEACCESS_H
#define DATABASEACCESS_H

#include <optional>
#include <vector>
#include <string_view>
#include "database/SimpleDatabase.h"

class DatabaseAccess
{
    Database::SimpleDatabase& m_database;

    std::string convertToJson(const std::optional<std::vector<Id3Info>> list);
    std::string convertToJson(const std::vector<Database::Playlist>& list);

public:
    DatabaseAccess() = delete;
    DatabaseAccess(Database::SimpleDatabase& simpleDatabase): m_database(simpleDatabase) {}

    std::string access(const utility::Extractor::UrlInformation &urlInfo);
    std::optional<std::vector<char>> getVirtualFile(const std::string_view& target) const {
        logger(LoggerFramework::Level::debug) << "searching for file: "<<target<<"\n";
        auto pos1 = target.find_last_of("/");
        if (pos1 != std::string_view::npos) {
            auto pos2 = target.find_last_of(".");
            if (pos2 != std::string_view::npos) {
                std::string uid = std::string(target.substr(pos1+1, pos2-pos1-1));
                logger(LoggerFramework::Level::debug) << "searching for cover UID <"<<uid<<">\n";
                return m_database.getCover(uid);
            }
                    }
        return std::nullopt;
    }

};

#endif // DATABASEACCESS_H
