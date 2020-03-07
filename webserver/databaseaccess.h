#ifndef DATABASEACCESS_H
#define DATABASEACCESS_H

#include "database/SimpleDatabase.h"

class DatabaseAccess
{
    Database::SimpleDatabase& m_database;

    std::string convertToJson(const std::optional<std::vector<Id3Info>> list);

public:
    DatabaseAccess() = delete;
    DatabaseAccess(Database::SimpleDatabase& simpleDatabase): m_database(simpleDatabase) {}

    std::string access(const utility::Extractor::UrlInformation &urlInfo);

};

#endif // DATABASEACCESS_H
