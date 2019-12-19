#include "Session.h"
#include "common/NameGenerator.h"
#include "id3tagreader/id3TagReader.h"

void session::fail(boost::system::error_code ec, const std::string& what) {
    // some errors are annoying .. remove some until handshake and shutdown is corrected
    if (what != "handshake" && what != "shutdown")
    std::cerr << what << ": " << ec.message() << "\n";
}

void session::run() {
    auto self { shared_from_this() };

    m_reqHeader.body_limit(std::numeric_limits<std::uint64_t>::max());
    http::async_read_header(m_socket, m_buffer, m_reqHeader,
                            [this, self]( boost::system::error_code ec, std::size_t bytes_transferred) {
                                on_read_header(ec, bytes_transferred);
                            });

}

void session::handle_upload_request()
{
    auto self { shared_from_this() };

    std::cerr << "transfering data (via POST) with data of length: " << m_reqHeader.content_length() << "\n";
    std::stringstream audioPath;
    audioPath << ServerConstant::base_path << "/" << ServerConstant::audioPath;
    auto namegen = NameGenerator::create(audioPath.str(), ".mp3");

    m_reqFile = std::make_unique < http::request_parser < http::file_body >> (std::move(m_reqHeader));

    std::cerr << "writing file with name <"<<namegen.filename<<"> ... ";

    boost::beast::error_code error_code;
    m_reqFile->get().body().open(namegen.filename.c_str(), boost::beast::file_mode::write, error_code);

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
    http::async_read(m_socket, m_buffer, *m_reqFile.get(),
                     [this, self, namegen](boost::system::error_code ec,
                                             std::size_t bytes_transferred) {
                         std::cerr << "finished read <" << namegen.unique_id << ">\n";

                         boost::ignore_unused(ec, bytes_transferred);

                         http::response <http::empty_body> res{http::status::ok,
                                                               m_reqFile.get()->get().version()};
                         res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                         res.set(http::field::content_type, "audio/mp3");
                         res.content_length(0);
                         res.keep_alive(m_reqFile.get()->get().keep_alive());
                         std::string cover = id3TagReader::extractCover(namegen.unique_id);
                         database.addToDatabase(namegen.unique_id, cover);
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
    http::async_read(m_socket, m_buffer, req ,
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
    m_requestHandler.handle_request(*m_req.get(), m_lambda);
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

    auto self { shared_from_this() };

    // Read another request
    http::async_read_header(m_socket, m_buffer, m_reqHeader,
                            [this, self]( boost::system::error_code ec, std::size_t bytes_transferred) {
                                on_read_header(ec, bytes_transferred);
                            });
}

session::session(tcp::socket socket)
        : m_socket(std::move(socket))
        , m_lambda(*this)
{
}

void session::do_close() {

    boost::system::error_code ec;
    // Perform the shutdown
    m_socket.shutdown(boost::asio::socket_base::shutdown_both, ec);

    if(ec)
        return fail(ec, "shutdown");

}

session::send_lambda::send_lambda(session &self)
        : m_self(self)
{
}
