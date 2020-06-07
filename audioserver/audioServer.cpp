#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <optional>
#include <snc/client.h>

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
    snc::Client client("audioserver", ioc, "127.0.0.1", 12001);

    logger(Level::info) << "create player instance for";

    auto player = std::unique_ptr<BasePlayer>(new GstPlayer(ioc));

    Database::SimpleDatabase database(config->m_enableCache);

    SessionHandler sessionHandler;
    DatabaseAccess databaseWrapper(database);
    PlaylistAccess playlistWrapper(database);

    auto updateUI = [&sessionHandler]( const std::string& songID, const std::string& playlistID, const std::string& currPlaylistID,
                  int position, bool doLoop, bool doShuffle, bool playing, double volume) {

        //logger(Level::debug) << "send updateUID json with with songID: <"<<songID<<"> playlistID: <"<<playlistID<<">\n";

        nlohmann::json songBroadcast;
        nlohmann::json songInfo;
        songInfo["songID"] = songID;
        songInfo["playlistID"] = playlistID;
        songInfo["curPlaylistID"] = currPlaylistID;
        songInfo["position"] = position;
        songInfo["loop"] = doLoop;
        songInfo["shuffle"] = doShuffle;
        songInfo["playing"] = playing;
        songInfo["volume"] = volume;
        songBroadcast["SongBroadcastMessage"] = songInfo;
        sessionHandler.broadcast(songBroadcast.dump());

    };

    auto externalSelect = [&database, &player](const std::string& raw_msg) {
        try {
        nlohmann::json msg = nlohmann::json::parse(raw_msg);
      database.setCurrentPlaylistUniqueId(msg.at("Album"));
      auto albumPlaylistAndNames = database.getAlbumPlaylistAndNames();
      player->startPlay(albumPlaylistAndNames,msg.at("Title"));
        } catch (std::exception& exp) {
            logger(Level::warning) << "Cannot parse message (Error is > " << exp.what() << "):\n " << raw_msg << "\n";
        }
    };

    client.recvHandler([externalSelect](const std::string& other, const std::string& raw_msg) {
        logger(Level::info) << "received Message from <" << other << ">\n";
        externalSelect(raw_msg);
    });

    //player->onUiChange(updateUI);

    player->setSongEndCB([&player, &updateUI](const std::string& songID){
        boost::ignore_unused(songID);
        updateUI("", player->getPlaylistID(), player->getPlaylistID(), 0, player->getLoop(), player->getShuffle(), false, player->getVolume());
        logger(Level::info) << "end handler called for current song\n";
    });

    PlayerAccess playerWrapper(player, [&database]() -> Common::AlbumPlaylistAndNames {
                                   logger(Level::debug) << "Request to get playlist and playlist names\n";
                                   return database.getAlbumPlaylistAndNames();
    });

    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::database, http::verb::get, PathCompare::exact,
                                 [&databaseWrapper](const http::request_parser<http::string_body>& request) -> std::string {
        auto url = utility::Extractor::getUrlInformation(std::string(request.get().target()));
        return databaseWrapper.access(url);
    });

    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::playlist, http::verb::get, PathCompare::exact,
                                 [&playlistWrapper](const http::request_parser<http::string_body>& request) -> std::string {
        auto url = utility::Extractor::getUrlInformation(std::string(request.get().target()));
        if (!url)
            logger(Level::debug) << "url not set correctly\n";
        return playlistWrapper.access(url);
    });

    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::player, http::verb::post, PathCompare::exact,
                                 [&playerWrapper](const http::request_parser<http::string_body>& request) -> std::string {
        auto url = utility::Extractor::getUrlInformation(std::string(request.get().target()));
        return playerWrapper.access(url);
    });

    sessionHandler.addVirtualFileHandler([&databaseWrapper](const std::string_view& _target) -> std::optional<std::vector<char>> {
        // split target
        auto target = utility::urlConvert(std::string(_target));
        logger(Level::debug) << "virtual file request for <"<<target<<">\n";

        if (target.substr(0,5) == "/img/") {
            return databaseWrapper.getVirtualFile(target);
        }
//        else
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
        file.name = name.unique_id;
        file.extension = std::string(ServerConstant::mp3Extension);
        return database.addNewAudioFileUniqueId(file);
    };

    sessionHandler.addUploadHandler(ServerConstant::AccessPoints::upload,
                                    generateName,
                                    uploadFinishHandler);



    RepeatTimer websocketSonginfoSenderTimer(ioc, std::chrono::milliseconds(500));

    // create a timer service to request actual player information and send them to the session handlers
    websocketSonginfoSenderTimer.setHandler( [&player, &database, &updateUI](){

        if (player) {

            std::string songID;
            std::string playlistID;
            std::string currPlaylistID;

            int timePercent { 0 };

            if (auto _playlistID = database.getCurrentPlaylistUniqueID())
                playlistID = *_playlistID;

            bool loop = player->getLoop();
            bool shuffle = player->getShuffle();

            if (player->isPlaying()) {
                songID = player->getSongID();
                currPlaylistID = player->getPlaylistID();
                timePercent = player->getSongPercentage()/100;
            }

            updateUI(songID, playlistID, currPlaylistID, timePercent, loop, shuffle, player->isPlaying(), player->getVolume());
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
