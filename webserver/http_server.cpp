#include "common/server_certificate.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
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

#include "Listener.h"
#include "Session.h"
#include "playerinterface/MPlayer.h"
#include "playerinterface/mpvplayer.h"
#include "database/SimpleDatabase.h"

std::unique_ptr<Player> player;
SimpleDatabase database;
std::string currentPlaylist;
bool albumPlaylist {false};

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

boost::beast::string_view ServerConstant::base_path{"/var/audioserver"};

int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 4 && argc != 3)
    {
        std::cerr <<
            "Usage:" << argv[0] << " <address> <port> [doc_root]\n" <<
            "Example:\n" <<
            "    "<<argv[0]<<" 0.0.0.0 8080 .\n";
        return EXIT_FAILURE;
    }
    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    if (argc == 4)  {
        ServerConstant::base_path = boost::beast::string_view(argv[3]);
        std::cout << "setting base path to <"<<ServerConstant::base_path<<">\n";
    }

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // create all strings for the directories used in initialisation
    std::stringstream mp3Dir;
    std::stringstream coverDir;
    std::stringstream playlistDir;
    std::stringstream playerLogDir;

    mp3Dir << ServerConstant::base_path << "/" << ServerConstant::audioPath;
    coverDir << ServerConstant::base_path << "/" << ServerConstant::coverPath;
    playlistDir << ServerConstant::base_path << "/" << ServerConstant::playlistPath;
    playerLogDir << ServerConstant::base_path << "/" << ServerConstant::playerLogPath;

    std::cout << "path: \naudiopath: "<<mp3Dir.str()<<"\ncoverpath: "<<coverDir.str()
              << "\nplaylist: " << playlistDir.str() << "\nPlayerlog dir: "<<playerLogDir.str()
              << "\n";

    std::cout << "creating database by reading all mp3 files\n";
    database.loadDatabase(mp3Dir.str(), coverDir.str());

    std::cout << "remove temporal files and load all Playlists available\n";
    database.removeTemporalPlaylists();
    database.loadAllPlaylists(playlistDir.str());
    if (!database.showAllPlaylists().empty())
        currentPlaylist = database.showAllPlaylists().back().second;

    if (currentPlaylist.empty()) std::cout << "no current playlist specified\n";
    else std::cout << "current playlist on startup is: <"<< currentPlaylist<<">\n";

    std::cout << "\n";
    std::cout << "create player instance\n";
    std::cout << "player logs go to: " << playerLogDir.str() <<"\n";

    //player = std::make_unique<MPlayer>(ioc, "config.dat", playerLogDir.str());
    player.reset(new MpvPlayer(ioc, "config.dat", playerLogDir.str()));

    auto sessionCreator = [](tcp::socket& socket) {
        std::cerr << "session creator lambda called\n";
        std::make_shared<session>(std::move(socket))->run();
    };

    std::cerr << "shared Listener creation\n";

    // Create and launch a listening port
    std::make_shared<Listener>(
        ioc,
        tcp::endpoint{address, port},
        sessionCreator)->run();

    std::cout << "run server\n";

    ioc.run();

    std::cerr << "ioc finished\n";

    return EXIT_SUCCESS;
}
