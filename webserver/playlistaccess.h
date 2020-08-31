#ifndef PLAYLISTACCESS_H
#define PLAYLISTACCESS_H

#include <string>
#include <memory>

#include "database/SimpleDatabase.h"
#include "database/playlist.h"
#include "playerinterface/Player.h"

class PlaylistAccess
{
    Database::SimpleDatabase& m_database;
    std::unique_ptr<BasePlayer>& m_player;

    std::string convertToJson(const std::optional<std::vector<Id3Info>> list);
    std::string convertToJson(const std::vector<Database::Playlist> list);

public:
    PlaylistAccess(Database::SimpleDatabase& simpleDatabase, std::unique_ptr<BasePlayer>& player)
        : m_database(simpleDatabase), m_player(player) {}

    std::string access(const utility::Extractor::UrlInformation &urlInfo);

    std::string restAPIDefinition();

};

#endif // PLAYLISTACCESS_H
