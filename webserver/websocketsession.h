#ifndef WEBSOCKETSESSION_H
#define WEBSOCKETSESSION_H

#include <memory>
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
    websocket::stream<beast::tcp_stream> m_websocketStream;
    boost::asio::ip::tcp::endpoint m_endpoint;
    SessionHandler& m_sessionHandler;
    beast::flat_buffer m_readBuffer;
    beast::flat_buffer m_writeBuffer;

public:
    // Take ownership of the socket
    explicit
    WebsocketSession(tcp::socket&& socket, SessionHandler& sessionHandler)
        : m_websocketStream(std::move(socket)),
          m_endpoint(m_websocketStream.next_layer().socket().remote_endpoint()),
          m_sessionHandler(sessionHandler)
    {
    }

    ~WebsocketSession() {
       m_sessionHandler.removeWebsocketConnection(m_endpoint);
    }

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

        // Accept the websocket handshake
        m_websocketStream.async_accept(
            req,
            beast::bind_front_handler(
                &WebsocketSession::on_accept,
                shared_from_this()));
    }

    bool write(const std::string& message) {

        // test if last write has been done successful
        if (m_writeBuffer.size() != 0)
            return false;

        beast::ostream(m_writeBuffer) << message;

        m_websocketStream.async_write(
            m_writeBuffer.data(),
            beast::bind_front_handler(
                &WebsocketSession::on_write,
                shared_from_this()));

        return true;
    }

private:

    void
    fail(beast::error_code ec, char const* what)
    {
        logger(Level::warning) << what << ": " << ec.message() << "\n";
    }

    void
    on_accept(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "accept");

        if (write(R"({"result": "done"})"))
            logger(Level::info) << "websocket result write successful\n";
        else
            logger(Level::info) << "websocket result write failed\n";

        // Read a message
        do_read();
    }

    void
    do_read()
    {
        // Read a message into our buffer
        m_websocketStream.async_read(
            m_readBuffer,
            beast::bind_front_handler(
                &WebsocketSession::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the websocket_session was closed
        if(ec == websocket::error::closed)
            return;

        if(ec)
            fail(ec, "read");
        else {
        // go on reading
        do_read();
        }
    }

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        if(ec)
            return fail(ec, "write");

        // Clear the buffer
        m_writeBuffer.consume(bytes_transferred);

    }
};

#endif // WEBSOCKETSESSION_H
