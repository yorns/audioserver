#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <optional>
#include <snc/client.h>
#include <boost/uuid/uuid_io.hpp>

#include <boost/lexical_cast.hpp>
#include "nlohmann/json.hpp"

#include "common/mime_type.h"
#include "common/Extractor.h"
#include "common/logger.h"
#include "common/generalPlaylist.h"
#include "common/repeattimer.h"
#include "webserver/Listener.h"
#include "webserver/Session.h"
#include "webserver/databaseaccess.h"
#include "webserver/playlistaccess.h"
#include "webserver/playeraccess.h"
#include "webserver/wifiaccess.h"
#include "playerinterface/mpvplayer.h"
#include "playerinterface/gstplayer.h"
#include "database/SimpleDatabase.h"
#include "id3tagreader/id3TagReader.h"


using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>
using namespace LoggerFramework;

void printPaths() {

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

struct Config {
    std::string m_address;
    std::string m_port;
    std::string m_basePath;
    uint32_t m_amplify;

    bool m_enableCache;

    LoggerFramework::Level m_logLevel;
};

std::optional<Config> readConfig(std::string configFile) {
    Config config;

    std::ifstream ifs(configFile);
    nlohmann::json configData = nlohmann::json::parse(ifs);

    std::string debugLogLevel;

    try {
        config.m_address = configData["IpAddress"];
        config.m_port = configData["Port"];
        config.m_basePath = configData["BasePath"];
        config.m_enableCache = configData["EnableCache"];
        debugLogLevel = configData["LogLevel"];
        if (configData.find("Amplify") != configData.end()) {
            config.m_amplify = configData["Amplify"];
        }
        else {
            config.m_amplify = 1;
        }

        config.m_logLevel = LoggerFramework::Level::debug;

        if (debugLogLevel == "info") {
            config.m_logLevel = LoggerFramework::Level::info;
        }
        else if (debugLogLevel == "warning") {
            config.m_logLevel = LoggerFramework::Level::warning;
        }
        else if (debugLogLevel == "error") {
            config.m_logLevel = LoggerFramework::Level::error;
    }

    } catch (std::exception& ex) {
        std::cerr << "cannot read config <"<<ex.what()<<"\n";
        return std::nullopt;
    }

    return config;
}

int helpOutput(const char* command) {
    logger(Level::info) <<
        "Usage:" << command << " [config file name]\n" <<
        "Example:\n" <<
        "    "<< command <<" /my/path/config.json\n";
    return EXIT_FAILURE;
}

boost::uuids::uuid extractUUID (const std::string& uuidString) {
    boost::uuids::uuid extract;
    try {
        extract = boost::lexical_cast<boost::uuids::uuid>(uuidString);
    } catch (std::exception& ex) {
        logger(Level::warning) << "cannot extract id "<< uuidString <<": "<< ex.what() << "\n";
    }

    return extract;
}


int main(int argc, char* argv[])
{
    std::string configFileName {"/etc/audioserver.json"};
    // Check command line arguments.
    if (argc > 2)
    {
        return helpOutput(argv[0]);
    }
    else if ( argc == 2) {
        configFileName = argv[1];
    }

    auto config = readConfig(configFileName);

    if (!config) {
        std::cerr << "cannot read config file <" << configFileName << ">\n";
        return EXIT_FAILURE;
    }

    ServerConstant::base_path = ServerConstant::sv{ config->m_basePath };

    auto const address = boost::asio::ip::make_address(config->m_address);
    auto const port = boost::lexical_cast<unsigned short>(config->m_port.c_str());
    ServerConstant::base_path = std::string_view(config->m_basePath);
    logger(Level::info) << "setting base path to <"<<ServerConstant::base_path<<">\n";

    std::setlocale(LC_ALL, "de_DE.UTF-8");

    globalLevel = config->m_logLevel;

    printPaths();

    boost::asio::io_context ioc;

    logger(Level::info) << "connection client <audioserver> to broker\n";

    logger(Level::info) << "create player instance for";


    snc::Client sncClient("audioserver", ioc, "127.0.0.1", 12001);

    Database::SimpleDatabase database(config->m_enableCache);
    WifiManager wifiManager;

    SessionHandler sessionHandler;
    DatabaseAccess databaseWrapper(database);
    auto player = std::unique_ptr<BasePlayer>(new GstPlayer(ioc));
    PlaylistAccess playlistWrapper(database, player);
    WifiAccess wifiAccess(wifiManager);

    player->setAmplify(config->m_amplify);

    auto updateUI = [&sessionHandler, &sncClient, &player, &database] ( )
    {
        std::string title;
        std::string album;
        std::string performer;
        std::string cover = "/img/unknown.png";

        auto emptyUID = boost::uuids::uuid();
        boost::uuids::uuid songID { emptyUID };
        boost::uuids::uuid playlistID { emptyUID };
        boost::uuids::uuid currPlaylistID { emptyUID };

        if (auto _playlistID = database.getCurrentPlaylistUniqueID())
            playlistID = *_playlistID;

        currPlaylistID = player->getPlaylistID();

        if (player->isPlaying()) {

            songID = player->getSongID();
            auto songData = database.searchAudioItems(songID, Database::SearchItem::uid , Database::SearchAction::uniqueId);

            if (!player->getTitle().empty()) {
                title = player->getTitle();
            }
            else if (songData.size() > 0) {
                title = songData[0].title_name;
            }
            logger(LoggerFramework::Level::info) << "update title with <"<<title<<">\n";

            if (!player->getAlbum().empty()) {
                album = player->getAlbum();
            }
            else if (songData.size() > 0) {
                album = songData[0].album_name;
            }
            logger(LoggerFramework::Level::info) << "update album with <"<<album<<">\n";

            if (!player->getPerformer().empty()) {
                performer = player->getPerformer();
            }
            else if (songData.size() > 0) {
                performer = songData[0].performer_name;
            }
            logger(LoggerFramework::Level::info) << "update performer with <"<<performer<<">\n";


            if (songData.size() > 0) {
                cover = songData[0].urlCoverFile;
            }
            logger(LoggerFramework::Level::info) << "update cover with <"<<cover<<">\n";


        } else {
            auto playlistData = database.searchPlaylistItems(playlistID);

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
        songInfo["position"] = player->getSongPercentage();
        songInfo["loop"] = player->getLoop();
        songInfo["shuffle"] = player->getShuffle();
        songInfo["playing"] = player->isPlaying();
        songInfo["paused"] = player->isPause();
        songInfo["volume"] = player->getVolume();
        songInfo["single"] = player->isSingle();
        songInfo["title"] = title;
        songInfo["album"] = album;
        songInfo["performer"] = performer;
        songInfo["cover"] = cover;
        songBroadcast["SongBroadcastMessage"] = songInfo;

        sessionHandler.broadcast(songBroadcast.dump());
        sncClient.send(snc::Client::SendType::cl_broadcast, "", songBroadcast.dump());

    };

    auto externalSelect = [&database, &player, &sessionHandler, &sncClient](const std::string& other, const std::string& raw_msg) {
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

                        const auto& albumPlaylists = database.searchPlaylistItems(albumName);

                        if (albumPlaylists.size() == 1) {
                            logger(Level::info) << "album found\n";
                            const std::string& title_low = Common::str_tolower(titleName);
                            for (const auto& elem : database.getIdListOfItemsInPlaylistId(albumPlaylists[0].getUniqueID())) {

                                if (elem.getNormalizedTitle() == title_low) {
                                    logger(Level::info) << "External request found for <"<<albumName<<":"<<titleName<<"> - " << albumPlaylists[0].getUniqueID() << " " << elem.uid <<"\n";

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
                      database.setCurrentPlaylistUniqueId(std::move(albumId));
                      position = data.at("position");

                      player->setPlaylist(database.getAlbumPlaylistAndNames());
                      player->startPlay(titleId, position);
                    }
                    else {
                        logger(Level::warning) << "could not set album and/or title\n";
                    }
                }
                if (command == "stop") {
                    logger(Level::info) << "stop play\n";
                    player->stop();
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

                const auto& albumPlaylists = database.searchPlaylistItems(albumName);

                if (albumPlaylists.size() == 1) {
                    const std::string& title_low = Common::str_tolower(titleName);
                    for (const auto& elem : database.getIdListOfItemsInPlaylistId(albumPlaylists[0].getUniqueID())) {

                        if (elem.getNormalizedTitle() == title_low) {
                            logger(Level::info) << "External request found for <"<<albumName<<":"<<titleName<<"> - " << albumPlaylists[0].getUniqueID() << " " << elem.uid <<"\n";
                            nlohmann::json msg;
                            nlohmann::json reply;
                            reply["albumID"] = boost::uuids::to_string(albumPlaylists[0].getUniqueID());
                            reply["titleID"] = boost::uuids::to_string(elem.uid);
                            msg["reply"] = reply;
                            sncClient.send(snc::Client::SendType::cl_send, other, msg.dump());
                        }
                    }
                }
            }

        } catch (std::exception& exp) {
            logger(Level::warning) << "Cannot parse message (Error is > " << exp.what() << "):\n " << raw_msg << "\n";
        }
    };

    sncClient.recvHandler([externalSelect](const std::string& other, const std::string& raw_msg) {
        logger(Level::info) << "received Message from <" << other << ">: "<<raw_msg<<"\n";
        externalSelect(other, raw_msg);
    });

    player->setSongEndCB([&database, &updateUI](const boost::uuids::uuid& songID){
        boost::ignore_unused(songID);
        auto currentPlaylistUniqueIDOptional = database.getCurrentPlaylistUniqueID();
        boost::uuids::uuid currentPlaylistUniqueID;
        if (currentPlaylistUniqueIDOptional)
            currentPlaylistUniqueID = *currentPlaylistUniqueIDOptional;
        updateUI();
        logger(Level::info) << "end handler called for current song\n";
    });

    PlayerAccess playerWrapper(player, [&database]() -> Common::AlbumPlaylistAndNames {
                                   logger(Level::debug) << "Request to get playlist and playlist names\n";
                                   return database.getAlbumPlaylistAndNames();
    });

    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::database, http::verb::get, PathCompare::exact,
                                 [&databaseWrapper](const http::request_parser<http::string_body>& request) -> std::string {
        auto url = utility::Extractor::getUrlInformation(request.get().target(), ServerConstant::AccessPoints::database);
        return databaseWrapper.access(url);
    });

    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::playlist, http::verb::get, PathCompare::exact,
                                 [&playlistWrapper](const http::request_parser<http::string_body>& request) -> std::string {
        auto url = utility::Extractor::getUrlInformation(request.get().target(), ServerConstant::AccessPoints::playlist);
        if (!url)
            logger(Level::debug) << "url not set correctly\n";
        return playlistWrapper.access(url);
    });

    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::player, http::verb::post, PathCompare::exact,
                                 [&playerWrapper](const http::request_parser<http::string_body>& request) -> std::string {
        auto url = utility::Extractor::getUrlInformation(std::string(request.get().target()), ServerConstant::AccessPoints::player);
        return playerWrapper.access(url);
    });

    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::wifi, http::verb::post, PathCompare::exact,
                                 [&wifiAccess](const http::request_parser<http::string_body>& request) -> std::string {
        auto url = utility::Extractor::getUrlInformation(std::string(request.get().target()), ServerConstant::AccessPoints::wifi);
        return wifiAccess.access(url);
    });

    /*
    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::Virtual::image, http::verb::get, PathCompare::prefix,
                                 [&databaseWrapper](const http::request_parser<http::string_body>& request) -> std::string {
        // handle virtual image requests
        auto url = utility::Extractor::getUrlInformation(std::string(request.get().target()));
        return databaseWrapper.access(url);        
    });
    */
    
    sessionHandler.addVirtualImageHandler([&databaseWrapper, &database](const std::string_view& _target) -> std::optional<std::vector<char>> {
        // split target
        auto target = utility::urlConvert(std::string(_target));
        logger(Level::debug) << "test for virtual image request for <"<<target<<">\n";

        if (target.substr(0,5) == "/img/") {
            if (auto virtualFile = databaseWrapper.getVirtualFile(target)) {
               return virtualFile;
            } else {
                boost::uuids::uuid unknownCover;
                try {
                    unknownCover = boost::lexical_cast<boost::uuids::uuid>(ServerConstant::unknownCoverFileUid);
                } catch(std::exception& ex) {
                    logger(Level::error) << " ERROR: cannot convert unknown cover uuid string: " << ex.what() << "\n";
                }
               return database.getCover(unknownCover);
            }
        }
        return std::nullopt;
    });

    sessionHandler.addVirtualAudioHandler([&database](const std::string_view& _target) -> std::optional<std::string> {
        auto target = utility::urlConvert(std::string(_target));
        logger(Level::debug) << "test virtual audio request for <"<<target<<">\n";

        if (target.substr(0,7) == "/audio/") {
            auto pos1 = target.find_last_of("/");
            if (pos1 != std::string_view::npos) {
                auto pos2 = target.find_last_of(".");
                { //if (pos2 != std::string_view::npos) {
                    std::string uidStr = std::string(target.substr(pos1+1, pos2-pos1-1));
                    logger(LoggerFramework::Level::debug) << "searching for audio UID <"<<uidStr<<">\n";
                    boost::uuids::uuid uid;
                    try {
                        uid = boost::lexical_cast<boost::uuids::uuid>(uidStr);
                    } catch(std::exception& ex) {
                        logger(Level::warning) << "could not interpret <"<<uidStr<<">: "<< ex.what()<<"\n";
                        return std::nullopt;
                    }

                    return database.getFileFromUUID(uid);
                }
            }
        }
        return std::nullopt;
    });

    sessionHandler.addVirtualPlaylistHandler([&database](const std::string_view& _target) -> std::optional<std::string> {
        auto target = utility::urlConvert(std::string(_target));
        logger(Level::debug) << "virtual playlist request for <"<<target<<">\n";

        if (target.substr(0,4) == "/pl/") {
            auto pos1 = target.find_last_of("/");
            if (pos1 != std::string_view::npos) {
                auto pos2 = target.find_last_of(".");
                if (pos2 != std::string_view::npos) {
                    std::string uidStr = std::string(target.substr(pos1+1, pos2-pos1-1));
                    logger(LoggerFramework::Level::debug) << "searching for playlist UID <"<<uidStr<<">\n";
                    boost::uuids::uuid uid;
                    try {
                        uid = boost::lexical_cast<boost::uuids::uuid>(uidStr);
                    } catch(std::exception& ex) {
                        logger(Level::warning) << "could not interpret <"<<uidStr<<">: "<< ex.what()<<"\n";
                        return std::nullopt;
                    }
                    return database.getM3UPlaylistFromUUID(uid);
                }
            }
        }
        return std::nullopt;
    });



    auto generateName = []() -> Common::NameGenerator::GenerationName
    {
        auto name = Common::NameGenerator::create(Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::AudioMp3) , ".mp3");
        logger(Level::debug) << "generating file name: " << name.filename << "\n";
        return name;
    };

    auto uploadFinishHandler = [&database](const Common::NameGenerator::GenerationName& name)-> bool
    {
        Common::FileNameType file;
        file.dir = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::AudioMp3);
        file.name = name.unique_id;
        file.extension = std::string(ServerConstant::mp3Extension);
        return database.addNewAudioFileUniqueId(file);
    };

    sessionHandler.addUploadHandler(ServerConstant::AccessPoints::upload,
                                    generateName,
                                    uploadFinishHandler);



    RepeatTimer websocketSonginfoSenderTimer(ioc, 500ms ); //std::chrono::milliseconds(500));

    // create a timer service to request actual player information and send them to the session handlers
    websocketSonginfoSenderTimer.setHandler([&player, &updateUI](){
        if (player) {
            updateUI();
        }
    });

    websocketSonginfoSenderTimer.start();

    auto sessionCreator = [&sessionHandler](tcp::socket&& socket) {
        static uint32_t sessionId { 0 };
        logger(Level::debug) << "--- new session created with ID <" << sessionId << "> ---\n";
        std::string localHtmlDir { Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Html) };
        std::make_shared<Session>(std::move(socket), sessionHandler, std::move(localHtmlDir), sessionId++)->start();
    };


    logger(Level::info) << "shared Listener creation and run\n";
    std::make_shared<Listener>(
                ioc,
                tcp::endpoint{address, port},
                sessionCreator)->run();


    logger(Level::info) << "Rest Accesspoints:\n" << sessionHandler.generateRESTInterfaceDocumentation();

    database.loadDatabase();

    logger(Level::info) << "server started\n";

    ioc.run();

    logger(Level::info) << "ioc finished\n";

    return EXIT_SUCCESS;
}
