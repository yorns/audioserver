#ifndef SERVER_SESSION_H
#define SERVER_SESSION_H

#include "common/server_certificate.hpp"

#include <boost/optional/optional_io.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
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
#include "common/Constants.h"

#include "RequestHandler.h"

using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
namespace http = boost::beast::http;

// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session>
{

    // Report a failure
    void fail(boost::system::error_code ec, char const* what);

    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct send_lambda
    {
        session& self_;

        explicit send_lambda(session& self);

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
    boost::beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    std::unique_ptr<http::request_parser<http::string_body>> req_;
    std::unique_ptr<http::request_parser<http::file_body>> reqFile_;
    // Start with an empty_body parser
    http::request_parser<http::empty_body> reqHeader_;
    std::shared_ptr<void> res_;
    send_lambda lambda_;

    RequestHandler m_requestHandler;

    std::string generate_unique_name();

public:

    explicit session( tcp::socket socket, ssl::context& ctx, std::shared_ptr<std::string const> const& doc_root);

    void run();

    void on_handshake(boost::system::error_code ec);
    void do_read();
    void on_read_header( boost::system::error_code ec, std::size_t bytes_transferred);
    void on_read( boost::system::error_code ec, std::size_t bytes_transferred);
    void on_write( boost::system::error_code ec, std::size_t bytes_transferred, bool close);
    void do_close();
    void on_shutdown(boost::system::error_code ec);
};


#endif //SERVER_SESSION_H
