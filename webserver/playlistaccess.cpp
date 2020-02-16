#include "playlistaccess.h"
#include <nlohmann/json.hpp>
#include "common/logger.h"

using namespace LoggerFramework;

std::string PlaylistAccess::convertToJson(const std::vector<Id3Info> list) {
    nlohmann::json json;
    for(auto item : list) {
        nlohmann::json jentry;
        jentry[std::string(ServerConstant::Parameter::Database::uid)] = item.uid;
        jentry[std::string(ServerConstant::Parameter::Database::interpret)] = item.performer_name;
        jentry[std::string(ServerConstant::Parameter::Database::album)] = item.album_name;
        jentry[std::string(ServerConstant::Parameter::Database::titel)] = item.titel_name;
        jentry[std::string(ServerConstant::Parameter::Database::imageFile)] = item.imageFile;
        jentry[std::string(ServerConstant::Parameter::Database::trackNo)] = item.track_no;
        json.push_back(jentry);
    }
    return json.dump(2);
}

std::string PlaylistAccess::access(const utility::Extractor::UrlInformation &urlInfo) {

    logger(Level::debug) << "player request: <"<<urlInfo->parameter<<"> with value <"<<urlInfo->value<<">\n";

    if (urlInfo->parameter == ServerConstant::Command::create) {
        std::string ID = m_database.createPlaylist(urlInfo->value);
        m_albumPlaylist = false;
        logger(Level::info) << "create playlist with name <" << urlInfo->value << "> "<<"-> "<<ID<<" \n";
        if (!ID.empty()) {
            m_currentPlaylist = ID;
            m_albumPlaylist = false;
            m_database.writeChangedPlaylists();
            return R"({"result": "ok"})";
        } else {
            return R"({"result": "playlist not new"})";
        }
    }

    if (urlInfo->parameter == ServerConstant::Command::createAlbumList) {
        std::string ID = m_database.createAlbumPlaylistTmp(urlInfo->value);
        logger(Level::info) << "create album playlist with name <" << urlInfo->value << "> "<<"-> "<<ID<<" \n";
        if (!ID.empty()) {
            m_currentPlaylist = ID;
            m_albumPlaylist = true;
            m_database.writeChangedPlaylists();
            return R"({"result": "ok"})";
        } else {
            return R"({"result": "playlist not new"})";
        }
    }


    if (urlInfo->parameter == ServerConstant::Command::change) {

        if (m_database.isPlaylist(urlInfo->value)) {
            logger(Level::info) << "changed to playlist with name <" << urlInfo->value << ">\n";
            m_currentPlaylist = m_database.getNameFromHumanReadable(urlInfo->value);
            m_albumPlaylist = false;
            logger(Level::debug) << "current playlist is <" << m_currentPlaylist << ">\n";
            return R"({"result": "ok"})";
        } else {
            logger(Level::warning) << "playlist with name <" << urlInfo->value << "> not found\n";
            return R"({"result": "playlist not found"})";
        }
    }

    if (urlInfo->parameter == ServerConstant::Command::add) {
        if (!m_currentPlaylist.empty() && m_database.isPlaylistID(m_currentPlaylist)) {
            logger(Level::debug) << "add title with name <" << urlInfo->value << "> to playlist <"<<m_currentPlaylist<<">\n";
            m_database.addToPlaylistID(m_currentPlaylist, urlInfo->value);
            m_database.writeChangedPlaylists();
            return R"({"result": "ok"})";
        } else {
            return R"({"result": "cannot add <)" + urlInfo->value + "> to playlist <" + m_currentPlaylist + ">}";
        }
    }

    if (urlInfo->parameter == ServerConstant::Command::show) {
        try {
            if ((urlInfo->value.empty() && !m_currentPlaylist.empty()) || m_database.isPlaylist(urlInfo->value)) {
                std::string playlistName(m_currentPlaylist);
                if (!urlInfo->value.empty())
                    playlistName = m_database.getNameFromHumanReadable(urlInfo->value);
                logger(Level::debug) << "show playlist <" << playlistName << ">\n";
                auto list = m_database.showPlaylist(playlistName);
                std::vector<Id3Info> infoList;
                for (auto item : list) {
                    if (item.empty()) continue;
                    infoList.push_back(
                                m_database.findInDatabase(item, SimpleDatabase::DatabaseSearchType::uid).front());
                }
                return convertToJson(infoList);
            }
        } catch (...) {}
        return "[]";
    }

    if (urlInfo->parameter == ServerConstant::Command::showLists) {
        logger(Level::info) << "show all playlists\n";
        std::vector<std::pair<std::string, std::string>> lists = m_database.showAllPlaylists();
        if (!lists.empty()) {
            logger(Level::debug) << "playlists are \n";
            nlohmann::json json;
            nlohmann::json json1;
            try {
                for (auto item : lists) {
                    logger(Level::debug) << item.first << " - " << item.second << "\n";
                    nlohmann::json jentry;
                    jentry[std::string(ServerConstant::Parameter::Database::uid)] = item.first;
                    jentry[std::string(ServerConstant::Parameter::Database::playlist)] = item.second;
                    json1.push_back(jentry);
                }
                json["playlists"] = json1;
                json["actualPlaylist"] = m_database.getHumanReadableName(m_currentPlaylist);
            } catch (std::exception& e) {
                logger(Level::warning) << "error: " << e.what() << "\n";
            }
            return json.dump(2);
        } else {
            return "[]";
        }
    }

    return R"({"result": "cannot find parameter <)" + urlInfo->parameter + "> in playlist\"}";

}