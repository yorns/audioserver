#include "websocketsession.h"

WebsocketSession::WebsocketSession(boost::asio::ip::tcp::socket &&socket, SessionHandler &sessionHandler)
    : m_websocketStream(std::move(socket)),
      m_endpoint(m_websocketStream.next_layer().socket().remote_endpoint()),
      m_sessionHandler(sessionHandler)
{
}

WebsocketSession::~WebsocketSession() {
    m_sessionHandler.removeWebsocketConnection(m_endpoint);
}

bool WebsocketSession::write(std::string &&message) {

    if (m_state == State::unconnected)
        return false;

    // test queue size and remove oldest packet

    m_queue.emplace_back(message);

    sendQueued();

    return true;
}

void WebsocketSession::fail(beast::error_code ec, const char *what)
{
    logger(Level::warning) << what << ": " << ec.message() << "\n";
}

void WebsocketSession::sendQueued() {

    if (m_state != State::idle || m_queue.empty())
        return;

    auto& message = m_queue.front();

    beast::ostream(m_writeBuffer) << message;

    m_state = State::write;
    m_queue.pop_front();

    m_websocketStream.async_write(
                m_writeBuffer.data(),
                beast::bind_front_handler(
                    &WebsocketSession::on_write,
                    shared_from_this()));


}

void WebsocketSession::on_accept(beast::error_code ec)
{
    if(ec)
        return fail(ec, "accept");

    m_state = State::idle; // accept was successful

    do_read();
}

void WebsocketSession::do_read()
{
    // Read a message into our buffer
    m_websocketStream.async_read(
                m_readBuffer,
                beast::bind_front_handler(
                    &WebsocketSession::on_read,
                    shared_from_this()));
}

void WebsocketSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    // This indicates that the websocket_session was closed
    if(ec == websocket::error::closed) {
        logger(Level::info) << "Websocket Session to <" << m_endpoint <<">";
        return;
    }

    if(ec)
        fail(ec, "read");
    else {
        // go on reading
        do_read();
    }
}

void WebsocketSession::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    if(ec)
        return fail(ec, "write");

    // Clear the buffer
    m_writeBuffer.consume(bytes_transferred);

    if (m_state != State::write) {
        logger(Level::warning) << "write finish handler call on wrong state\n";
    }
    m_state = State::idle;
    sendQueued();



}
