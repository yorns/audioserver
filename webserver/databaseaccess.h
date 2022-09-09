#ifndef DATABASEACCESS_H
#define DATABASEACCESS_H

#include <optional>
#include <vector>
#include <string_view>
#include <boost/uuid/uuid_io.hpp>
#include "database/SimpleDatabase.h"

class DatabaseAccess
{
    //Database::SimpleDatabase& m_database;
    std::shared_ptr<Database::SimpleDatabase> m_database;

    std::string convertToJson(const std::optional<std::vector<Id3Info>> list);
    std::string convertToJson(const std::vector<Database::Playlist>& list);

    std::optional<std::string> extractUuidFromTarget(std::string_view target);

    bool testUrlPath(std::string_view url, const std::string& path) {
        if (url.substr(0,2+path.length()) == "/"+path+"/")
            return true;
        else
            return false;
    }


public:
    DatabaseAccess() = delete;
    DatabaseAccess(std::shared_ptr<Database::SimpleDatabase> simpleDatabase): m_database(simpleDatabase) {}

    std::string access(const utility::Extractor::UrlInformation &urlInfo);

    std::string restAPIDefinition();

    bool loadDatabase() { if (!m_database) return false; m_database->loadDatabase(); return true; }

    std::optional<std::vector<char>> virtualImageHandler(const std::string_view& _target);
    std::optional<std::string> virtualAudioHandler(const std::string_view& _target);
    std::optional<std::string> virtualPlaylistHandler(const std::string_view& _target);
    std::optional<std::vector<char>> getVirtualFile(const std::string_view& target) const;

    std::shared_ptr<Database::SimpleDatabase> getDatabase() { return m_database; }

};

#endif // DATABASEACCESS_H
