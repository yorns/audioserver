#ifndef SERVER_LISTENER_H
#define SERVER_LISTENER_H

#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include "common/logger.h"

using tcp = boost::asio::ip::tcp;

typedef std::function<void (tcp::socket&)> SessionCreatorFunction;

class Listener : public std::enable_shared_from_this<Listener>
{
    tcp::acceptor m_acceptor;
    tcp::socket m_socket;
    SessionCreatorFunction m_sessionCaller;

    void failPrint(boost::system::error_code ec, char const *what);
    void doAccept();
    void onAccept(boost::system::error_code ec);

public:
    Listener(
            boost::asio::io_context& ioc,
            tcp::endpoint endpoint,
            SessionCreatorFunction&& creator);

    ~Listener() { logger(LoggerFramework::Level::debug) << "Listener destructor\n"; }
    void run();
};

#endif //SERVER_LISTENER_H
