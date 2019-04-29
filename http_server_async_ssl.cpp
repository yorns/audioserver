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

#include "common/server_certificate.hpp"

#include <boost/optional/optional_io.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.


#include "common/Extractor.h"
#include "Player.h"
#include "MPlayer.h"
#include "Id3Reader.h"
#include "SimpleDatabase.h"

std::unique_ptr<Player> player;
SimpleDatabase database;

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>


namespace ServerConstant {
    boost::beast::string_view fileRootPath {"../mp3/"};
}

// Return a reasonable mime type based on the extension of a file.
boost::beast::string_view
mime_type(boost::beast::string_view path)
{
    using boost::beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == boost::beast::string_view::npos)
            return boost::beast::string_view{};
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))  return "text/html";
    if(iequals(ext, ".html")) return "text/html";
    if(iequals(ext, ".php"))  return "text/html";
    if(iequals(ext, ".css"))  return "text/css";
    if(iequals(ext, ".txt"))  return "text/plain";
    if(iequals(ext, ".js"))   return "application/javascript";
    if(iequals(ext, ".json")) return "application/json";
    if(iequals(ext, ".xml"))  return "application/xml";
    if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))  return "video/x-flv";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpe"))  return "image/jpeg";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".bmp"))  return "image/bmp";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff")) return "image/tiff";
    if(iequals(ext, ".tif"))  return "image/tiff";
    if(iequals(ext, ".svg"))  return "image/svg+xml";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string
path_cat(
    boost::beast::string_view base,
    boost::beast::string_view path)
{
    if(base.empty())
        return path.to_string();
    std::string result = base.to_string();
#if BOOST_MSVC
    char constexpr path_separator = '\\';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}


http::response<http::string_body> generate_result_packet(http::status status,
                                                         boost::beast::string_view why,
                                                         http::request_parser<http::string_body>& req,
                                                         boost::beast::string_view mime_type = {"text/html"})
{
    http::response<http::string_body> res{status, req.get().version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type.to_string());
    res.keep_alive(req.get().keep_alive());
    res.body() = why.to_string();
    res.prepare_payload();
    return res;
}


// This function produces an HTTP response for the given
// req.get().est. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template<
        class Send>
void
handle_request(
        boost::beast::string_view doc_root,
        http::request_parser<http::string_body>& req,
        Send&& send)
{

    // Make sure we can handle the method
    if( req.get().method() != http::verb::get &&
        req.get().method() != http::verb::head &&
        req.get().method() != http::verb::post)
        return send(generate_result_packet(http::status::bad_request, "Unknown HTTP-method", req));

    // Request path must be absolute and not contain "..".
    if( req.get().target().empty() ||
        req.get().target()[0] != '/' ||
        req.get().target().find("..") != boost::beast::string_view::npos)
        return send(generate_result_packet(http::status::bad_request, "Illegal request-target", req));

    // has this been a post message
    if(req.get().method() == http::verb::post) {
        std::string path = path_cat(doc_root, req.get().target());
        std::cerr << "posted: "+path+"\n";

        auto urlInfo = utility::Extractor::getUrlInformation(path);

        std::string result {"done"};
        if (urlInfo) {
            std::cerr << "url extracted to: "<<urlInfo->command<< " - "<<urlInfo->parameter<<"="<<urlInfo->value<<"\n";

            if (player && urlInfo->command == "player" && urlInfo->parameter == "file") {
                player->startPlay(ServerConstant::fileRootPath.to_string()+urlInfo->value,"");
            }

            if (player && urlInfo->command == "player" && urlInfo->parameter == "next" && urlInfo->value == "true") {
                player->next_file();
            }

            if (player && urlInfo->command == "player" && urlInfo->parameter == "prev" && urlInfo->value == "true") {
                player->prev_file();
            }

            if (player && urlInfo->command == "player" && urlInfo->parameter == "stop" && urlInfo->value == "true") {
                player->stop();
            }

            if (urlInfo->command == "playlist" && urlInfo->parameter == "create") {
                std::cout << "create playlist with name <"<<urlInfo->value<<">";
            }

            if (urlInfo->command = "playlist" && urlInfo->parameter == "add")
        }
        else {
            result = "request failed";
        }

        nlohmann::json json;
        json["result"] = result;

        return send(generate_result_packet(http::status::ok, json.dump(2), req, "application/json"));
    }

    if(req.get().method() == http::verb::get) {
        std::string path = path_cat(doc_root, req.get().target());

        boost::optional<std::tuple<std::string, SimpleDatabase::DatabaseSearchType>> jsonList = database.findInDatabaseToJson(path);

        if (jsonList) {
            std::cerr << "Database find requested at " + path + "\n";

            if (std::get<SimpleDatabase::DatabaseSearchType>(jsonList.get()) !=
                SimpleDatabase::DatabaseSearchType::unknown) {

                return send(generate_result_packet(http::status::ok, std::get<std::string>(jsonList.get()),
                                                   req, "application/json"));
            } else {
                return send(generate_result_packet(http::status::ok, " unknown find string ", req));
            }

        }

    }

    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.get().target());
    if(req.get().target().back() == '/')
        path.append("index.html");


    // Attempt to open the file
    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if(ec == boost::system::errc::no_such_file_or_directory)
        return send(generate_result_packet(http::status::not_found, req.get().target(), req));

    // Handle an unknown error
    if(ec)
        return send(generate_result_packet(http::status::internal_server_error, ec.message(), req));

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if(req.get().method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.get().version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.get().keep_alive());
        return send(std::move(res));
    }

    // Respond to GET request
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.get().version())};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.get().keep_alive());
    return send(std::move(res));
}

//------------------------------------------------------------------------------

// Report a failure
void
fail(boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session>
{
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct send_lambda
    {
        session& self_;

        explicit
        send_lambda(session& self)
            : self_(self)
        {
        }

        template<bool isRequest, class Body, class Fields>
        void
        operator()(http::message<isRequest, Body, Fields>&& msg) const
        {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<
                http::message<isRequest, Body, Fields>>(std::move(msg));

            // Store a type-erased version of the shared
            // pointer in the class to keep it alive.
            self_.res_ = sp;

            auto selfself { self_.shared_from_this() };
            // Write the response
            http::async_write(
                self_.stream_,
                *sp, [this, selfself](boost::system::error_code ,
                            std::size_t ){
                        self_.do_close();
                    });
        }
    };

    tcp::socket socket_;
    ssl::stream<tcp::socket&> stream_;
    boost::asio::strand<
        boost::asio::io_context::executor_type> strand_;
    boost::beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    std::unique_ptr<http::request_parser<http::string_body>> req_;
    std::unique_ptr<http::request_parser<http::file_body>> reqFile_;
    // Start with an empty_body parser
    http::request_parser<http::empty_body> reqHeader_;
    std::shared_ptr<void> res_;
    send_lambda lambda_;

public:
    // Take ownership of the socket
    explicit
    session(
        tcp::socket socket,
        ssl::context& ctx,
        std::shared_ptr<std::string const> const& doc_root)
        : socket_(std::move(socket))
        , stream_(socket_, ctx)
        , strand_(socket_.get_executor())
        , doc_root_(doc_root)
        , lambda_(*this)
    {
    }

    // Start the asynchronous operation
    void
    run()
    {
        auto self { shared_from_this() };
        // Perform the SSL handshake
        stream_.async_handshake(
            ssl::stream_base::server,
            [this, self](boost::system::error_code ec) {
                on_handshake(ec);
            });
    }

    void
    on_handshake(boost::system::error_code ec)
    {
        if(ec)
            return fail(ec, "handshake");

        do_read();
    }

    void
    do_read()
    {
        auto self { shared_from_this() };
        // Read a request

        reqHeader_.body_limit(100*1024*1024);

        http::async_read_header(stream_, buffer_, reqHeader_,
            [this, self]( boost::system::error_code ec,
                std::size_t bytes_transferred)
            { on_read_header(ec, bytes_transferred); });
    }

    void
    on_read_header( boost::system::error_code ec,
                    std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if (ec == http::error::end_of_stream)
            return do_close();

        if (ec)
            return fail(ec, "read");

        std::cerr << "handle read (async header read)\n";

        auto self { shared_from_this() };

        if (reqHeader_.get().method() == http::verb::put) {

            std::cerr << "transfering data with length: " << reqHeader_.content_length() << "\n";

            boost::uuids::random_generator generator;
            boost::uuids::uuid _name = generator();
            std::stringstream file_name_str;
            file_name_str << ServerConstant::fileRootPath << _name << ".mp3";
            std::string file_name { file_name_str.str() };

            reqFile_ = std::make_unique < http::request_parser < http::file_body >> (std::move(reqHeader_));

            boost::beast::error_code error_code;
            reqFile_->get().body().open(file_name.c_str(), boost::beast::file_mode::write, error_code);

            if (error_code) {
                std::cerr << "error: " << error_code << "\n";
                http::response<http::string_body> res{http::status::internal_server_error, reqFile_->get().version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(reqFile_->get().keep_alive());
                res.body() = "An error occurred: 'open file failed for writing'";
                res.prepare_payload();
                return lambda_(std::move(res));
            }

            // Read a request
            http::async_read(stream_, buffer_, *reqFile_.get(),
                             [this, self, file_name](boost::system::error_code ec,
                                          std::size_t bytes_transferred) {
                                 std::cerr << "finished read <" << file_name << ">\n";
                                 http::response <http::empty_body> res{http::status::ok,
                                                                       reqFile_.get()->get().version()};
                                 res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                                 res.set(http::field::content_type, "audio/mp3");
                                 res.content_length(0);
                                 res.keep_alive(reqFile_.get()->get().keep_alive());
                                 return lambda_(std::move(res));
                             }
            );
            std::cerr << "async read started\n";
        } else {
            req_ = std::make_unique < http::request_parser < http::string_body >> (std::move(reqHeader_));
            // Read a request
            auto& req = *req_.get();
            http::async_read(stream_, buffer_, req ,
                             [this, self](boost::system::error_code ec,
                                          std::size_t bytes_transferred) { on_read(ec, bytes_transferred); });
        }
    }

    void
    on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if(ec == http::error::end_of_stream)
            return do_close();

        if(ec)
            return fail(ec, "read");

        std::cerr << "handle read (async read)\n";

        // Send the response
        handle_request(*doc_root_, *req_.get(), lambda_);
    }

    void
    on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred,
        bool close)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        if(close)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // We're done with the response so delete it
        res_ = nullptr;

        // Read another request
        do_read();
    }

    void
    do_close()
    {
        // Perform the SSL shutdown
        stream_.async_shutdown(
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &session::on_shutdown,
                    shared_from_this(),
                    std::placeholders::_1)));
    }

    void
    on_shutdown(boost::system::error_code ec)
    {
        if(ec)
            return fail(ec, "shutdown");

        // At this point the connection is closed gracefully
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    ssl::context& ctx_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    std::shared_ptr<std::string const> doc_root_;

public:
    listener(
        boost::asio::io_context& ioc,
        ssl::context& ctx,
        tcp::endpoint endpoint,
        std::shared_ptr<std::string const> const& doc_root)
        : ctx_(ctx)
        , acceptor_(ioc)
        , socket_(ioc)
        , doc_root_(doc_root)
    {
        boost::system::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            boost::asio::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void
    run()
    {
        if(! acceptor_.is_open())
            return;
        do_accept();
    }

    void
    do_accept()
    {
        acceptor_.async_accept(
            socket_,
            std::bind(
                &listener::on_accept,
                shared_from_this(),
                std::placeholders::_1));
    }

    void
    on_accept(boost::system::error_code ec)
    {
        if(ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the session and run it
            std::make_shared<session>(
                std::move(socket_),
                ctx_,
                doc_root_)->run();
        }

        // Accept another connection
        do_accept();
    }
};

//------------------------------------------------------------------------------



int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 5)
    {
        std::cerr <<
            "Usage: http-server-async-ssl <address> <port> <doc_root> <threads>\n" <<
            "Example:\n" <<
            "    http-server-async-ssl 0.0.0.0 8080 . 1\n";
        return EXIT_FAILURE;
    }
    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const doc_root = std::make_shared<std::string>(argv[3]);
    auto const threads = std::max<int>(1, std::atoi(argv[4]));

    // The io_context is required for all I/O
    boost::asio::io_context ioc{threads};

    database.loadDatabase(ServerConstant::fileRootPath.to_string());

    player = std::make_unique<MPlayer>(ioc, "config.dat", "$(HOME)/.player");

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::sslv23};

    // This holds the self-signed certificate used by the server
    load_server_certificate(ctx);

    ctx.set_verify_mode(boost::asio::ssl::verify_none);

    // Create and launch a listening port
    std::make_shared<listener>(
        ioc,
        ctx,
        tcp::endpoint{address, port},
        doc_root)->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    return EXIT_SUCCESS;
}
