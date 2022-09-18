#include "playlistaccess.h"
#include <nlohmann/json.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include "common/logger.h"
#include "common/FileType.h"
#include "database/NameType.h"

using namespace LoggerFramework;

std::string PlaylistAccess::convertToJson(const std::optional<boost::uuids::uuid> actualPlaylistUuid) {
    if (actualPlaylistUuid) {
        nlohmann::json entry;
        try {
            entry["current"] = boost::lexical_cast<std::string>(*actualPlaylistUuid);
        } catch (boost::bad_lexical_cast& ex) {
            boost::ignore_unused(ex);
            return R"({"current": ""})";
        }
        return entry.dump(2);
    }
    return R"({"current": ""})";
}

std::string PlaylistAccess::add(const std::string &value) {
    auto currentPlaylist { m_database.getDatabase()->getCurrentPlaylistUniqueID() };
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

        if (m_database.getDatabase()->addToPlaylistUID(*currentPlaylist, std::move(uniqueID))) {
            m_database.getDatabase()->writeChangedPlaylists();
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

std::string PlaylistAccess::create(const std::string &value) {
    auto ID = m_database.getDatabase()->createPlaylist(value, Database::Persistent::isPermanent);
    if (ID && !ID->is_nil()) {
        logger(Level::info) << "create playlist with name <" << value << "> "<<"-> "<<*ID<<" \n";
        m_database.getDatabase()->setCurrentPlaylistUniqueId(std::move(*ID));
        m_database.getDatabase()->writeChangedPlaylists();
        return R"({"result": "ok"})";
    } else {
        logger(Level::warning) << "create playlist with name <" << value << "> failed, name is not new\n";
        return R"({"result": "playlist not new"})";
    }
}

std::string PlaylistAccess::getAlbumList(const std::string &value) {
    auto list = m_database.getDatabase()->searchPlaylistItems(value, Database::SearchAction::alike);

    std::sort(std::begin(list), std::end(list), [](Database::Playlist& item1, Database::Playlist& item2) { return item1.getUniqueID() > item2.getUniqueID(); });

    return convertToJson(list);
}

std::string PlaylistAccess::getAlbumUid(const std::string &value) {
    auto list = m_database.getDatabase()->searchPlaylistItems(value, Database::SearchAction::uniqueId);

    return convertToJson(list);
}

std::string PlaylistAccess::change(const std::string &value) {

    auto playlistList = m_database.getDatabase()->searchPlaylistItems(value, Database::SearchAction::uniqueId);

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

    if (m_player.hasPlayer()) {
        m_player.stop();
        m_player.resetPlayer();
    }
    m_database.getDatabase()->setCurrentPlaylistUniqueId(playlistList[0].getUniqueID());

    if (!m_player.hasPlayer() || m_player.setPlaylist(m_database.getDatabase()->getAlbumPlaylistAndNames()))
        return R"({"result": "ok"})";
    else
        return R"({"result": "could not set playlist"})";

}

std::string PlaylistAccess::show(const std::string &value) {

    boost::uuids::uuid playlistName;

    if (!value.empty()) {
        try {
            playlistName = boost::lexical_cast<boost::uuids::uuid>(value);
        } catch (std::exception& ) {
            playlistName = boost::uuids::uuid();
        }
    }
    else if ( auto playlist = m_database.getDatabase()->getCurrentPlaylistUniqueID() ) {
        playlistName = *playlist;
    }

    auto itemList = m_database.getDatabase()->getIdListOfItemsInPlaylistId(playlistName);

    logger(Level::info) << "show (" << itemList.size() << ") elements for playlist <" << boost::uuids::to_string(playlistName) << ">\n";
    return convertToJson(itemList);

}

std::string PlaylistAccess::showLists(const std::string &value) {

    boost::ignore_unused(value);

    logger(Level::info) << "show all playlists\n";
    std::vector<std::pair<std::string, boost::uuids::uuid>> lists = m_database.getDatabase()->getAllPlaylists();
    if (!lists.empty()) {
        logger(Level::debug) << "playlists are \n";
        nlohmann::json json;
        nlohmann::json jsonList;
        try {
            for (auto item : lists) {
                logger(Level::debug) << item.first << " - " << item.second << "\n";
                nlohmann::json jentry;
                jentry[std::string(ServerConstant::JsonField::uid)] = boost::lexical_cast<std::string>(item.second);
                jentry[std::string(ServerConstant::JsonField::playlist)] = item.first;
                jsonList.push_back(jentry);
            }
            json[ServerConstant::JsonField::playlists] = jsonList;
            if (auto currentPlaylistUniqueID = m_database.getDatabase()->getCurrentPlaylistUniqueID()) {
                if (auto playlistRealname = m_database.getDatabase()->convertPlaylist(*currentPlaylistUniqueID))
                    json[ServerConstant::JsonField::currentPlaylist] = *playlistRealname;
                else
                    json[ServerConstant::JsonField::currentPlaylist] = std::string("");
            }
        } catch (std::exception& e) {
            logger(Level::warning) << "error: " << e.what() << "\n";
            return "[]";
        }
        return json.dump(2);
    } else {
        return "[]";
    }
}

std::string PlaylistAccess::getCurrentPlaylistUID(const std::string &value) {

    boost::ignore_unused(value);

    auto playlistUID = m_database.getDatabase()->getCurrentPlaylistUniqueID();
    return convertToJson(playlistUID);
}

std::string PlaylistAccess::convertToJson(const std::optional<std::vector<Id3Info>> list) {

    nlohmann::json json;

    try {
        if (list) {
            for(auto item : *list) {
                nlohmann::json jentry;
//                jentry[std::string(ServerConstant::Parameter::Database::uid)] = boost::uuids::to_string(item.uid);
//                jentry[std::string(ServerConstant::Parameter::Database::performer)] = item.performer_name;
//                jentry[std::string(ServerConstant::Parameter::Database::album)] = item.album_name;
//                jentry[std::string(ServerConstant::Parameter::Database::title)] = item.title_name;
//                jentry[std::string(ServerConstant::Parameter::Database::imageFile)] = item.urlCoverFile;
//                jentry[std::string(ServerConstant::Parameter::Database::trackNo)] = item.track_no;
//                jentry[std::string(ServerConstant::Parameter::Database::url)] = item.urlAudioFile;
                jentry[std::string(ServerConstant::JsonField::uid)] = boost::uuids::to_string(item.uid);
                jentry[std::string(ServerConstant::JsonField::performer)] = item.performer_name;
                jentry[std::string(ServerConstant::JsonField::album)] = item.album_name;
                jentry[std::string(ServerConstant::JsonField::title)] = item.title_name;
                jentry[std::string(ServerConstant::JsonField::cover)] = item.urlCoverFile;
                jentry[std::string(ServerConstant::JsonField::trackNo)] = item.track_no;
                jentry[std::string(ServerConstant::JsonField::url)] = item.urlAudioFile;
                json.push_back(jentry);
            }
        }
    } catch (const std::exception& ex) {
        logger(Level::error) << "conversion to json failed: " << ex.what();
        return "[]";
    }

    return json.dump(2);
}

std::string PlaylistAccess::convertToJson(const std::vector<Database::Playlist> list) {

    nlohmann::json json;

    try {
        for(auto item : list) {
            nlohmann::json jentry;
//            jentry[std::string(ServerConstant::Parameter::Database::uid)] = boost::uuids::to_string(item.getUniqueID());
//            jentry[std::string(ServerConstant::Parameter::Database::album)] = item.getName();
//            jentry[std::string(ServerConstant::Parameter::Database::performer)] = item.getPerformer();
//            jentry[std::string(ServerConstant::Parameter::Database::imageFile)] = item.getCover();
            jentry[std::string(ServerConstant::JsonField::uid)] = boost::uuids::to_string(item.getUniqueID());
            jentry[std::string(ServerConstant::JsonField::album)] = item.getName();
            jentry[std::string(ServerConstant::JsonField::performer)] = item.getPerformer();
            jentry[std::string(ServerConstant::JsonField::cover)] = item.getCover();
            json.push_back(jentry);
        }

    } catch (const nlohmann::json::exception& ex) {
        logger(Level::error) << "conversion to json failed: " << ex.what();
    }

    return json.dump(2);
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
        return create(value);
    }

    if (parameter == ServerConstant::Command::getAlbumList) {
        return getAlbumList(value);
    }

    if (parameter == ServerConstant::Command::getAlbumUid) {
        return getAlbumUid(value);
    }

    /* change to new playlist */
    if (parameter == ServerConstant::Command::change) {
        return change(value);
    }

    if (parameter == ServerConstant::Command::add) {
        return add(value);
    }

    if (parameter == ServerConstant::Command::show) {
        return show(value);
    }

    /* returns all playlist names */
    if (parameter == ServerConstant::Command::showLists) {
        return showLists(value);
    }

    if (parameter == ServerConstant::Command::currentPlaylistUID) {
        return getCurrentPlaylistUID(value);
    }

    return R"({"result": "cannot find parameter <)" + std::string(parameter) + "> in playlist\"}";

}
