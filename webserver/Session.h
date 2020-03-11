#ifndef SERVER_SESSION_H
#define SERVER_SESSION_H

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <string_view>
#include "sessionhandler.h"
#include "websocketsession.h"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace websocket = boost::beast::websocket;

// Handles an HTTP server connection
class Session : public std::enable_shared_from_this<Session>
{

    std::weak_ptr<WebsocketSession> webSocket;
    // Report a failure
    void fail(boost::system::error_code ec, const std::string& what);

    tcp::socket m_socket;
    boost::beast::flat_buffer m_buffer;

    SessionHandler& m_sessionHandler;
    std::string m_filePath;

    uint32_t m_runID {0};

    void handle_file_request(std::string target, http::verb method, uint32_t version, bool keep_alive);

    void returnMessage();

    void on_read_header(std::shared_ptr<http::request_parser<http::empty_body>> requestHandler, boost::system::error_code ec, std::size_t bytes_transferred);
    void on_read( boost::system::error_code ec, std::size_t bytes_transferred);
    void on_write( boost::system::error_code ec, std::size_t bytes_transferred, bool close);
    void do_close();

    bool is_unknown_http_method(http::request_parser<http::empty_body>& req) const;
    bool is_illegal_request_target(http::request_parser<http::empty_body>& req) const;

    http::response<http::string_body> generate_result_packet(http::status status,
                                                             std::string_view why,
                                                             uint32_t version,
                                                             bool keep_alive,
                                                             std::string_view mime_type = {"text/html"});

    template <class Body>
    void answer(http::response<Body>&& response) {
        auto self { shared_from_this() };
        // do answer synchron, so we do not need to keep the resonse message until end of connection
        http::write(m_socket, response);
        do_close();
        logger(Level::debug) << "resonse send without an error\n";
    }

public:

    explicit Session(tcp::socket socket, SessionHandler& sessionHandler, std::string&& filePath, uint32_t runID);
    Session() = delete;

    Session(const Session& ) = delete;
    Session(Session&& ) = delete;

    virtual ~Session() { logger(LoggerFramework::Level::info) << "<" << m_runID << "> session ended\n"; }

    void start();


};



#endif //SERVER_SESSION_H
