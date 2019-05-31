#ifndef SERVER_LISTENER_H
#define SERVER_LISTENER_H

#include <iostream>
#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>

typedef std::function<void (tcp::socket&, ssl::context&)> SessionCreatorFunction;

class Listener : public std::enable_shared_from_this<Listener>
{
    ssl::context& m_ctx;
    tcp::acceptor m_acceptor;
    tcp::socket m_socket;
    SessionCreatorFunction m_sessionCaller;

    void fail_print(boost::system::error_code ec, char const *what);
    void do_accept();
    void on_accept(boost::system::error_code ec);

public:
    Listener(
            boost::asio::io_context& ioc,
            ssl::context& ctx,
            tcp::endpoint endpoint,
            SessionCreatorFunction&& creator);

    ~Listener() {std::cerr << "Listener destructor\n"; }
    void run();
};




#endif //SERVER_LISTENER_H
