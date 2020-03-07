#include <boost/filesystem.hpp>
#include "common/Constants.h"

#include "playeraccess.h"

using namespace LoggerFramework;

std::string PlayerAccess::access(const utility::Extractor::UrlInformation &urlInfo) {

    if ( urlInfo->parameter == ServerConstant::Command::play &&
         urlInfo->value == ServerConstant::Value::_true) {

        logger(Level::info) << "Play request\n";

        const auto [playlist, playlistUniqueId, playlistName] = m_getAlbumPlaylistAndNames();

        m_player->startPlay(playlist, playlistUniqueId, playlistName);
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

    if (urlInfo->parameter == ServerConstant::Parameter::Player::shuffle) {
        if (urlInfo->value == ServerConstant::Value::_true) {
            m_player->do_shuffle(true);
            return R"({"result": "ok"})";
        }
        if (urlInfo->value == ServerConstant::Value::_false) {
            m_player->do_shuffle(false);
            return R"({"result": "ok"})";
        }
        return R"({"result": "unknown value set; allowed <true|false>"})";
    }

    if (urlInfo->parameter == ServerConstant::Parameter::Player::repeat) {
        if (urlInfo->value == ServerConstant::Value::_true) {
            m_player->do_cycle(true);
            return R"({"result": "ok"})";
        }
        if (urlInfo->value == ServerConstant::Value::_false) {
            m_player->do_cycle(false);
            return R"({"result": "ok"})";
        }
        return R"({"result": "unknown value set; allowed <true|false>"})";
    }

    return R"({"result": "player command unknown"})";
}
