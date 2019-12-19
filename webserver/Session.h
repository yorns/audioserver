#ifndef SERVER_SESSION_H
#define SERVER_SESSION_H

#include "common/server_certificate.hpp"

#include <boost/optional/optional_io.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/config.hpp>
#include <boost/filesystem.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "nlohmann/json.hpp"
#include "common/Constants.h"

#include "RequestHandler.h"

using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
namespace http = boost::beast::http;

extern SimpleDatabase database;

// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session>
{

    // Report a failure
    void fail(boost::system::error_code ec, const std::string& what);

    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct send_lambda
    {
        session& m_self;

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
            m_self.m_result = sp;

            auto selfself { m_self.shared_from_this() };
            // Write the response
            http::async_write(
                    m_self.m_socket,
                    *sp, [this, selfself](boost::system::error_code ,
                                          std::size_t ){
                        m_self.do_close();
                    });
        }
    };

    tcp::socket m_socket;
    boost::beast::flat_buffer m_buffer;
    std::shared_ptr<std::string const> m_doc_root;
    std::unique_ptr<http::request_parser<http::string_body>> m_req;
    std::unique_ptr<http::request_parser<http::file_body>> m_reqFile;
    http::request_parser<http::empty_body> m_reqHeader;
    std::shared_ptr<void> m_result;
    send_lambda m_lambda;

    RequestHandler m_requestHandler;

    void handle_upload_request();
    void handle_normal_request();


public:

    explicit session( tcp::socket socket);

    void run();

    void on_read_header( boost::system::error_code ec, std::size_t bytes_transferred);
    void on_read( boost::system::error_code ec, std::size_t bytes_transferred);
    void on_write( boost::system::error_code ec, std::size_t bytes_transferred, bool close);
    void do_close();

};



#endif //SERVER_SESSION_H
