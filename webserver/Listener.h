#ifndef SERVER_LISTENER_H
#define SERVER_LISTENER_H

#include <iostream>
#include <memory>
#include <functional>
#include <boost/asio.hpp>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

typedef std::function<void (tcp::socket&)> SessionCreatorFunction;

class Listener : public std::enable_shared_from_this<Listener>
{
    tcp::acceptor m_acceptor;
    tcp::socket m_socket;
    SessionCreatorFunction m_sessionCaller;

    void fail_print(boost::system::error_code ec, char const *what);
    void do_accept();
    void on_accept(boost::system::error_code ec);

public:
    Listener(
            boost::asio::io_context& ioc,
            tcp::endpoint endpoint,
            SessionCreatorFunction&& creator);

    ~Listener() {std::cerr << "Listener destructor\n"; }
    void run();
};

#endif //SERVER_LISTENER_H
