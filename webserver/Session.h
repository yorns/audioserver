#ifndef SERVER_SESSION_H
#define SERVER_SESSION_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <string_view>
#include "sessionhandler.h"
#include "websocketsession.h"
#include "database/SimpleDatabase.h"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace websocket = boost::beast::websocket;

struct http_range {
    uint64_t from;
    uint64_t to;
    bool import(const std::string& data) {
        // TODO: change from boost to std (needs code changes)
        boost::regex expr{"bytes=(\\d*)-(\\d*)"};
        logger(LoggerFramework::Level::debug) << "analysing <"<<data<<">\n";
        boost::smatch what;
        if (boost::regex_search(data, what, expr)) {
            std::string fromString = what[1];
            std::string toString = what[2];
            logger(LoggerFramework::Level::debug) << "search results: " << fromString << " - " << toString <<"\n";
            try {
            if (!fromString.empty())
                from = boost::lexical_cast<uint32_t>(fromString);
            else
                from = 0;
            if (!toString.empty())
                to = boost::lexical_cast<uint32_t>(toString);
            else
                to = 0;
            } catch (std::exception& ex) { logger(LoggerFramework::Level::warning) << "lexical cast error: "<<ex.what()<<"\n"; }
          return true;
        }
        return false;
    }
    http_range() : from(0), to(0) {}
    http_range(const std::string& data) : from(0), to(0) { import(data); }
};


// Handles an HTTP server connection
class Session : public std::enable_shared_from_this<Session>
{

    typedef std::function<std::optional<std::string>(const std::string&)> PasswordFind;

    std::weak_ptr<WebsocketSession> webSocket;
    PasswordFind m_passwordFind;

    // Report a failure
    void fail(boost::system::error_code ec, const std::string& what);

    tcp::socket m_socket;
    boost::beast::flat_buffer m_buffer;

    SessionHandler& m_sessionHandler;
    std::string m_filePath;

    uint32_t m_runID {0};

    void handle_file_request(std::string target, http::verb method, uint32_t version, bool keep_alive, std::optional<http_range> rangeData);
    void handle_regular_file_request(std::string path, http::verb method, uint32_t version, bool keep_alive, std::optional<http_range> rangeData);

    void returnMessage();

    void on_read_header(std::shared_ptr<http::request_parser<http::empty_body>> requestHandler, boost::system::error_code ec, std::size_t bytes_transferred);
    void on_read( boost::system::error_code ec, std::size_t bytes_transferred);
    void on_write( boost::system::error_code ec, std::size_t bytes_transferred, bool close);
    void do_close();

    bool is_unknown_http_method(http::request_parser<http::empty_body>& req) const;
    bool is_illegal_request_target(http::request_parser<http::empty_body>& req) const;

    std::shared_ptr<http::response<http::string_body>> generate_result_packet(http::status status,
                                                             std::string_view why,
                                                             uint32_t version,
                                                             bool keep_alive,
                                                             std::string_view mime_type = {"text/html"});

    template <class Body>
    bool answer(http::response<Body>&& response) {
        auto self { shared_from_this() };
        // do answer synchron, so we do not need to keep the resonse message until end of connection
        const auto keep_alive = response.keep_alive();
        boost::beast::error_code ec;
        http::write( m_socket, response, ec );
        if (ec) {
            do_close();
            return false;
        }
        logger(Level::info) << "<"<<m_runID<< "> resonse send without an error\n";
        if (!keep_alive) {
            do_close();
        }
        else {
            logger(Level::info) << "<"<<m_runID<< "> keep alive set, start next receive\n";
            start();
        }
        return true;
    }

    template <class Body>
    bool answer(std::shared_ptr<http::response<Body>> response) {
        auto self { shared_from_this() };
        http::async_write( m_socket, *response, [this, self, response](const boost::beast::error_code& ec, std::size_t sendsize ) {
            if (ec) {
                logger(LoggerFramework::Level::warning) << "<"<<m_runID<< "> sending failed (after "<< sendsize <<") - closing connection\n: " << ec.message() <<"\n";
                do_close();
                return;
            }

            logger(Level::info) << "<"<<m_runID<< "> resonse send without an error\n";
            if (!(response->keep_alive())) {
                do_close();
            }
            else {
                logger(Level::info) << "<"<<m_runID<< "> keep alive set, start next receive\n";
                start();
            }
        });
        return true;
    }

public:

    explicit Session(tcp::socket socket, SessionHandler& sessionHandler, std::string&& filePath,
                     PasswordFind&& credentialPW, uint32_t runID);
    Session() = delete;

    Session(const Session& ) = delete;
    Session(Session&& ) = delete;

    virtual ~Session() { logger(LoggerFramework::Level::debug) << "<" << m_runID << "> session ended\n"; }

    void start();


};



#endif //SERVER_SESSION_H
