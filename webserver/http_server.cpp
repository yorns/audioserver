#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/optional/optional_io.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <boost/filesystem.hpp>

#include "nlohmann/json.hpp"
#include "common/mime_type.h"
#include "common/Extractor.h"
#include "common/logger.h"

#include "id3tagreader/id3TagReader.h"

#include "Listener.h"
#include "Session.h"
#include "playerinterface/mpvplayer.h"
#include "database/SimpleDatabase.h"
#include "webserver/databaseaccess.h"
#include "webserver/playlistaccess.h"
#include "webserver/playeraccess.h"
#include "common/repeattimer.h"

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>
using namespace LoggerFramework;

std::string_view ServerConstant::base_path{"/var/audioserver"};

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
    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    if (argc == 4)  {
        ServerConstant::base_path = std::string_view(argv[3]);
        logger(Level::info) << "setting base path to <"<<ServerConstant::base_path<<">\n";
    }

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // create all strings for the directories used in initialisation
    std::stringstream mp3Dir;
    std::stringstream coverDir;
    std::stringstream playlistDir;
    std::stringstream playerLogDir;
    std::stringstream htmlDir;

    mp3Dir << ServerConstant::base_path << "/" << ServerConstant::audioPath;
    coverDir << ServerConstant::base_path << "/" << ServerConstant::coverPath;
    playlistDir << ServerConstant::base_path << "/" << ServerConstant::playlistPath;
    playerLogDir << ServerConstant::base_path << "/" << ServerConstant::playerLogPath;
    htmlDir << ServerConstant::base_path << "/" << ServerConstant::htmlPath;

    logger(Level::info) << "path: \naudiopath: "<<mp3Dir.str()<<"\ncoverpath: "<<coverDir.str()
              << "\nplaylist: " << playlistDir.str() << "\nPlayerlog dir: "<<playerLogDir.str()
              << "\n";

    SimpleDatabase database;
    std::string currentPlaylist;
    std::unique_ptr<BasePlayer> player;

    logger(Level::info) << "creating database by reading all mp3 files\n";
    database.loadDatabase(mp3Dir.str(), coverDir.str());

    logger(Level::info) << "remove temporal files and load all Playlists available\n";
    database.removeTemporalPlaylists();
    database.loadAllPlaylists(playlistDir.str());
    if (!database.showAllPlaylists().empty())
        currentPlaylist = database.showAllPlaylists().back().second;

    if (currentPlaylist.empty())
        logger(Level::info) << "no current playlist specified\n";

    else logger(Level::info) << "current playlist on startup is: <"<< currentPlaylist<<">\n";

    logger(Level::info) << "\n";
    logger(Level::info) << "create player instance\n";

    player.reset(new MpvPlayer(ioc));

    SessionHandler sessionHandler;
    DatabaseAccess databaseWrapper(database);
    PlaylistAccess playlistAccess(database, currentPlaylist);
    PlayerAccess playerAccess(player, currentPlaylist);

    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::database, http::verb::get, PathCompare::exact,
                                 [&databaseWrapper](const http::request_parser<http::string_body>& request) -> std::string {
        auto url = utility::Extractor::getUrlInformation(std::string(request.get().target()));
        return databaseWrapper.access(url);
    });
    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::playlist, http::verb::get, PathCompare::exact,
                                 [&playlistAccess](const http::request_parser<http::string_body>& request) -> std::string {
        auto url = utility::Extractor::getUrlInformation(std::string(request.get().target()));
        return playlistAccess.access(url);
    });
    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::player, http::verb::post, PathCompare::exact,
                                 [&playerAccess](const http::request_parser<http::string_body>& request) -> std::string {
        auto url = utility::Extractor::getUrlInformation(std::string(request.get().target()));
        return playerAccess.access(url);
    });

    auto generateName = [&mp3Dir]() -> NameGenerator::GenerationName
    {
        auto name = NameGenerator::create(mp3Dir.str() , ".mp3");
        logger(Level::debug) << "generating file name: " << name.filename << "\n";
        return name;
    };

    auto uploadFinishHandler = [&database](const NameGenerator::GenerationName& name)-> bool
    {
        logger(Level::debug) << "creating cover for: " << name.unique_id << "\n";
        id3TagReader reader;
        auto cover = reader.extractCover(name.unique_id);
        if (cover) {
            database.addToDatabase(name.unique_id, *cover);
            return true;
        }
        database.addToDatabase(name.unique_id, "");
        return true;
    };

    sessionHandler.addUploadHandler(ServerConstant::AccessPoints::upload,
                                    generateName,
                                    uploadFinishHandler);

    auto sessionCreator = [&sessionHandler, &htmlDir](tcp::socket& socket) {
        logger(Level::debug) << "\n--- new session created ---\n";
        std::string localHtmlDir{htmlDir.str()};
        std::make_shared<Session>(std::move(socket), sessionHandler, std::move(localHtmlDir))->start();
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
