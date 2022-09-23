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
    std::string cover = ServerConstant::unknownCoverUrl;

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
        logger(LoggerFramework::Level::info) << "update songID with <"<<boost::lexical_cast<std::string>(songID)<<">\n";

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
        songInfo[ServerConstant::JsonField::Websocket::songId] = boost::lexical_cast<std::string>(songID);
        songInfo[ServerConstant::JsonField::Websocket::playlistId] = boost::lexical_cast<std::string>(playlistID);
        songInfo[ServerConstant::JsonField::Websocket::curPlaylistId] = boost::lexical_cast<std::string>(currPlaylistID);
        songInfo[ServerConstant::JsonField::Websocket::playlist] = album;
    } catch (std::exception& ) {
        logger(Level::warning) << "cannot convert ID to string - sending empty information\n";
        songInfo[ServerConstant::JsonField::Websocket::songId] = boost::lexical_cast<std::string>(emptyUID);
        songInfo[ServerConstant::JsonField::Websocket::playlistId] = boost::lexical_cast<std::string>(emptyUID);
        songInfo[ServerConstant::JsonField::Websocket::curPlaylistId] = boost::lexical_cast<std::string>(emptyUID);
        songInfo[ServerConstant::JsonField::Websocket::playlist] = "";
    }

    songInfo[ServerConstant::JsonField::Websocket::position] = playerWrapper.getSongPercentage();
    songInfo[ServerConstant::JsonField::Websocket::loop] = playerWrapper.getLoop();
    songInfo[ServerConstant::JsonField::Websocket::shuffle] = playerWrapper.getShuffle();
    songInfo[ServerConstant::JsonField::Websocket::playing] = playerWrapper.isPlaying();
    songInfo[ServerConstant::JsonField::Websocket::paused] = playerWrapper.isPause();
    songInfo[ServerConstant::JsonField::Websocket::volume] = playerWrapper.getVolume();
    songInfo[ServerConstant::JsonField::Websocket::single] = playerWrapper.isSingle();
    songInfo[ServerConstant::JsonField::Websocket::title] = title;
    songInfo[ServerConstant::JsonField::Websocket::album] = album;
    songInfo[ServerConstant::JsonField::Websocket::performer] = performer;
    songInfo[ServerConstant::JsonField::Websocket::cover] = cover;

    songBroadcast[ServerConstant::SNC::songBroadcastMessage] = songInfo;

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
        if (msg.find(ServerConstant::SNC::commandMessage) != msg.end()) {
            auto command = msg.at(ServerConstant::SNC::commandMessage);
            if (command == ServerConstant::SNC::commandStart && msg.find(ServerConstant::SNC::dataMessage) != msg.end()) {
                auto data = msg.at(ServerConstant::SNC::dataMessage);
                boost::uuids::uuid albumId{0}, titleId{0};
                uint32_t position {0};
                if (data.find(ServerConstant::SNC::albumId) != data.end() &&
                        data.find(ServerConstant::SNC::titleId) != data.end()) {
                    albumId = extractUUID(std::string(data.at(ServerConstant::SNC::albumId)));
                    titleId = extractUUID(std::string(data.at(ServerConstant::SNC::titleId)));
                    logger(Level::info) << "received album <" << albumId << "> and title <" << titleId << ">\n";
                }
                else if (data.find(ServerConstant::SNC::album) != data.end() &&
                         data.find(ServerConstant::SNC::title) != data.end()) {

                    const auto& albumName = std::string(data.at(ServerConstant::SNC::album));
                    const auto& titleName = std::string(data.at(ServerConstant::SNC::title));

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
                    position = data.at(ServerConstant::SNC::position);

                    if (playerWrapper.hasPlayer()) {
                        playerWrapper.setPlaylist(databaseWrapper.getDatabase()->getAlbumPlaylistAndNames());
                        playerWrapper.startPlay(titleId, position);
                    }
                }
                else {
                    logger(Level::warning) << "could not set album and/or title\n";
                }
            }
            if (command == ServerConstant::SNC::commandStop) {
                logger(Level::info) << "stop play\n";
                if (playerWrapper.hasPlayer())
                    playerWrapper.stop();
            }
        }
        if (msg.find(ServerConstant::SNC::ssidMessage) != msg.end()) {
            sessionHandler.broadcast(msg.dump());

        }
        if (msg.find(ServerConstant::SNC::commandRequest) != msg.end()) {

            logger(Level::info) << "request for album and song id\n";
            auto request = msg.at(ServerConstant::SNC::commandRequest);

            const auto& albumName = std::string(request.at(ServerConstant::SNC::album));
            const auto& titleName = std::string(request.at(ServerConstant::SNC::title));

            const auto& albumPlaylists = databaseWrapper.getDatabase()->searchPlaylistItems(albumName);

            if (albumPlaylists.size() == 1) {
                const std::string& title_low = Common::str_tolower(titleName);
                for (const auto& elem : databaseWrapper.getDatabase()->getIdListOfItemsInPlaylistId(albumPlaylists[0].getUniqueID())) {

                    if (elem.getNormalizedTitle() == title_low) {
                        logger(Level::info) << "External request found for <"<<albumName<<":"<<titleName<<"> - "
                                            << albumPlaylists[0].getUniqueID() << " " << elem.uid <<"\n";
                        nlohmann::json msg;
                        nlohmann::json reply;
                        reply[ServerConstant::SNC::albumId] = boost::uuids::to_string(albumPlaylists[0].getUniqueID());
                        reply[ServerConstant::SNC::titleId] = boost::uuids::to_string(elem.uid);
                        msg[ServerConstant::SNC::CommandReply] = reply;
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
