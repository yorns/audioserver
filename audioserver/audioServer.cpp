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
#include "config/config.h"

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
#include "common/misc.h"

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>
using namespace LoggerFramework;
using namespace std::chrono_literals;

int helpOutput(const char* command) {
    logger(Level::info) <<
        "Usage:" << command << " [config file name]\n" <<
        "Example:\n" <<
        "    "<< command <<" /my/path/config.json\n";
    return EXIT_FAILURE;
}


int main(int argc, char* argv[])
{
    // set default config file
    std::string configFileName {"/usr/local/etc/audioserver.json"};

    // Check command line arguments.
    if (argc > 2)
    {
        return helpOutput(argv[0]);
    }
    else if ( argc == 2) {
        configFileName = argv[1];
    }

    // generate config
    auto config = std::make_shared<Common::Config>();

    if (!config) {
        std::cerr << "cannot read config file <" << configFileName << ">\n";
        return EXIT_FAILURE;
    }

    config->readConfig(configFileName);

    LoggerFramework::setGlobalLevel(config->m_logLevel);
    config->print();


    // set some fixed values for the connection
    auto const address = boost::asio::ip::make_address(config->m_address);
    auto const port = boost::lexical_cast<unsigned short>(config->m_port.c_str());
    ServerConstant::base_path = std::string_view(config->m_basePath);

    std::setlocale(LC_ALL, "de_DE.UTF-8");

    Common::printPaths();

    /* set dynamic web handling */
    boost::asio::io_context ioc;

    logger(Level::info) << "connection client <audioserver> to broker\n";
    std::shared_ptr<snc::Client> sncClient; // = std::make_shared<snc::Client>("audioserver", ioc, "127.0.0.1", 12001);

    /* generate the databe and fill it */
    std::shared_ptr<Database::SimpleDatabase> database = std::make_shared<Database::SimpleDatabase>(config->m_enableCache);
    database->loadDatabase();

    /* generate a wifi manager */
    WifiManager wifiManager;
    std::shared_ptr<BasePlayer> player(nullptr);

    /* generate the player */
    if (config->isPlayerType(Common::Config::PlayerType::GstPlayer)) {
        player = std::shared_ptr<BasePlayer>(new GstPlayer(ioc));
        player->setAmplify(config->m_amplify);
    }
    if (config->isPlayerType(Common::Config::PlayerType::MpvPlayer)) {
        player = std::shared_ptr<BasePlayer>(new MpvPlayer(ioc));
        player->setAmplify(config->m_amplify);
    }

    if (!player) {
        logger(Level::info) << "no player set, using web version only\n";
    }

    /* generate dynamic access point */
    SessionHandler sessionHandler;
    DatabaseAccess databaseWrapper(database);
    PlayerAccess   playerWrapper(player);
    WifiAccess     wifiWrapper(wifiManager);
    PlaylistAccess playlistWrapper(database, player);

    /* create a new full qualified name (with uuid) */
    auto generateName = []() -> Common::NameGenerator::GenerationName
    {
        auto name = Common::NameGenerator::create(Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::AudioMp3) , ".mp3");
        logger(Level::debug) << "generating file name: " << name.filename << "\n";
        return name;
    };

    /* handler that is called, when upload has finished for a specific file */
    auto uploadFinishHandler = [&databaseWrapper](const Common::NameGenerator::GenerationName& name)-> bool
    {
        logger(Level::info) << "file upload was successfull, now add file to database\n";
        Common::FileNameType file;
        file.dir = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::AudioMp3);
        file.name = name.unique_id;
        file.extension = std::string(ServerConstant::mp3Extension);
        return databaseWrapper.getDatabase()->addNewAudioFileUniqueId(file);
    };

    /* handler to handle a web session call */
    auto sessionCreator = [&sessionHandler, &databaseWrapper](tcp::socket&& socket) {
        static uint32_t sessionId { 0 };
        logger(Level::info) << "--- new session created with ID <" << sessionId << "> ---\n";
        std::string localHtmlDir { Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Html) };
        std::make_shared<Session>(std::move(socket), sessionHandler, std::move(localHtmlDir), [&databaseWrapper]
                                  (const std::string& name){
            logger(Level::info) << "audioServer::sessionCreator\n";
            return databaseWrapper.getDatabase()->passwordFind(name); },  sessionId++)->start();
    };

    if (sncClient) {
        sncClient->recvHandler([&databaseWrapper, &playerWrapper, &sessionHandler, &sncClient]
                               (const std::string& other, const std::string& raw_msg) {
            Common::audioserver_externalSelect(databaseWrapper, playerWrapper, sessionHandler, sncClient, other, raw_msg);
        });
    }

    if (player) {
        auto songEndCallback = [&sessionHandler, &sncClient, & playerWrapper, &databaseWrapper](const boost::uuids::uuid& songID){
            boost::ignore_unused(songID);
            Common::audioserver_updateUI(sessionHandler, sncClient, playerWrapper, databaseWrapper);
            logger(Level::info) << "end handler called for current song\n";
        };
      player->setSongEndCB(std::move(songEndCallback));
    }

    /* set session handler for different access points (mostly REST) */

    /* database access point */
    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::database, http::verb::get, PathCompare::exact,
                                 [&databaseWrapper](const http::request_parser<http::string_body>& request) -> std::string {
        if (auto url = utility::Extractor::getUrlInformation(request.get().target(), ServerConstant::AccessPoints::database))
            return databaseWrapper.access(url);
        else
            logger(Level::debug) << "url not set correctly\n";
        return "";
    });

    /* playlist access point */
    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::playlist, http::verb::get, PathCompare::exact,
                                 [&playlistWrapper](const http::request_parser<http::string_body>& request) -> std::string {
        if (auto url = utility::Extractor::getUrlInformation(request.get().target(), ServerConstant::AccessPoints::playlist))
            return playlistWrapper.access(url);
        else
            logger(Level::debug) << "url not set correctly\n";
        return "";
    });

    /* player access point */
    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::player, http::verb::post, PathCompare::exact,
                                 [&playerWrapper](const http::request_parser<http::string_body>& request) -> std::string {
        if (auto url = utility::Extractor::getUrlInformation(std::string(request.get().target()), ServerConstant::AccessPoints::player))
            return playerWrapper.access(url);
        else
            logger(Level::debug) << "url not set correctly\n";
        return "";
    });

    /* wifi access point */
    sessionHandler.addUrlHandler(ServerConstant::AccessPoints::wifi, http::verb::post, PathCompare::exact,
                                 [&wifiWrapper](const http::request_parser<http::string_body>& request) -> std::string {
        if (auto url = utility::Extractor::getUrlInformation(std::string(request.get().target()), ServerConstant::AccessPoints::wifi))
            return wifiWrapper.access(url);
        else
            logger(Level::debug) << "url not set correctly\n";
        return "";
    });
    
    
    /* add access point for the virtual images */
    sessionHandler.addVirtualImageHandler([&databaseWrapper]
                                          (const std::string_view& target)
    { return databaseWrapper.virtualImageHandler(target); } );

    /* add accesspoint for virtual audio files */
    sessionHandler.addVirtualAudioHandler([&databaseWrapper]
                                          (const std::string_view& target)
    { return databaseWrapper.virtualAudioHandler(target); } );

    /*  */
    sessionHandler.addVirtualPlaylistHandler([&databaseWrapper]
                                             (const std::string_view& target)
       { return databaseWrapper.virtualPlaylistHandler(target); } );

    sessionHandler.addUploadHandler(ServerConstant::AccessPoints::upload,
                                    generateName,
                                    uploadFinishHandler);

    /* create a websocket handler that is called every 1/2 second to sent UI information */
    RepeatTimer websocketSonginfoSenderTimer(ioc, 500ms ); //std::chrono::milliseconds(500));

    websocketSonginfoSenderTimer.setHandler([&sessionHandler, &sncClient, &playerWrapper, &databaseWrapper](){
        if (playerWrapper.hasPlayer()) {
            Common::audioserver_updateUI(sessionHandler, sncClient, playerWrapper, databaseWrapper);

        //    updateUI();
        }
    });

    /* start websocket handler */
    websocketSonginfoSenderTimer.start();

    /* one listener at a specific port can create/run multiple sessions */
    logger(Level::info) << "shared Listener creation and run\n";
    std::make_shared<Listener>(
                ioc,
                tcp::endpoint{address, port},
                sessionCreator)->run();

    // only to show all REST accesspoints
    logger(Level::info) << "Rest Accesspoints:\n" << sessionHandler.generateRESTInterfaceDocumentation();

    logger(Level::info) << "server set up and now started\n";

    /* start the main run loop */
    ioc.run();

    logger(Level::info) << "ioc finished\n";

    return EXIT_SUCCESS;
}
