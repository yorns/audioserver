#ifndef PLAYERACCESS_H
#define PLAYERACCESS_H

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

    std::string access(const utility::Extractor::UrlInformation &urlInfo) {

        if ( urlInfo->parameter == ServerConstant::Command::play &&
             urlInfo->value == ServerConstant::Value::_true) {
            logger(Level::info) << "Play request\n";
            std::stringstream albumPlaylist;
            std::stringstream genericPlaylist;
            albumPlaylist << ServerConstant::base_path << "/" << ServerConstant::albumPlaylistDirectory << "/" << m_currentPlaylist << ".m3u";
            genericPlaylist << ServerConstant::base_path << "/" << ServerConstant::playlistPath << "/" << m_currentPlaylist << ".m3u";
            m_player->startPlay(albumPlaylist?albumPlaylist.str():genericPlaylist.str(), "");
            return R"({"result": "ok"})";
        }

        if (urlInfo->parameter == ServerConstant::Parameter::Player::next &&
                urlInfo->value == ServerConstant::Value::_true) {
            m_player->next_file();
            return R"({"result": "ok"})";
        }

        if (urlInfo->parameter == ServerConstant::Parameter::Player::previous &&
                urlInfo->value == ServerConstant::Value::_true) {
            m_player->prev_file();
            return R"({"result": "ok"})";
        }

        if (urlInfo->parameter == ServerConstant::Parameter::Player::stop &&
                urlInfo->value == ServerConstant::Value::_true) {
            m_player->stop();
            return R"({"result": "ok"})";
        }

        if (urlInfo->parameter == ServerConstant::Parameter::Player::pause &&
                urlInfo->value == ServerConstant::Value::_true) {
            m_player->pause();
            return R"({"result": "ok"})";
        }

        return R"({"result": "player command unknown"})";
    }


};

#endif // PLAYERACCESS_H
