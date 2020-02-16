#ifndef PLAYERACCESS_H
#define PLAYERACCESS_H

#include <memory>
#include <string>
#include "common/Extractor.h"
#include "playerinterface/Player.h"

class PlayerAccess
{
    std::unique_ptr<BasePlayer>& m_player;
    std::string& m_currentPlaylist;

public:
    PlayerAccess(std::unique_ptr<BasePlayer>& player, std::string& currentPlaylist)
        : m_player(player), m_currentPlaylist(currentPlaylist) {}

    PlayerAccess() = delete;

    std::string access(const utility::Extractor::UrlInformation &urlInfo);


};

#endif // PLAYERACCESS_H
