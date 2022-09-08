#include "misc.h"

#include <boost/uuid/uuid.hpp>
#include <boost/lexical_cast.hpp>

#include "webserver/playeraccess.h"

void Common::printPaths() {

    // create all strings for the directories used in initialisation
    std::stringstream mp3Dir;
    std::stringstream coverDir;
    std::stringstream playlistDir;
    std::stringstream playerLogDir;
    std::stringstream htmlDir;

    mp3Dir << ServerConstant::base_path << "/" << ServerConstant::audioPathMp3;
    coverDir << ServerConstant::base_path << "/" << ServerConstant::coverPath;
    playlistDir << ServerConstant::base_path << "/" << ServerConstant::playlistPathM3u;
    htmlDir << ServerConstant::base_path << "/" << ServerConstant::htmlPath;

    logger(Level::info) << "path: \n";
    logger(Level::info) << " audio path    : " << mp3Dir.str() << "\n";
    logger(Level::info) << " cover path    : " << coverDir.str() << "\n";
    logger(Level::info) << " playlist path : " << playlistDir.str() << "\n";
    logger(Level::info) << " html path     : " << htmlDir.str() << "\n";

}

boost::uuids::uuid Common::extractUUID(const std::string &uuidString) {
    boost::uuids::uuid extract;
    try {
        extract = boost::lexical_cast<boost::uuids::uuid>(uuidString);
    } catch (std::exception& ex) {
        logger(Level::warning) << "cannot extract id "<< uuidString <<": "<< ex.what() << "\n";
    }

    return extract;
}


void Common::audioserver_updateUI(SessionHandler &sessionHandler,
                                  std::shared_ptr<snc::Client> sncClient,
                                  PlayerAccess& playerWrapper,
                                  DatabaseAccess &databaseWrapper)
{
    std::string title;
    std::string album;
    std::string performer;
    std::string cover = "img/unknown.png";

    auto emptyUID = boost::uuids::uuid();
    boost::uuids::uuid songID { emptyUID };
    boost::uuids::uuid playlistID { emptyUID };
    boost::uuids::uuid currPlaylistID { emptyUID };

    // do not send player information, when there is no player
    if (!playerWrapper.hasPlayer())
        return;

    if (auto _playlistID = databaseWrapper.getDatabase()->getCurrentPlaylistUniqueID())
        playlistID = *_playlistID;

    currPlaylistID = playerWrapper.getPlaylistID();

    if (playerWrapper.isPlaying()) {

        songID = playerWrapper.getSongID();
        auto songData = databaseWrapper.getDatabase()->searchAudioItems(songID, Database::SearchItem::uid , Database::SearchAction::uniqueId);

        if (!playerWrapper.getTitle().empty()) {
            title = playerWrapper.getTitle();
        }
        else if (songData.size() > 0) {
            title = songData[0].title_name;
        }
        logger(LoggerFramework::Level::info) << "update title with <"<<title<<">\n";

        if (!playerWrapper.getAlbum().empty()) {
            album = playerWrapper.getAlbum();
        }
        else if (songData.size() > 0) {
            album = songData[0].album_name;
        }
        logger(LoggerFramework::Level::info) << "update album with <"<<album<<">\n";

        if (!playerWrapper.getPerformer().empty()) {
            performer = playerWrapper.getPerformer();
        }
        else if (songData.size() > 0) {
            performer = songData[0].performer_name;
        }
        logger(LoggerFramework::Level::info) << "update performer with <"<<performer<<">\n";


        if (songData.size() > 0 && !songData[0].urlCoverFile.empty()) {
            cover = songData[0].urlCoverFile;
        } else {
            logger(LoggerFramework::Level::info) << "no cover set\n";
        }
        logger(LoggerFramework::Level::info) << "update cover with <"<<cover<<">\n";


    } else {
        auto playlistData = databaseWrapper.getDatabase()->searchPlaylistItems(playlistID);

        if (playlistData.size() > 0) {
            album = playlistData[0].getName();
            performer = playlistData[0].getPerformer();

            cover = playlistData[0].getCover();
        }
    }

    nlohmann::json songBroadcast;
    nlohmann::json songInfo;
    try {
        songInfo["songID"] = boost::lexical_cast<std::string>(songID);
        songInfo["playlistID"] = boost::lexical_cast<std::string>(playlistID);
        songInfo["curPlaylistID"] = boost::lexical_cast<std::string>(currPlaylistID);
        songInfo["song"] = title;
        songInfo["playlist"] = album;
    } catch (std::exception& ) {
        logger(Level::warning) << "cannot convert ID to string - sending empty information\n";
        songInfo["songID"] = boost::lexical_cast<std::string>(emptyUID);
        songInfo["playlistID"] = boost::lexical_cast<std::string>(emptyUID);
        songInfo["curPlaylistID"] = boost::lexical_cast<std::string>(emptyUID);
        songInfo["song"] = "";
        songInfo["playlist"] = "";
    }
    songInfo["position"] = playerWrapper.getSongPercentage();
    songInfo["loop"] = playerWrapper.getLoop();
    songInfo["shuffle"] = playerWrapper.getShuffle();
    songInfo["playing"] = playerWrapper.isPlaying();
    songInfo["paused"] = playerWrapper.isPause();
    songInfo["volume"] = playerWrapper.getVolume();
    songInfo["single"] = playerWrapper.isSingle();
    songInfo["title"] = title;
    songInfo["album"] = album;
    songInfo["performer"] = performer;
    songInfo["cover"] = cover;
    songBroadcast["SongBroadcastMessage"] = songInfo;

    sessionHandler.broadcast(songBroadcast.dump());

    if (sncClient)
        sncClient->send(snc::Client::SendType::cl_broadcast, "", songBroadcast.dump());

}

void Common::audioserver_externalSelect(DatabaseAccess& databaseWrapper,
                                        PlayerAccess &playerWrapper,
                                        SessionHandler &sessionHandler,
                                        std::shared_ptr<snc::Client> sncClient,
                                        const std::string &other,
                                        const std::string &raw_msg)
{
    logger(Level::debug) << "received Message from <" << other << ">: "<<raw_msg<<"\n";

    try {
        nlohmann::json msg = nlohmann::json::parse(raw_msg);
        if (msg.find("CmdMsg") != msg.end()) {
            auto command = msg.at("CmdMsg");
            if (command == "start" && msg.find("data") != msg.end()) {
                auto data = msg.at("data");
                boost::uuids::uuid albumId{0}, titleId{0};
                uint32_t position {0};
                if (data.find("albumID") != data.end() && data.find("titleID") != data.end()) {
                    albumId = extractUUID(std::string(data.at("albumID")));
                    titleId = extractUUID(std::string(data.at("titleID")));
                    logger(Level::info) << "received album <" << albumId << "> and title <" << titleId << ">\n";
                }
                else if (data.find("album") != data.end() && data.find("title") != data.end()) {

                    const auto& albumName = std::string(data.at("album"));
                    const auto& titleName = std::string(data.at("title"));

                    logger(Level::info) << "received album <" << albumName << "> and title <" << titleName << ">\n";

                    const auto& albumPlaylists = databaseWrapper.getDatabase()->searchPlaylistItems(albumName);

                    if (albumPlaylists.size() == 1) {
                        logger(Level::info) << "album found\n";
                        const std::string& title_low = Common::str_tolower(titleName);
                        for (const auto& elem : databaseWrapper.getDatabase()->getIdListOfItemsInPlaylistId(albumPlaylists[0].getUniqueID())) {

                            if (elem.getNormalizedTitle() == title_low) {
                                logger(Level::info) << "External request found for <" << albumName << ":" << titleName
                                                    << "> - " << albumPlaylists[0].getUniqueID() << " " << elem.uid << "\n";

                                albumId = albumPlaylists[0].getUniqueID();
                                titleId = elem.uid;
                            }
                        }
                    }
                }
                else {
                    std::cout << "no infos found to handle start of album/title pair. (not given?)";
                }

                if (!albumId.is_nil() && !titleId.is_nil() ) {
                    databaseWrapper.getDatabase()->setCurrentPlaylistUniqueId(std::move(albumId));
                    position = data.at("position");

                    if (playerWrapper.hasPlayer()) {
                        playerWrapper.setPlaylist(databaseWrapper.getDatabase()->getAlbumPlaylistAndNames());
                        playerWrapper.startPlay(titleId, position);
                    }
                }
                else {
                    logger(Level::warning) << "could not set album and/or title\n";
                }
            }
            if (command == "stop") {
                logger(Level::info) << "stop play\n";
                if (playerWrapper.hasPlayer())
                    playerWrapper.stop();
            }
        }
        if (msg.find("SsidMessage") != msg.end()) {
            sessionHandler.broadcast(msg.dump());

        }
        if (msg.find("Request") != msg.end()) {
            logger(Level::info) << "request for album and song id\n";
            auto request = msg.at("Request");
            const auto& albumName = std::string(request.at("album"));
            const auto& titleName = std::string(request.at("title"));

            const auto& albumPlaylists = databaseWrapper.getDatabase()->searchPlaylistItems(albumName);

            if (albumPlaylists.size() == 1) {
                const std::string& title_low = Common::str_tolower(titleName);
                for (const auto& elem : databaseWrapper.getDatabase()->getIdListOfItemsInPlaylistId(albumPlaylists[0].getUniqueID())) {

                    if (elem.getNormalizedTitle() == title_low) {
                        logger(Level::info) << "External request found for <"<<albumName<<":"<<titleName<<"> - "
                                            << albumPlaylists[0].getUniqueID() << " " << elem.uid <<"\n";
                        nlohmann::json msg;
                        nlohmann::json reply;
                        reply["albumID"] = boost::uuids::to_string(albumPlaylists[0].getUniqueID());
                        reply["titleID"] = boost::uuids::to_string(elem.uid);
                        msg["reply"] = reply;
                        if (sncClient)
                            sncClient->send(snc::Client::SendType::cl_send, other, msg.dump());
                    }
                }
            }
        }

    } catch (std::exception& exp) {
        logger(Level::warning) << "Cannot parse message (Error is > " << exp.what() << "):\n " << raw_msg << "\n";
    }

}
