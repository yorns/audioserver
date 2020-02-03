#ifndef PLAYLISTACCESS_H
#define PLAYLISTACCESS_H

#include <string>
#include <nlohmann/json.hpp>

#include "database/SimpleDatabase.h"
#include "common/logger.h"

class PlaylistAccess
{
    SimpleDatabase& m_database;
    std::string& m_currentPlaylist;
    bool m_albumPlaylist  { false };

    std::string convertToJson(const std::vector<Id3Info> list);

public:
    PlaylistAccess(SimpleDatabase& simpleDatabase, std::string& currentPlaylist)
        : m_database(simpleDatabase), m_currentPlaylist(currentPlaylist) {}

    std::string access(const utility::Extractor::UrlInformation &urlInfo);
};

#endif // PLAYLISTACCESS_H
