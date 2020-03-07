#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <boost/filesystem.hpp>

#include "nlohmann/json.hpp"

#include "common/mime_type.h"
#include "common/Extractor.h"
#include "common/logger.h"
#include "common/repeattimer.h"
#include "webserver/Listener.h"
#include "webserver/Session.h"
#include "webserver/databaseaccess.h"
#include "webserver/playlistaccess.h"
#include "webserver/playeraccess.h"
#include "playerinterface/mpvplayer.h"
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

    mp3Dir << ServerConstant::base_path << "/" << ServerConstant::audioPath;
    coverDir << ServerConstant::base_path << "/" << ServerConstant::coverPath;
    playlistDir << ServerConstant::base_path << "/" << ServerConstant::playlistPath;
    htmlDir << ServerConstant::base_path << "/" << ServerConstant::htmlPath;

    logger(Level::info) << "path: \n";
    logger(Level::info) << " audio path    : " << mp3Dir.str() << "\n";
    logger(Level::info) << " cover path    : " << coverDir.str() << "\n";
    logger(Level::info) << " playlist path : " << playlistDir.str() << "\n";
    logger(Level::info) << " html path     : " << htmlDir.str() << "\n";

}

int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 4 && argc != 3)
    {
        logger(Level::info) <<
            "Usage:" << argv[0] << " <address> <port> [doc_root]\n" <<
            "Example:\n" <<
            "    "<<argv[0]<<" 0.0.0.0 8080 .\n";
        return EXIT_FAILURE;
    }

    ServerConstant::base_path = ServerConstant::sv{"/var/audioserver"};

    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    if (argc == 4)  {
        ServerConstant::base_path = std::string_view(argv[3]);
        logger(Level::info) << "setting base path to <"<<ServerConstant::base_path<<">\n";
    }

    globalLevel = Level::debug;

    printPaths();

    boost::asio::io_context ioc;

    Database::SimpleDatabase database;
    database.loadDatabase();

    logger(Level::info) << "create player instance\n";
    auto player = std::unique_ptr<BasePlayer>(new MpvPlayer(ioc));

    SessionHandler sessionHandler;
    DatabaseAccess databaseWrapper(database);
    PlaylistAccess playlistWrapper(database);

    PlayerAccess playerWrapper(player, [&database]() -> std::tuple<const std::vector<std::string>, const std::string, const std::string>{
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
        return playlistWrapper.access(url);
    });
    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::player, http::verb::post, PathCompare::exact,
                                 [&playerWrapper](const http::request_parser<http::string_body>& request) -> std::string {
        auto url = utility::Extractor::getUrlInformation(std::string(request.get().target()));
        return playerWrapper.access(url);
    });

    auto generateName = []() -> Common::NameGenerator::GenerationName
    {
        auto name = Common::NameGenerator::create(Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Audio) , ".mp3");
        logger(Level::debug) << "generating file name: " << name.filename << "\n";
        return name;
    };

    auto uploadFinishHandler = [&database](const Common::NameGenerator::GenerationName& name)-> bool
    {
        return database.addNewAudioFileUniqueId(name.unique_id);
    };

    sessionHandler.addUploadHandler(ServerConstant::AccessPoints::upload,
                                    generateName,
                                    uploadFinishHandler);

    auto sessionCreator = [&sessionHandler](tcp::socket& socket) {
        logger(Level::debug) << "--- new session created ---\n";
        static uint32_t sessionId { 0 };
        std::string localHtmlDir{ Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Html) };
        std::make_shared<Session>(std::move(socket), sessionHandler, std::move(localHtmlDir), sessionId++)->start();
    };

    RepeatTimer websocketSonginfoSenderTimer(ioc, std::chrono::milliseconds(500));

    // create a timer service to request actual song information and send them to the session handlers
    // gues together audio player and webservers websocket
    websocketSonginfoSenderTimer.setHandler( [&player, &sessionHandler](){
      if (player && player->isPlaying()) {
          std::string songID = player->getSongID();
          uint32_t timePercent = player->getSongPercentage();
          nlohmann::json songBroadcast;
          nlohmann::json songInfo;
          songInfo["songID"] = songID;
          songInfo["position"] = timePercent*1.0/100.0;
          songBroadcast["SongBroadcastMessage"] = songInfo;
          sessionHandler.broadcast(songBroadcast.dump());
      }
    });

    websocketSonginfoSenderTimer.start();

    logger(Level::info) << "shared Listener creation and run\n";
    std::make_shared<Listener>(
                ioc,
                tcp::endpoint{address, port},
                sessionCreator)->run();

    logger(Level::info) << "server started\n";

    ioc.run();

    logger(Level::info) << "ioc finished\n";

    return EXIT_SUCCESS;
}
