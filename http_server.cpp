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
//#include "common/NameGenerator.h"

#include "Listener.h"
#include "Session.h"
#include "MPlayer.h"
#include "SimpleDatabase.h"

std::unique_ptr<Player> player;
SimpleDatabase database;
std::string currentPlaylist;
bool albumPlaylist {false};

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

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
    }

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    std::stringstream mp3Dir;
    std::stringstream coverDir;
    std::stringstream playlistDir;
    std::stringstream playerLogDir;

    mp3Dir << ServerConstant::base_path << "/" << ServerConstant::audioPath;
    coverDir << ServerConstant::base_path << "/" << ServerConstant::coverPath;
    playlistDir << ServerConstant::base_path << "/" << ServerConstant::playlistPath;
    playerLogDir << ServerConstant::base_path << "/" << ServerConstant::playerLogPath;

    database.loadDatabase(mp3Dir.str(), coverDir.str());
    database.loadAllPlaylists(playlistDir.str());
    if (!database.showAllPlaylists().empty())
        currentPlaylist = database.showAllPlaylists().back().second;

    std::cout << "current playlist on startup is: <"<< currentPlaylist<<">\n";
    std::cout << "player logs go to: " << playerLogDir.str() <<"\n";

    player = std::make_unique<MPlayer>(ioc, "config.dat", playerLogDir.str());

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::sslv23};

    // This holds the self-signed certificate used by the server
    load_server_certificate(ctx);

    ctx.set_verify_mode(boost::asio::ssl::verify_none);

    auto sessionCreator = [](tcp::socket& socket, ssl::context& ctx) {
        std::make_shared<session>(std::move(socket), ctx)->run();
    };

    // Create and launch a listening port
    std::make_shared<Listener>(
        ioc,
        ctx,
        tcp::endpoint{address, port},
        sessionCreator)->run();

    ioc.run();

    std::cerr << "ioc finished\n";

    return EXIT_SUCCESS;
}
