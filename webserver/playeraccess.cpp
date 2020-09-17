#include <boost/filesystem.hpp>
#include "common/Constants.h"

#include "playeraccess.h"

using namespace LoggerFramework;

std::string PlayerAccess::access(const utility::Extractor::UrlInformation &urlInfo) {

    if (!urlInfo || urlInfo->m_parameterList.size() != 1) {
        logger(Level::warning) << "invalid url given for database access\n";
        return R"({"result": "illegal url given" })";
    }

    auto parameter = urlInfo->m_parameterList.at(0).name;
    auto value = std::string(urlInfo->m_parameterList.at(0).value);

    logger(Level::info) << "player access - parameter: <"<<parameter<<"> value: <"<<value<<">\n";

    if ( parameter == ServerConstant::Command::play &&
         value == ServerConstant::Value::_true) {

        if (m_player->isPlaying()) {
            logger(Level::info) << "Pause request\n";
            m_player->pause_toggle();
        } else {
            logger(Level::info) << "Play request\n";

        if (m_player->startPlay())
            return R"({"result": "ok"})";
        else
            return R"({"result": "cannot find playlist"})";
        }
    }

    if (parameter == ServerConstant::Parameter::Player::next &&
            value == ServerConstant::Value::_true) {
        m_player->next_file();
        return R"({"result": "ok"})";
    }

    if (parameter == ServerConstant::Parameter::Player::previous &&
            value == ServerConstant::Value::_true) {
        m_player->prev_file();
        return R"({"result": "ok"})";
    }

    if (parameter == ServerConstant::Parameter::Player::stop &&
            value == ServerConstant::Value::_true) {
        m_player->stop();
        return R"({"result": "ok"})";
    }

    if (parameter == ServerConstant::Parameter::Player::reset &&
            value == ServerConstant::Value::_true) {
        m_player->resetPlayer();
        return R"({"result": "ok"})";
    }

    if (parameter == ServerConstant::Parameter::Player::pause &&
            value == ServerConstant::Value::_true) {
        m_player->pause_toggle();
        return R"({"result": "ok"})";
    }

    if (parameter == ServerConstant::Parameter::Player::select) {
        try {
            auto ID = boost::lexical_cast<boost::uuids::uuid>(value);
            if (m_player->jump_to_fileUID(ID))
                return R"({"result": "ok"})";
            else {
                logger(Level::warning) << "cannot find neither song nor playlist\n";
                return R"({"result": "cannot find neither song nor playlist"})";
            }
        } catch (std::exception&) {
            logger(Level::warning) << "wrong audio UID: cannot be extracted\n";
            return R"({"result": "wrong ID"})";
        }
    }

    if (parameter == ServerConstant::Parameter::Player::toPosition) {
        int position {0};
        try {
            position = std::stoi(value);
        } catch (const std::exception& ex) {
            logger(Level::warning) << "toPosition request with value <"
                                   << value
                                   << "> could not be parsed ("
                                   << ex.what() << ")";
            return R"({"result": "jump to position: position could not be parsed"})";
        }
        m_player->jump_to_position(position);
        return R"({"result": "ok"})";
    }

    if (parameter == ServerConstant::Parameter::Player::volume) {
        uint32_t volume {0};
        try {
            volume = static_cast<uint32_t>(std::stoi(value));
        } catch (const std::exception& ex) {
            logger(Level::warning) << "volume request with value <"
                                   << value
                                   << "> could not be parsed ("
                                   << ex.what() << ")";
            return R"({"result": "jump to position: position could not be parsed"})";
        }
        m_player->setVolume(volume);
        return R"({"result": "ok"})";
    }

    if (parameter == ServerConstant::Parameter::Player::fastForward &&
            value == ServerConstant::Value::_true) {
        m_player->seek_forward();
        return R"({"result": "ok"})";
    }

    if (parameter == ServerConstant::Parameter::Player::fastBackward &&
            value == ServerConstant::Value::_true) {
        m_player->seek_backward();
        return R"({"result": "ok"})";
    }

    if (parameter == ServerConstant::Parameter::Player::toggleShuffle) {
        if (value == ServerConstant::Value::_true) {
            m_player->toggleShuffle();
            return R"({"result": "ok"})";
        }
        return R"({"result": "unknown value set; allowed <true|false>"})";
    }

    if (parameter == ServerConstant::Parameter::Player::toggleLoop) {
        if (value == ServerConstant::Value::_true) {
            m_player->toogleLoop();
            return R"({"result": "ok"})";
        }
        return R"({"result": "unknown value set; allowed <true|false>"})";
    }

    return R"({"result": "player command unknown"})";
}
