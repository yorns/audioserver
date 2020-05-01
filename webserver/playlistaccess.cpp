#include "playlistaccess.h"
#include <nlohmann/json.hpp>
#include "common/logger.h"
#include "common/FileType.h"
#include "database/NameType.h"

using namespace LoggerFramework;

std::string PlaylistAccess::convertToJson(const std::optional<std::vector<Id3Info>> list) {

    nlohmann::json json;

    try {
        if (list) {
            for(auto item : *list) {
                nlohmann::json jentry;
                jentry[std::string(ServerConstant::Parameter::Database::uid)] = item.uid;
                jentry[std::string(ServerConstant::Parameter::Database::interpret)] = item.performer_name;
                jentry[std::string(ServerConstant::Parameter::Database::album)] = item.album_name;
                jentry[std::string(ServerConstant::Parameter::Database::titel)] = item.title_name;
                std::string relativCoverPath =
                        Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::CoversRelative) +
                        "/" + item.uid + item.fileExtension;
                jentry[std::string(ServerConstant::Parameter::Database::imageFile)] = relativCoverPath;
                jentry[std::string(ServerConstant::Parameter::Database::trackNo)] = item.track_no;
                json.push_back(jentry);
            }
        }
    } catch (const nlohmann::json::exception& ex) {
        logger(Level::error) << "conversion to json failed: " << ex.what();
        return "[]";
    }

    return json.dump();
}

std::string PlaylistAccess::convertToJson(const std::vector<Database::Playlist> list) {

    nlohmann::json json;

    try {
        for(auto item : list) {
            nlohmann::json jentry;
            jentry[std::string(ServerConstant::Parameter::Database::uid)] = item.getUniqueID();
            jentry[std::string(ServerConstant::Parameter::Database::album)] = item.getName();
            jentry[std::string(ServerConstant::Parameter::Database::interpret)] = ""; // item.getPerformer();
            jentry[std::string(ServerConstant::Parameter::Database::imageFile)] = item.getCover();
            json.push_back(jentry);
        }

    } catch (const nlohmann::json::exception& ex) {
        logger(Level::error) << "conversion to json failed: " << ex.what();
    }

    return json.dump();
}


std::string PlaylistAccess::access(const utility::Extractor::UrlInformation &urlInfo) {

    logger(Level::debug) << "playlist request: <"<<urlInfo->parameter<<"> with value <"<<urlInfo->value<<">\n";

    if (urlInfo->parameter == ServerConstant::Command::create) {
        auto ID = m_database.createPlaylist(urlInfo->value, Database::Persistent::isPermanent);
        if (ID && !ID->empty()) {
            logger(Level::info) << "create playlist with name <" << urlInfo->value << "> "<<"-> "<<*ID<<" \n";
            m_database.setCurrentPlaylistUniqueId(*ID);
            m_database.writeChangedPlaylists();
            return R"({"result": "ok"})";
        } else {
            logger(Level::warning) << "create playlist with name <" << urlInfo->value << "> failed, name is not new\n";
            return R"({"result": "playlist not new"})";
        }
    }

    if (urlInfo->parameter == ServerConstant::Command::getAlbumList) {
        auto list = m_database.searchPlaylistItems(urlInfo->value, Database::SearchAction::alike);
        return convertToJson(list);
    }


    /* change to new playlist */
    if (urlInfo->parameter == ServerConstant::Command::change) {

        auto playlistList = m_database.searchPlaylistItems(urlInfo->value, Database::SearchAction::uniqueId);

        if (playlistList.size() != 1) {
            logger(Level::warning) << "playlist with uniqueId <" << urlInfo->value << "> not found\n";
            return R"({"result": "playlist not found"})";
        }

        m_database.setCurrentPlaylistUniqueId(playlistList[0].getUniqueID());
        return R"({"result": "ok"})";
    }

    if (urlInfo->parameter == ServerConstant::Command::add) {
        auto currentPlaylist { m_database.getCurrentPlaylistUniqueID() };
        if ( currentPlaylist) {
            logger(Level::debug) << "add audio file with unique ID <" << urlInfo->value
                                 << "> to playlist <"
                                 << *currentPlaylist
                                 << ">\n";
            std::string uniqueID { urlInfo->value };
            if (m_database.addToPlaylistUID(*currentPlaylist, std::move(uniqueID))) {
                m_database.writeChangedPlaylists();
                logger(Level::debug) << "adding audio file <" << urlInfo->value <<"> to playlist ID <"<<*currentPlaylist<<">\n";
                return R"({"result": "ok"})";
            }
        }
        else {
            logger(Level::warning)<< "no current playlist to add audio file id <"<<urlInfo->value<<"> to\n";
        }
        return R"({"result": "cannot add <)" + urlInfo->value + "> to playlist <" + *currentPlaylist + ">}";

    }

    if (urlInfo->parameter == ServerConstant::Command::show) {

        std::string playlistName;
        if (!urlInfo->value.empty())
            playlistName = urlInfo->value;
        else if ( auto playlist = m_database.getCurrentPlaylistUniqueID() ) {
            auto itemList = m_database.getIdListOfItemsInPlaylistId(*playlist);
            logger(Level::debug) << "show (" << itemList.size() << ") elements for playlist <" << *playlist << ">\n";
            return convertToJson(itemList);
        }
        return "[]";
    }

    /* returns all playlist names */
    if (urlInfo->parameter == ServerConstant::Command::showLists) {
        logger(Level::info) << "show all playlists\n";
        std::vector<std::pair<std::string, std::string>> lists = m_database.getAllPlaylists();
        if (!lists.empty()) {
            logger(Level::debug) << "playlists are \n";
            nlohmann::json json;
            nlohmann::json jsonList;
            try {
                for (auto item : lists) {
                    logger(Level::debug) << item.first << " - " << item.second << "\n";
                    nlohmann::json jentry;
                    jentry[std::string(ServerConstant::Parameter::Database::uid)] = item.first;
                    jentry[std::string(ServerConstant::Parameter::Database::playlist)] = item.second;
                    jsonList.push_back(jentry);
                }
                json["playlists"] = jsonList;
                if (auto currentPlaylistUniqueID = m_database.getCurrentPlaylistUniqueID()) {
                if (auto playlistRealname = m_database.convertPlaylist(*currentPlaylistUniqueID, Database::NameType::uniqueID))
                    json["actualPlaylist"] = *playlistRealname;
                else
                    json["actualPlaylist"] = std::string("");
                }
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
