//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP SSL server, asynchronous
//
//------------------------------------------------------------------------------
// curl -X POST --insecure https://localhost:8080/mypost?data=123\&data2=456
// curl http://myservice --upload-file file.txt
// curl https://localhost:8080/ --upload-file 01\ Kryptonite.mp3  --insecure
// curl https://localhost:8080/find?overall=Kry --insecure
// curl -X POST --insecure https://localhost:8080/player?play=6407c910-621b-4608-a1fb-f5d9a81fc30a


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
#include <nlohmann/json.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include "common/mime_type.h"
#include "common/Extractor.h"

#include "Listener.h"
#include "Session.h"
#include "Player.h"
#include "MPlayer.h"
#include "SimpleDatabase.h"

std::unique_ptr<Player> player;
SimpleDatabase database;
std::string currentPlaylist;

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>



//------------------------------------------------------------------------------



int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 4)
    {
        std::cerr <<
            "Usage:" << argv[0] << " <address> <port> <doc_root>\n" <<
            "Example:\n" <<
            "    "<<argv[0]<<" 0.0.0.0 8080 .\n";
        return EXIT_FAILURE;
    }
    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const doc_root = std::make_shared<std::string>(argv[3]);
//    auto const threads = std::max<int>(1, std::atoi(argv[4]));

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    database.loadDatabase(ServerConstant::fileRootPath.to_string(), ServerConstant::coverRootPath.to_string());
    database.loadAllPlaylists("playlist");
    if (!database.showAllPlaylists().empty())
        currentPlaylist = database.showAllPlaylists().back().second;

    std::cout << "current playlist on startup is: "<< currentPlaylist<<"\n";

    player = std::make_unique<MPlayer>(ioc, "config.dat", "$(HOME)/.player");

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::sslv23};

    // This holds the self-signed certificate used by the server
    load_server_certificate(ctx);

    ctx.set_verify_mode(boost::asio::ssl::verify_none);

    auto sessionCreator = [doc_root](tcp::socket& socket, ssl::context& ctx) {
        std::make_shared<session>(std::move(socket), ctx, doc_root)->run();
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
