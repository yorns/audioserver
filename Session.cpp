#include "Session.h"

void session::fail(boost::system::error_code ec, char const *what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

void session::run() {
    auto self { shared_from_this() };
    // Perform the SSL handshake
    m_stream.async_handshake(
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

    m_reqHeader.body_limit(40*1024*1024); // file size should be 40 MByte at maximum

    http::async_read_header(m_stream, m_buffer, m_reqHeader,
                            [this, self]( boost::system::error_code ec, std::size_t bytes_transferred) {
                                on_read_header(ec, bytes_transferred);
                            });
}

std::string session::generate_filename(const std::string& uniqueId)
{
    std::stringstream file_name_str;
    file_name_str << ServerConstant::base_path << "/" << ServerConstant::fileRootPath <<"/" << uniqueId << ".mp3";
    return file_name_str.str();

}

std::string session::generate_unique_id() {

    boost::uuids::random_generator generator;
    boost::uuids::uuid _name = generator();

    std::stringstream tmp;
    tmp << _name;
    return tmp.str();
}

void session::handle_upload_request()
{
    auto self { shared_from_this() };

    std::cerr << "transfering data (via POST) with data of length: " << m_reqHeader.content_length() << "\n";
    std::string unique_id = generate_unique_id();
    std::string file_name = generate_filename(unique_id);

    m_reqFile = std::make_unique < http::request_parser < http::file_body >> (std::move(m_reqHeader));

    std::cerr << "writing file with name <"<<file_name<<"> ... ";

    boost::beast::error_code error_code;
    m_reqFile->get().body().open(file_name.c_str(), boost::beast::file_mode::write, error_code);

    if (error_code) {
        std::cerr << "with error: " << error_code << " - returning an error message to sender\n";
        http::response<http::string_body> res{http::status::internal_server_error, m_reqFile->get().version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(m_reqFile->get().keep_alive());
        res.body() = "An error occurred: 'open file failed for writing'";
        res.prepare_payload();
        return m_lambda(std::move(res));
    }

    std::cerr << "started\n";
    // Read a request
    http::async_read(m_stream, m_buffer, *m_reqFile.get(),
                     [this, self, unique_id](boost::system::error_code ec,
                                             std::size_t bytes_transferred) {
                         std::cerr << "finished read <" << unique_id << ">\n";

                         http::response <http::empty_body> res{http::status::ok,
                                                               m_reqFile.get()->get().version()};
                         res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                         res.set(http::field::content_type, "audio/mp3");
                         res.content_length(0);
                         res.keep_alive(m_reqFile.get()->get().keep_alive());
                         std::string cover = id3TagReader::extractCover(unique_id);
                         database.addToDatabase(unique_id, cover);
                         return m_lambda(std::move(res));
                     }
    );
    //std::cerr << "async read started\n";
}

void session::handle_normal_request()
{
    auto self { shared_from_this() };
    m_req = std::make_unique < http::request_parser < http::string_body >> (std::move(m_reqHeader));
    // Read a request
    auto& req = *m_req.get();
    http::async_read(m_stream, m_buffer, req ,
                     [this, self](boost::system::error_code ec,
                                  std::size_t bytes_transferred) { on_read(ec, bytes_transferred); });

}

void session::on_read_header(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == http::error::end_of_stream)
        return do_close();

    if (ec)
        return fail(ec, "read");

    //std::cerr << "handle read (async header read)\n";


    if (m_reqHeader.get().method() == http::verb::post && m_reqHeader.get().target() == "/upload") {
        handle_upload_request();
    } else {
        handle_normal_request();
    }
}

void session::on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if(ec == http::error::end_of_stream)
        return do_close();

    if(ec)
        return fail(ec, "read");

    //std::cerr << "handle read (async read)\n";

    // Send the response
    m_requestHandler.handle_request(*m_doc_root, *m_req.get(), m_lambda);
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
    m_result = nullptr;

    // Read another request
    do_read();
}

session::session(tcp::socket socket, ssl::context &ctx, std::shared_ptr<std::string const> const &doc_root)
        : m_socket(std::move(socket))
        , m_stream(m_socket, ctx)
        , m_doc_root(doc_root)
        , m_lambda(*this)
{
}

void session::do_close() {

    auto self { shared_from_this() };

    // Perform the SSL shutdown
    m_stream.async_shutdown(
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
        : m_self(self)
{
}
