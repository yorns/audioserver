#ifndef DATABASEACCESS_H
#define DATABASEACCESS_H

#include <optional>
#include <vector>
#include <string_view>
#include <boost/uuid/uuid_io.hpp>
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
    std::optional<std::vector<char>> getVirtualFile(const std::string_view& target) const;

    std::string restAPIDefinition();

};

#endif // DATABASEACCESS_H
