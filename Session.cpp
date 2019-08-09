#include "Session.h"

void session::fail(boost::system::error_code ec, char const *what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

void session::run() {
    auto self { shared_from_this() };
    // Perform the SSL handshake
    stream_.async_handshake(
            ssl::stream_base::server,
            [this, self](boost::system::error_code ec) {
                on_handshake(ec);
            });
}

void session::on_handshake(boost::system::error_code ec) {
    if(ec)
        return fail(ec, "handshake");

    do_read();
}

void session::do_read() {
    auto self { shared_from_this() };
    // Read a request

    reqHeader_.body_limit(40*1024*1024); // file size should be 40 MByte at maximum

    http::async_read_header(stream_, buffer_, reqHeader_,
                            [this, self]( boost::system::error_code ec, std::size_t bytes_transferred) {
                                on_read_header(ec, bytes_transferred);
                            });
}

std::string session::generate_unique_name() {

    boost::uuids::random_generator generator;
    boost::uuids::uuid _name = generator();
    std::stringstream file_name_str;
    file_name_str << ServerConstant::fileRootPath <<"/" << _name << ".mp3";
    return file_name_str.str();

}

void session::on_read_header(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == http::error::end_of_stream)
        return do_close();

    if (ec)
        return fail(ec, "read");

    std::cerr << "handle read (async header read)\n";

    auto self { shared_from_this() };

    if (reqHeader_.get().method() == http::verb::put) {

        std::cerr << "transfering data (via PUT) with length: " << reqHeader_.content_length() << "\n";
        std::string file_name  = generate_unique_name();

        reqFile_ = std::make_unique < http::request_parser < http::file_body >> (std::move(reqHeader_));

        std::cerr << "writing file with name <"<<file_name<<"> ... ";
        boost::beast::error_code error_code;
        reqFile_->get().body().open(file_name.c_str(), boost::beast::file_mode::write, error_code);

        if (error_code) {
            std::cerr << "with error: " << error_code << " - returning an error message to sender\n";
            http::response<http::string_body> res{http::status::internal_server_error, reqFile_->get().version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(reqFile_->get().keep_alive());
            res.body() = "An error occurred: 'open file failed for writing'";
            res.prepare_payload();
            return lambda_(std::move(res));
        }

        std::cerr << "started\n";
        // Read a request
        http::async_read(stream_, buffer_, *reqFile_.get(),
                         [this, self, file_name](boost::system::error_code ec,
                                                 std::size_t bytes_transferred) {
                             std::cerr << "finished read <" << file_name << ">\n";
                             http::response <http::empty_body> res{http::status::ok,
                                                                   reqFile_.get()->get().version()};
                             res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                             res.set(http::field::content_type, "audio/mp3");
                             res.content_length(0);
                             res.keep_alive(reqFile_.get()->get().keep_alive());
                             return lambda_(std::move(res));
                         }
        );
        std::cerr << "async read started\n";
    } else {
        req_ = std::make_unique < http::request_parser < http::string_body >> (std::move(reqHeader_));
        // Read a request
        auto& req = *req_.get();
        http::async_read(stream_, buffer_, req ,
                         [this, self](boost::system::error_code ec,
                                      std::size_t bytes_transferred) { on_read(ec, bytes_transferred); });
    }
}

void session::on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if(ec == http::error::end_of_stream)
        return do_close();

    if(ec)
        return fail(ec, "read");

    std::cerr << "handle read (async read)\n";

    // Send the response
    m_requestHandler.handle_request(*doc_root_, *req_.get(), lambda_);
}

void session::on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close) {
    boost::ignore_unused(bytes_transferred);

    if(ec)
        return fail(ec, "write");

    if(close)
    {
        // This means we should close the connection, usually because
        // the response indicated the "Connection: close" semantic.
        return do_close();
    }

    // We're done with the response so delete it
    res_ = nullptr;

    // Read another request
    do_read();
}

session::session(tcp::socket socket, ssl::context &ctx, std::shared_ptr<std::string const> const &doc_root)
        : socket_(std::move(socket))
        , stream_(socket_, ctx)
        , doc_root_(doc_root)
        , lambda_(*this)
{
}

void session::do_close() {

    auto self { shared_from_this() };

    // Perform the SSL shutdown
    stream_.async_shutdown(
            [this, self](boost::system::error_code ec) {
                on_shutdown(ec);
            });
}

void session::on_shutdown(boost::system::error_code ec) {

    if(ec)
        return fail(ec, "shutdown");

    // At this point the connection is closed gracefully
}

session::send_lambda::send_lambda(session &self)
        : self_(self)
{
}
