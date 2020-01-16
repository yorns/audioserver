#include "Session.h"
//#include "common/NameGenerator.h"
#include "common/logger.h"

void session::fail(boost::system::error_code ec, const std::string& what) {
    // some errors are annoying .. remove some until handshake and shutdown is corrected
    if (what != "handshake" && what != "shutdown")
        logger(Level::warning) << what << ": " << ec.message() << "\n";
}

void session::handle_file_request(const std::string &file_root, std::string target, http::verb method, uint32_t version, bool keep_alive) {

    std::string path = file_root + '/' + target;
    if(target.back() == '/')
        path.append("index.html");

    logger(Level::debug) << "file request on: <"<< path << ">\n";

    // Attempt to open the file
    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if(ec == boost::system::errc::no_such_file_or_directory) {
        return answer(generate_result_packet(http::status::not_found, target, version, keep_alive));
    }
    // Handle an unknown error
    if(ec) {
        return answer(generate_result_packet(http::status::internal_server_error, ec.message(), version, keep_alive));
    }

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if(method == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, version};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(keep_alive);
        return answer(std::move(res));
    }

    // Respond to GET request
    http::response<http::file_body> res{
        std::piecewise_construct,
                std::make_tuple(std::move(body)),
                std::make_tuple(http::status::ok, version)};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(keep_alive);
    return answer(std::move(res));
}


void session::start() {
    auto self { shared_from_this() };

    logger(Level::debug) << "start request\n";
    std::shared_ptr<http::request_parser<http::empty_body>> requestHandler(new http::request_parser<http::empty_body>);
    requestHandler->body_limit(std::numeric_limits<std::uint64_t>::max());

    http::async_read_header(m_socket, m_buffer, *requestHandler,
                            [this, self, requestHandler]( boost::system::error_code ec, std::size_t bytes_transferred) {
        logger(Level::debug) << "read header finished\n";
        on_read_header(requestHandler, ec, bytes_transferred);
    });

}

bool session::is_unknown_http_method(http::request_parser<http::empty_body>& req) const {
    return req.get().method() != http::verb::get &&
            req.get().method() != http::verb::head &&
            req.get().method() != http::verb::post;
}

bool session::is_illegal_request_target(http::request_parser<http::empty_body>& req) const {
    return req.get().target().empty() ||
            req.get().target()[0] != '/' ||
            req.get().target().find("..") != boost::beast::string_view::npos;
}

http::response<http::string_body> session::generate_result_packet(http::status status, boost::beast::string_view why, uint32_t version, bool keep_alive, boost::beast::string_view mime_type)
{
    http::response<http::string_body> res{status, version};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type.to_string());
    res.keep_alive(keep_alive);
    res.body() = why.to_string();
    res.prepare_payload();
    return res;
}


void session::on_read_header(std::shared_ptr<http::request_parser<http::empty_body>> requestHandler_sp,
                             boost::system::error_code ec, std::size_t bytes_transferred) {

    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == http::error::end_of_stream)
        return do_close();

    if (ec) {
        logger(Level::warning) << "wwooowww could not read the request! that is really fun!!\n";
        return fail(ec, "read");
    }

    if (is_unknown_http_method(*requestHandler_sp)) {
        logger(Level::debug) << "Method unknown\n";
        return answer(generate_result_packet(http::status::bad_request,
                                             "Unknown HTTP-method", requestHandler_sp->get().version(),
                                             requestHandler_sp->get().keep_alive()));
    }

    // Request path must be absolute and not contain "..".
    if (is_illegal_request_target(*requestHandler_sp)) {
        logger(Level::debug) << "Illegal request\n";
        return answer(generate_result_packet(http::status::bad_request,
                                             "Illegal request-target", requestHandler_sp->get().version(),
                                             requestHandler_sp->get().keep_alive()));
    }


    if (m_sessionHandler.isUploadFile(*requestHandler_sp)) {
        auto name = m_sessionHandler.getName(*requestHandler_sp);

        auto reqFile = std::make_shared < http::request_parser < http::file_body >> (std::move(*requestHandler_sp));
        requestHandler_sp->release();

        boost::beast::error_code error_code;
        reqFile->get().body().open(name.filename.c_str(), boost::beast::file_mode::write, error_code);

        if (error_code) {
            answer(generate_result_packet(http::status::internal_server_error, "An error occurred: 'open file failed for writing'", reqFile->get().version(), reqFile->get().keep_alive()));
        }
        else {
            auto self { shared_from_this() };

            http::async_read(m_socket, m_buffer, *reqFile.get(),
                             [this, self, name, reqFile](boost::system::error_code ec,
                                                     std::size_t bytes_transferred) {
                                 logger(debug) << "finished read <" << name.unique_id << ">\n";
                                 boost::ignore_unused(ec, bytes_transferred);
                                 // handleFileWrite(name.unique_id) .. do this outside:
            //                     std::string cover = id3TagReader::extractCover(name.unique_id);
            //                     database.addToDatabase(name.unique_id, cover);
                                 answer(generate_result_packet(http::status::ok , "",
                                                               reqFile->get().version(), reqFile->get().keep_alive(),
                                                               "audio/mp3"));
                            }
            );
        }
        return;
    }

    if (m_sessionHandler.isRestAccesspoint(*requestHandler_sp)) {
       // ownership goes to session handler
        auto requestString = std::make_shared < http::request_parser < http::string_body >> (std::move(*requestHandler_sp));
        requestHandler_sp->release();

        auto self { shared_from_this() };

        // do full read
        http::async_read(m_socket, m_buffer, *requestString.get(),
                         [this, self, requestString](boost::system::error_code ec,
                         std::size_t bytes_transferred) {
            if (!ec) {
                logger(debug) << "finished read on target <" << requestString->get().target() << ">\n";
                boost::ignore_unused(ec, bytes_transferred);

                // find if this is a rest request, run the handler
                auto reply = m_sessionHandler.callHandler(*requestString);

                // if not a rest request, try find the file
                if (reply.empty()) {
                    answer(generate_result_packet(http::status::not_found,
                                                  requestString->get().target(),
                                                  requestString->get().version(),
                                                  requestString->get().keep_alive()));
                }
                else {
                    answer(generate_result_packet(http::status::ok,
                                                  reply,
                                                  requestString->get().version(),
                                                  requestString->get().keep_alive()));
                }

                logger(Level::info) << "requestString use count: " << requestString.use_count() << "\n";

            }
        });

        logger(Level::info) << "requestHandler_sp use count: " << requestHandler_sp.use_count() << "\n";
        return;
    }

    logger(Level::debug) << "find the file on directory\n";

    handle_file_request(m_filePath, requestHandler_sp->get().target().to_string(),
                        requestHandler_sp->get().method(), requestHandler_sp->get().version(),
                        requestHandler_sp->get().keep_alive());
}



//void session::handle_upload_request()
//{
//    auto self { shared_from_this() };

//    logger(debug) << "transfering data (via POST) with data of length: " << m_reqHeader.content_length() << "\n";
//    std::stringstream audioPath;
//    audioPath << ServerConstant::base_path << "/" << ServerConstant::audioPath;
//    auto namegen = NameGenerator::create(audioPath.str(), ".mp3");

//    m_reqFile = std::make_unique < http::request_parser < http::file_body >> (std::move(m_reqHeader));

//    logger(debug) << "writing file with name <"<<namegen.filename<<"> ... ";

//    boost::beast::error_code error_code;
//    m_reqFile->get().body().open(namegen.filename.c_str(), boost::beast::file_mode::write, error_code);

//    if (error_code) {
//        logger(debug) << "with error: " << error_code << " - returning an error message to sender\n";
//        http::response<http::string_body> res{http::status::internal_server_error, m_reqFile->get().version()};
//        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
//        res.set(http::field::content_type, "text/html");
//        res.keep_alive(m_reqFile->get().keep_alive());
//        res.body() = "An error occurred: 'open file failed for writing'";
//        res.prepare_payload();
//        return m_lambda(std::move(res));
//    }

//    logger(debug) << "started\n";
//    // Read a request
//    http::async_read(m_socket, m_buffer, *m_reqFile.get(),
//                     [this, self, namegen](boost::system::error_code ec,
//                     std::size_t bytes_transferred) {
//        logger(debug) << "finished read <" << namegen.unique_id << ">\n";

//        boost::ignore_unused(ec, bytes_transferred);

//        http::response <http::empty_body> res{http::status::ok,
//                    m_reqFile.get()->get().version()};
//        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
//        res.set(http::field::content_type, "audio/mp3");
//        res.content_length(0);
//        res.keep_alive(m_reqFile.get()->get().keep_alive());
//        std::string cover = id3TagReader::extractCover(namegen.unique_id);
//        database.addToDatabase(namegen.unique_id, cover);
//        return m_lambda(std::move(res));
//    }
//    );
//    //logger(debug) << "async read started\n";
//}

//void session::handle_normal_request()
//{
//    auto self { shared_from_this() };

//    m_req = std::make_unique < http::request_parser < http::string_body >> (std::move(m_reqHeader));
//    // Read a request
//    auto& req = *m_req.get();
//    http::async_read(m_socket, m_buffer, req ,
//                     [this, self](boost::system::error_code ec,
//                                  std::size_t bytes_transferred) { on_read(ec, bytes_transferred); });

//}


//void session::on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
//    boost::ignore_unused(bytes_transferred);

//    // This means they closed the connection
//    if(ec == http::error::end_of_stream)
//        return do_close();

//    if(ec)
//        return fail(ec, "read");

//    //logger(debug) << "handle read (async read)\n";

//    // Send the response
//    RequestHandler requestHandler(this);
//    requestHandler.handle_request(*m_req.get(), m_lambda);
//}

//void session::on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close) {
//    boost::ignore_unused(bytes_transferred);

//    if(ec)
//        return fail(ec, "write");

//    if(close)
//    {
//        // This means we should close the connection, usually because
//        // the response indicated the "Connection: close" semantic.
//        return do_close();
//    }

//    // We're done with the response so delete it
//    m_result = nullptr;

//    auto self { shared_from_this() };

//    // Read another request
//    http::async_read_header(m_socket, m_buffer, m_reqHeader,
//                            [this, self]( boost::system::error_code ec, std::size_t bytes_transferred) {
//                                on_read_header(ec, bytes_transferred);
//                            });
//}

session::session(tcp::socket socket, const SessionHandler& sessionHandler, std::string&& filePath)
        : m_socket(std::move(socket)), m_sessionHandler(sessionHandler), m_filePath(std::move(filePath))
{
}

void session::do_close() {

    boost::system::error_code ec;
    // Perform the shutdown
    m_socket.shutdown(boost::asio::socket_base::shutdown_both, ec);

    if(ec)
        return fail(ec, "shutdown");

}

