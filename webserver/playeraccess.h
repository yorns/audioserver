#ifndef PLAYERACCESS_H
#define PLAYERACCESS_H

#include <memory>
#include <string>
#include "common/Extractor.h"
#include "playerinterface/Player.h"

class PlayerAccess
{
    typedef std::function<Common::AlbumPlaylistAndNames()> GetAlbumPlaylistAndNames;

    std::unique_ptr<BasePlayer>& m_player;
    GetAlbumPlaylistAndNames m_getAlbumPlaylistAndNames;

public:
    PlayerAccess(std::unique_ptr<BasePlayer>& player, GetAlbumPlaylistAndNames&& getAlbumPlaylistAndNames)
        : m_player(player), m_getAlbumPlaylistAndNames(std::move(getAlbumPlaylistAndNames)) {}

    PlayerAccess() = delete;

    std::string access(const utility::Extractor::UrlInformation &urlInfo);

    std::string restAPIDefinition();

};

#endif // PLAYERACCESS_H
