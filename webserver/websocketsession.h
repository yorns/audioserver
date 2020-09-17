#ifndef WEBSOCKETSESSION_H
#define WEBSOCKETSESSION_H

#include <memory>
#include <deque>
#include <boost/beast.hpp>
#include "common/logger.h"
#include "sessionhandler.h"

namespace beast = boost::beast;                 // from <boost/beast.hpp>
namespace http = beast::http;                   // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;         // from <boost/beast/websocket.hpp>
namespace net = boost::asio;                    // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>

using namespace LoggerFramework;

// Echoes back all received WebSocket messages
class WebsocketSession : public std::enable_shared_from_this<WebsocketSession>
{
    enum class State {
        unconnected,
        accept,
        idle,
//        read,  <- reading all the time
        write,
    };

    websocket::stream<beast::tcp_stream> m_websocketStream;
    boost::asio::ip::tcp::endpoint m_endpoint;
    SessionHandler& m_sessionHandler;
    beast::flat_buffer m_readBuffer;
    beast::flat_buffer m_writeBuffer;

    std::deque<std::string> m_queue;

    State m_state { State::unconnected };

public:
    // Take ownership of the socket
    explicit
    WebsocketSession(tcp::socket&& socket, SessionHandler& sessionHandler);

    ~WebsocketSession();

    // Start the asynchronous accept operation
    template<class Body, class Allocator>
    void
    do_accept(http::request<Body, http::basic_fields<Allocator>> req)
    {
        // Set suggested timeout settings for the websocket
        m_websocketStream.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        m_websocketStream.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " advanced-server");
            }));

        m_sessionHandler.addWebsocketConnection(shared_from_this(), m_endpoint);

        if (m_state != State::unconnected) {
            logger(Level::warning) << "<accept> call on connection, that is not <idle>\n";
            logger(Level::warning) << "abort connection\n";
            return;
        }
        m_state = State::accept;

        // Accept the websocket handshake
        m_websocketStream.async_accept(
            req,
            beast::bind_front_handler(
                &WebsocketSession::on_accept,
                shared_from_this()));
    }

    bool write(std::string&& message);

private:

    void
    fail( beast::error_code ec, char const* what );

    void sendQueued( );

    void on_accept( beast::error_code ec );

    void do_read( );

    void on_read( beast::error_code ec, std::size_t bytes_transferred );

    void on_write( beast::error_code ec, std::size_t bytes_transferred );
};

#endif // WEBSOCKETSESSION_H
