#ifndef PLAYLISTACCESS_H
#define PLAYLISTACCESS_H

#include <string>
#include <memory>

#include "webserver/databaseaccess.h"
#include "webserver/playeraccess.h"
#include "database/playlist.h"
#include "playerinterface/Player.h"

class PlaylistAccess
{
    DatabaseAccess m_database;
    PlayerAccess m_player;

    std::string convertToJson(const std::optional<std::vector<Id3Info>> list);
    std::string convertToJson(const std::vector<Database::Playlist> list);
    std::string convertToJson(const std::optional<boost::uuids::uuid> actualPlaylistUuid);

    std::string add(const std::string& value);
    std::string create(const std::string& value);
    std::string getAlbumList(const std::string& value);
    std::string getAlbumUid(const std::string &value);
    std::string change(const std::string& value);
    std::string show(const std::string& value);
    std::string showLists(const std::string& value);
    std::string getCurrentPlaylistUID(const std::string& value);

public:
    PlaylistAccess(DatabaseAccess database, PlayerAccess player)
        : m_database(database), m_player(player) {}

    std::string access(const utility::Extractor::UrlInformation &urlInfo);

    std::string restAPIDefinition();

};

#endif // PLAYLISTACCESS_H
