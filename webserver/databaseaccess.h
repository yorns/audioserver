#ifndef DATABASEACCESS_H
#define DATABASEACCESS_H

#include "database/SimpleDatabase.h"

class DatabaseAccess
{
    SimpleDatabase& m_database;

    std::string convertToJson(const std::vector<Id3Info> list);

public:
    DatabaseAccess() = delete;
    DatabaseAccess(SimpleDatabase& simpleDatabase): m_database(simpleDatabase) {}

    std::string access(const utility::Extractor::UrlInformation &urlInfo);

};

#endif // DATABASEACCESS_H
