#ifndef PLAYLISTACCESS_H
#define PLAYLISTACCESS_H

#include <string>

#include "database/SimpleDatabase.h"
#include "database/playlist.h"

class PlaylistAccess
{
    Database::SimpleDatabase& m_database;

    std::string convertToJson(const std::optional<std::vector<Id3Info>> list);
    std::string convertToJson(const std::vector<Database::Playlist> list);

public:
    PlaylistAccess(Database::SimpleDatabase& simpleDatabase)
        : m_database(simpleDatabase) {}

    std::string access(const utility::Extractor::UrlInformation &urlInfo);
};

#endif // PLAYLISTACCESS_H
