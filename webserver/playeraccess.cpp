#include <boost/filesystem.hpp>
#include "common/Constants.h"

#include "playeraccess.h"

using namespace LoggerFramework;

std::string PlayerAccess::access(const utility::Extractor::UrlInformation &urlInfo) {

    if (!urlInfo) {
        logger(Level::warning) << "invalid url given for playlist access\n";
        return R"({"result": "illegal url given" })";
    }

    logger(Level::info) << "player access - parameter: <"<<urlInfo->parameter<<"> value: <"<<urlInfo->value<<">\n";

    if ( urlInfo->parameter == ServerConstant::Command::play &&
         urlInfo->value == ServerConstant::Value::_true) {

        logger(Level::info) << "Play request\n";

        auto albumPlaylistAndNames = m_getAlbumPlaylistAndNames();

        if (m_player->startPlay(albumPlaylistAndNames,""))
          return R"({"result": "ok"})";
        else
            return R"({"result": "cannot find playlist"})";

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

    if (urlInfo->parameter == ServerConstant::Parameter::Player::select) {
        const std::string& fileUrl = urlInfo->value;
        if (m_player->jump_to_fileUID(fileUrl))
            return R"({"result": "ok"})";
        else {
            auto albumPlaylistAndNames = m_getAlbumPlaylistAndNames();

            if (albumPlaylistAndNames.m_playlistUniqueId != m_player->getPlaylistID() &&
                m_player->startPlay(albumPlaylistAndNames,fileUrl))
              return R"({"result": "ok"})";
            else
                return R"({"result": "cannot find neither song nor playlist"})";
        }
    }

    if (urlInfo->parameter == ServerConstant::Parameter::Player::toPosition) {
        int position {0};
        try {
            position = std::stoi(urlInfo->value);
        } catch (const std::exception& ex) {
            logger(Level::warning) << "toPosition request with value <"
                                   << urlInfo->value
                                   << "> could not be parsed ("
                                   << ex.what() << ")";
            return R"({"result": "jump to position: position could not be parsed"})";
        }
        m_player->jump_to_position(position);
        return R"({"result": "ok"})";
    }

    if (urlInfo->parameter == ServerConstant::Parameter::Player::volume) {
        uint32_t volume {0};
        try {
            volume = static_cast<uint32_t>(std::stoi(urlInfo->value));
        } catch (const std::exception& ex) {
            logger(Level::warning) << "volume request with value <"
                                   << urlInfo->value
                                   << "> could not be parsed ("
                                   << ex.what() << ")";
            return R"({"result": "jump to position: position could not be parsed"})";
        }
        m_player->setVolume(volume);
        return R"({"result": "ok"})";
    }

    if (urlInfo->parameter == ServerConstant::Parameter::Player::fastForward &&
            urlInfo->value == ServerConstant::Value::_true) {
        m_player->seek_forward();
        return R"({"result": "ok"})";
    }

    if (urlInfo->parameter == ServerConstant::Parameter::Player::fastBackward &&
            urlInfo->value == ServerConstant::Value::_true) {
        m_player->seek_backward();
        return R"({"result": "ok"})";
    }

    if (urlInfo->parameter == ServerConstant::Parameter::Player::toggleShuffle) {
        if (urlInfo->value == ServerConstant::Value::_true) {
            m_player->toggleShuffle();
            return R"({"result": "ok"})";
        }
        return R"({"result": "unknown value set; allowed <true|false>"})";
    }

    if (urlInfo->parameter == ServerConstant::Parameter::Player::toggleLoop) {
        if (urlInfo->value == ServerConstant::Value::_true) {
            m_player->toogleLoop();
            return R"({"result": "ok"})";
        }
        return R"({"result": "unknown value set; allowed <true|false>"})";
    }

    return R"({"result": "player command unknown"})";
}
