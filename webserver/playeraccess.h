#ifndef PLAYERACCESS_H
#define PLAYERACCESS_H

#include <boost/filesystem.hpp>

#include "playerinterface/Player.h"
#include "common/Extractor.h"
#include "common/Constants.h"

class PlayerAccess
{
    std::unique_ptr<Player>& m_player;
    std::string& m_currentPlaylist;

public:
    PlayerAccess(std::unique_ptr<Player>& player, std::string& currentPlaylist)
        : m_player(player), m_currentPlaylist(currentPlaylist) {}

    PlayerAccess() = delete;

    std::string access(const utility::Extractor::UrlInformation &urlInfo);


};

#endif // PLAYERACCESS_H
