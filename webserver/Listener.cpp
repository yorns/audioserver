#include "Listener.h"

Listener::Listener(boost::asio::io_context &ioc, tcp::endpoint endpoint,
                   SessionCreatorFunction &&creator)
        : m_acceptor(ioc)
        , m_socket(ioc)
        , m_sessionCaller (std::move(creator))
{
    boost::system::error_code ec;

    // Open the acceptor
    std::cout << __FILE__ << ":" << __LINE__ << "> open acceptor\n";
    m_acceptor.open(endpoint.protocol(), ec);

    if(ec) {
        fail_print(ec, "open");
        return;
    }

    // Allow address reuse
    std::cout << __FILE__ << ":" << __LINE__ << "> set reuse option\n";
    m_acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if(ec) {
        fail_print(ec, "set_option");
        return;
    }

    // Bind to the server address
    std::cout << __FILE__ << ":" << __LINE__ << "> bind acceptor to endpoint <"<<endpoint.address() << ":"<<endpoint.port() << "\n";
    m_acceptor.bind(endpoint, ec);
    if(ec) {
        fail_print(ec, "bind");
        return;
    }

    // Start listening for connections
    std::cout << __FILE__ << ":" << __LINE__ << " listen to connection\n";
    m_acceptor.listen( boost::asio::socket_base::max_listen_connections, ec);
    if(ec) {
        fail_print(ec, "listen");
        return;
    }

    std::cout << __FILE__ << ":" << __LINE__ << "> Listener established - waiting for connection requests\n";
}

void Listener::run() {
    if(! m_acceptor.is_open())
        return;
    do_accept();
}

void Listener::do_accept() {

    auto self { shared_from_this() };

    m_acceptor.async_accept( m_socket,
            [this, self](const boost::system::error_code& error)
            {
                on_accept(error);
            } );
}

void Listener::on_accept(boost::system::error_code ec) {

    if(ec) {
        fail_print(ec, "accept");
        return;
    }
    else {
        std::cout << __FILE__ << ":" << __LINE__ << "> Create the session and run it\n";
        m_sessionCaller(m_socket);
    }

    // Accept another connection
    do_accept();
}

void Listener::fail_print(boost::system::error_code ec, char const *what) {
    std::cerr << what << ": " << ec.message() << "\n";
}
