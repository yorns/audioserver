#include "Session.h"
#include "common/logger.h"
#include "websocketsession.h"

void Session::fail(boost::system::error_code ec, const std::string& what) {
    // some errors are annoying .. remove some until handshake and shutdown is corrected
    if (what != "handshake" && what != "shutdown")
        logger(Level::warning) << what << ": " << ec.message() << "\n";
}

void Session::start() {
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

bool Session::is_unknown_http_method(http::request_parser<http::empty_body>& req) const {
    return req.get().method() != http::verb::get &&
            req.get().method() != http::verb::head &&
            req.get().method() != http::verb::post;
}

bool Session::is_illegal_request_target(http::request_parser<http::empty_body>& req) const {
    return req.get().target().empty() ||
            req.get().target()[0] != '/' ||
            req.get().target().find("..") != boost::beast::string_view::npos;
}

http::response<http::string_body> Session::generate_result_packet(http::status status, boost::beast::string_view why,
                                                                  uint32_t version, bool keep_alive,
                                                                  boost::beast::string_view mime_type )
{
    http::response<http::string_body> res{status, version};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type.to_string());
    res.keep_alive(keep_alive);
    res.body() = why.to_string();
    res.prepare_payload();
    return res;
}

void Session::on_read_header(std::shared_ptr<http::request_parser<http::empty_body>> requestHandler_sp,
                             boost::system::error_code ec, std::size_t bytes_transferred) {

    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == http::error::end_of_stream)
        return do_close();

    if (ec) {
        return fail(ec, "read");
    }

    if (is_unknown_http_method(*requestHandler_sp)) {
        // read until finished
        logger(Level::debug) << "Method unknown\n";
        return answer(generate_result_packet(http::status::bad_request,
                                             "Unknown HTTP-method", requestHandler_sp->get().version(),
                                             requestHandler_sp->get().keep_alive()));
    }

    // Request path must be absolute and not contain "..".
    if (is_illegal_request_target(*requestHandler_sp)) {
        // read until finished
        logger(Level::debug) << "Illegal request\n";
        return answer(generate_result_packet(http::status::bad_request,
                                             "Illegal request-target", requestHandler_sp->get().version(),
                                             requestHandler_sp->get().keep_alive()));
    }

    // See if it is a WebSocket Upgrade
    if( websocket::is_upgrade(requestHandler_sp->get()) &&
        requestHandler_sp->get().target() == ServerConstant::AccessPoints::websocket)
    {
        logger(debug) << "request target is websocket upgrade\n";

        // read full request
        // if so, create a websocket_session by transferring the socket
        auto websocketSession = std::make_shared<WebsocketSession>(std::move(m_socket));
        websocketSession->do_accept(requestHandler_sp->release());

        m_sessionHandler.addWebsocketConnection(websocketSession);

        return;
    }

    if (m_sessionHandler.isUploadFile(*requestHandler_sp)) {

        logger(debug) << "request target is an upload point\n";

        auto name = m_sessionHandler.getName(*requestHandler_sp);

        auto reqFile = std::make_shared < http::request_parser < http::file_body >> (std::move(*requestHandler_sp));

        boost::beast::error_code error_code;
        reqFile->get().body().open(name.filename.c_str(), boost::beast::file_mode::write, error_code);

        if (error_code) {
            answer(generate_result_packet(http::status::internal_server_error,
                                          "An error occurred: 'open file failed for writing'",
                                          reqFile->get().version(), reqFile->get().keep_alive()));
        }
        else {
            auto self { shared_from_this() };

            auto read_done_handler = [this, self, name, reqFile](boost::system::error_code ec,
                    std::size_t bytes_transferred) {
                logger(debug) << "finished read <" << name.unique_id << ">\n";
                boost::ignore_unused(bytes_transferred);
                if (!ec) {
                    m_sessionHandler.callFileUploadHandler(*reqFile, name);
                    answer(generate_result_packet(http::status::ok , "",
                                                  reqFile->get().version(), reqFile->get().keep_alive(),
                                                  "audio/mp3"));
                }
                else {
                    logger(Level::warning) << "could not reade file <" << name.filename << ">\n";
                }
            };

            http::async_read(m_socket, m_buffer, *reqFile.get(), read_done_handler);
        }
        return;
    }

    if (m_sessionHandler.isRestAccesspoint(*requestHandler_sp)) {
       // ownership goes to session handler
        logger(debug) << "request target is a REST accesspoint\n";

        auto requestString = std::make_shared < http::request_parser < http::string_body >> (std::move(*requestHandler_sp));
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

    logger(Level::debug) << "find the file <" << requestHandler_sp->get().target().to_string() << "> on directory\n";

    handle_file_request(m_filePath, requestHandler_sp->get().target().to_string(),
                        requestHandler_sp->get().method(), requestHandler_sp->get().version(),
                        requestHandler_sp->get().keep_alive());
}

void Session::handle_file_request(const std::string &file_root, std::string target, http::verb method, uint32_t version, bool keep_alive) {

    std::string path = file_root + '/' + target;
    if(target.back() == '/')
        path.append("index.html");

    logger(Level::debug) << "file request on: <"<< path << ">\n";

    // read full request (should be empty, however ..)

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


Session::Session(tcp::socket socket, SessionHandler& sessionHandler, std::string&& filePath)
        : m_socket(std::move(socket)), m_sessionHandler(sessionHandler), m_filePath(std::move(filePath))
{
}

void Session::do_close() {

    boost::system::error_code ec;
    // Perform the shutdown
    m_socket.shutdown(boost::asio::socket_base::shutdown_both, ec);

    if(ec)
        return fail(ec, "shutdown");

}

