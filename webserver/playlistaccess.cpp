#include "playlistaccess.h"
#include <nlohmann/json.hpp>
#include <boost/uuid/uuid_io.hpp>
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
                jentry[std::string(ServerConstant::Parameter::Database::uid)] = boost::uuids::to_string(item.uid);
                jentry[std::string(ServerConstant::Parameter::Database::interpret)] = item.performer_name;
                jentry[std::string(ServerConstant::Parameter::Database::album)] = item.album_name;
                jentry[std::string(ServerConstant::Parameter::Database::titel)] = item.title_name;
                std::string relativCoverPath =
                        Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::CoversRelative) +
                        "/" + boost::lexical_cast<std::string>(item.uid) + item.fileExtension;
                jentry[std::string(ServerConstant::Parameter::Database::imageFile)] = relativCoverPath;
                jentry[std::string(ServerConstant::Parameter::Database::trackNo)] = item.track_no;
                json.push_back(jentry);
            }
        }
    } catch (const std::exception& ex) {
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
            jentry[std::string(ServerConstant::Parameter::Database::uid)] = boost::uuids::to_string(item.getUniqueID());
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

    if (!urlInfo || urlInfo->m_parameterList.size() != 1) {
        logger(Level::warning) << "invalid url given for database access\n";
        return R"({"result": "illegal url given" })";
    }

    auto parameter = urlInfo->m_parameterList.at(0).name;
    auto value = std::string(urlInfo->m_parameterList.at(0).value);

    logger(Level::info) << "playlist access - parameter: <"<<parameter<<"> value: <"<<value<<">\n";

    if (parameter == ServerConstant::Command::create) {
        auto ID = m_database.createPlaylist(value, Database::Persistent::isPermanent);
        if (ID && !ID->is_nil()) {
            logger(Level::info) << "create playlist with name <" << value << "> "<<"-> "<<*ID<<" \n";
            m_database.setCurrentPlaylistUniqueId(std::move(*ID));
            m_database.writeChangedPlaylists();
            return R"({"result": "ok"})";
        } else {
            logger(Level::warning) << "create playlist with name <" << value << "> failed, name is not new\n";
            return R"({"result": "playlist not new"})";
        }
    }

    if (parameter == ServerConstant::Command::getAlbumList) {
        auto list = m_database.searchPlaylistItems(value, Database::SearchAction::alike);

        std::sort(std::begin(list), std::end(list), [](Database::Playlist& item1, Database::Playlist& item2) { return item1.getUniqueID() > item2.getUniqueID(); });

        return convertToJson(list);
    }

    /* change to new playlist */
    if (parameter == ServerConstant::Command::change) {

        auto playlistList = m_database.searchPlaylistItems(value, Database::SearchAction::uniqueId);

        if (playlistList.size() != 1) {
            logger(Level::warning) << "playlist with uniqueId <" << value << "> not found\n";
            return R"({"result": "playlist not found"})";
        }

        auto blub = playlistList[0].getUniqueID();
        try {
        logger(Level::info) << "set current playlist to <" << boost::lexical_cast<std::string>(blub) << ">\n";
        } catch (std::exception& ex) {
            logger(LoggerFramework::Level::warning) << ex.what() << "\n";
        }

        m_player->stop();
        m_player->resetPlayer();
        m_database.setCurrentPlaylistUniqueId(playlistList[0].getUniqueID());

        if (m_player->setPlaylist(m_database.getAlbumPlaylistAndNames()))
            return R"({"result": "ok"})";
        else
            return R"({"result": "could not set playlist"})";
    }

    if (parameter == ServerConstant::Command::add) {
        auto currentPlaylist { m_database.getCurrentPlaylistUniqueID() };
        if ( currentPlaylist) {
            logger(Level::debug) << "add audio file with unique ID <" << value
                                 << "> to playlist <"
                                 << *currentPlaylist
                                 << ">\n";
            boost::uuids::uuid uniqueID;

            try {
                uniqueID = boost::lexical_cast<boost::uuids::uuid>( value );

            } catch (std::exception& ex) {
                logger(Level::warning) << "cannot read unique ID "<<value<<">: " << ex.what()<<"\n";
                return R"({"result": "cannot add <)" + value + "> to playlist <" + boost::uuids::to_string( *currentPlaylist ) + ">}";
            }

            if (m_database.addToPlaylistUID(*currentPlaylist, std::move(uniqueID))) {
                m_database.writeChangedPlaylists();
                logger(Level::debug) << "adding audio file <" << value
                                     << "> to playlist ID <" << boost::uuids::to_string(*currentPlaylist)
                                     << ">\n";
                return R"({"result": "ok"})";
            }
        }
        else {
            logger(Level::warning)<< "no current playlist to add audio file id <"<<value<<"> to\n";
        }
        return R"({"result": "cannot add <)" + value + "> to playlist <" + boost::uuids::to_string( *currentPlaylist ) + ">}";

    }

    if (parameter == ServerConstant::Command::show) {

        boost::uuids::uuid playlistName;

        if (!value.empty()) {
            try {
            playlistName = boost::lexical_cast<boost::uuids::uuid>(value);
            } catch (std::exception& ) {
                playlistName = boost::uuids::uuid();
            }
        }
        else if ( auto playlist = m_database.getCurrentPlaylistUniqueID() ) {
            playlistName = *playlist;
        }

        auto itemList = m_database.getIdListOfItemsInPlaylistId(playlistName);

        logger(Level::debug) << "show (" << itemList.size() << ") elements for playlist <" << boost::uuids::to_string(playlistName) << ">\n";
        return convertToJson(itemList);

    }

    /* returns all playlist names */
    if (parameter == ServerConstant::Command::showLists) {
        logger(Level::info) << "show all playlists\n";
        std::vector<std::pair<std::string, boost::uuids::uuid>> lists = m_database.getAllPlaylists();
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
                if (auto playlistRealname = m_database.convertPlaylist(*currentPlaylistUniqueID))
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

    return R"({"result": "cannot find parameter <)" + std::string(parameter) + "> in playlist\"}";

}
