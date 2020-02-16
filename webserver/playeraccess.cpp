#include <boost/filesystem.hpp>
#include "common/Constants.h"

#include "playeraccess.h"

using namespace LoggerFramework;

std::string PlayerAccess::access(const utility::Extractor::UrlInformation &urlInfo) {

    if ( urlInfo->parameter == ServerConstant::Command::play &&
         urlInfo->value == ServerConstant::Value::_true) {

        logger(Level::info) << "Play request\n";

        std::stringstream albumPlaylist;
        std::stringstream genericPlaylist;
        albumPlaylist << ServerConstant::base_path << "/" << ServerConstant::albumPlaylistDirectory << "/" << m_currentPlaylist << ".m3u";
        genericPlaylist << ServerConstant::base_path << "/" << ServerConstant::playlistPath << "/" << m_currentPlaylist << ".m3u";

        boost::filesystem::path pathAlbum { albumPlaylist.str() };
        std::string playlist = boost::filesystem::exists(pathAlbum)?albumPlaylist.str():genericPlaylist.str();

        m_player->startPlay(std::move(playlist));
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
        m_player->pause_toggle();
        return R"({"result": "ok"})";
    }

    return R"({"result": "player command unknown"})";
}
