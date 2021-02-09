#include "Session.h"

#include <boost/regex.hpp>
#include <boost/beast/core/file.hpp>

#include "common/mime_type.h"
#include "common/Constants.h"
#include "common/logger.h"



void Session::fail(boost::system::error_code ec, const std::string& what) {
    // some errors are annoying .. remove some until handshake and shutdown is corrected
    if (what != "handshake" && what != "shutdown")
        logger(Level::warning) << "<" << m_runID << ">" << what << ": " << ec.message() << "\n";
}

void Session::start() {
    auto self { shared_from_this() };

    logger(Level::debug) << "<" << m_runID << "> " << "request from client started\n";
    std::shared_ptr<http::request_parser<http::empty_body>> requestHandler(new http::request_parser<http::empty_body>);
    requestHandler->body_limit(std::numeric_limits<std::uint64_t>::max());

    http::async_read_header(m_socket, m_buffer, *requestHandler,
                            [this, self, requestHandler]( boost::system::error_code ec, std::size_t bytes_transferred) {
        logger(Level::debug) << "<" << m_runID << "> " << "read header finished\n";
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
            req.get().target().find("/../") != std::string_view::npos;
}

//http::response<http::string_body> Session::generate_result_packet(http::status status, std::string_view why,
//                                                                  uint32_t version, bool keep_alive,
//                                                                  std::string_view mime_type )
//{
//    http::response<http::string_body> res{status, version};
//    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
//    res.set(http::field::content_type, mime_type);
//    res.keep_alive(keep_alive);
//    res.body() = why;
//    res.prepare_payload();
//    return res;
//}

std::shared_ptr<http::response<http::string_body>> Session::generate_result_packet(http::status status, std::string_view why,
                                                                  uint32_t version, bool keep_alive,
                                                                  std::string_view mime_type )
{
    auto res = std::make_shared<http::response<http::string_body>>(status, version);
    res->set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res->set(http::field::content_type, mime_type);
    res->keep_alive(keep_alive);
    res->body() = why;
    res->prepare_payload();
    return res;
}


void Session::on_read_header(std::shared_ptr<http::request_parser<http::empty_body>> requestHandler_sp,
                             boost::system::error_code ec, std::size_t bytes_transferred) {

    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == http::error::end_of_stream) {
        logger(Level::warning) << "<" << m_runID << "> " << "End of stream\n";
        return do_close();
    }

    if (ec) {
        return fail(ec, "read");
    }

    logger(Level::info) << requestHandler_sp->get() <<"\n";

    auto rangeString = requestHandler_sp->get()[http::field::range];
    //auto encoding = requestHandler_sp->get()[http::field::accept_encoding];

    std::optional<http_range> rangeData;
    if (!rangeString.empty()) {
        logger(Level::info) << "range is <"<< rangeString <<">\n";
        rangeData = http_range(std::string(rangeString));
        if (rangeData) {
            logger(Level::info) << "range is <"<< rangeData->from << " to "<< rangeData->to <<">\n";
        }
    }
    else {
        logger(Level::info) << "no range set\n";
    }

    if (is_unknown_http_method(*requestHandler_sp)) {
        // read until finished
        logger(Level::debug) << "<" << m_runID << "> " << "Method unknown\n";
        answer(generate_result_packet(http::status::bad_request,
                                             "Unknown HTTP-method", requestHandler_sp->get().version(),
                                             requestHandler_sp->get().keep_alive()));
        return;
    }

    // Request path must be absolute and not contain "..".
    if (is_illegal_request_target(*requestHandler_sp)) {
        // read until finished
        logger(Level::debug) << "<" << m_runID << "> " << "Illegal request\n";
        answer(generate_result_packet(http::status::bad_request,
                                             "Illegal request-target", requestHandler_sp->get().version(),
                                             requestHandler_sp->get().keep_alive()));
        return;
    }

    logger(Level::debug) << "<" << m_runID << "> " << "reading target: " << requestHandler_sp->get().target() << "\n";

    if (m_sessionHandler.isUploadFile(*requestHandler_sp)) {

        logger(Level::debug) << "<" << m_runID << "> " << "request target is an upload point\n";

        auto name = m_sessionHandler.getName(*requestHandler_sp);

        auto reqFile = std::make_shared < http::request_parser < http::file_body >> (std::move(*requestHandler_sp));

        boost::beast::error_code error_code;
        reqFile->get().body().open(name.filename.c_str(), boost::beast::file_mode::write, error_code);

        if (error_code) {
            answer(generate_result_packet(http::status::internal_server_error,
                                          "An error occurred: " + error_code.message(),
                                          (*reqFile).get().version(), reqFile->get().keep_alive()));

        }
        else {                       

            auto self { shared_from_this() };

            auto read_done_handler = [this, self, name, reqFile](boost::system::error_code ec,
                        std::size_t bytes_transferred) {
                logger(Level::debug) << "<" << m_runID << "> " << "finished read <" << name.unique_id << ">\n";
                boost::ignore_unused(bytes_transferred);
                if (!ec) {
                    m_sessionHandler.callFileUploadHandler(*reqFile, name);
                    answer(generate_result_packet(http::status::ok , "",
                                                  reqFile->get().version(), reqFile->get().keep_alive(),
                                                  "audio/mp3"));
                    return;

                }
                else {
                    logger(Level::warning) << "<" << m_runID << "> " << "could not reade file <" << name.filename << ">\n";
                }
            };

            http::async_read(m_socket, m_buffer, *reqFile.get(), read_done_handler);
        }
        return;
    }

    if (m_sessionHandler.isRestAccesspoint(*requestHandler_sp)) {

        // ownership goes to session handler
        logger(Level::debug) << "<" << m_runID << "> " << "request target is a REST accesspoint\n";

        auto requestString = std::make_shared < http::request_parser < http::string_body >> (std::move(*requestHandler_sp));
        auto self { shared_from_this() };

        // do full read
        http::async_read(m_socket, m_buffer, *requestString,
                         [this, self, requestString](boost::system::error_code ec,
                         std::size_t bytes_transferred) {
            if (!ec) {
                logger(Level::debug) << "<" << m_runID << "> " << "finished read on target <" << requestString->get().target() << ">\n";
                boost::ignore_unused(ec, bytes_transferred);

                // find if this is a rest request, run the handler
                auto reply = m_sessionHandler.callHandler(*requestString);

                // if not a rest request, try find the file
                if (reply.empty()) {
                    logger(Level::debug) << "no reply set - answer with status <not found>\n";
                    answer(generate_result_packet(http::status::not_found,
                                                  requestString->get().target(),
                                                  requestString->get().version(),
                                                  requestString->get().keep_alive()));
                }
                else {
                    logger(Level::debug) << "reply set - answer with status <ok>\n";
                    answer(generate_result_packet(http::status::ok,
                                                  reply,
                                                  requestString->get().version(),
                                                  requestString->get().keep_alive()));

                }

            }
        });

        return;
    }

    auto requestString = std::make_shared < http::request_parser < http::string_body >> (std::move(*requestHandler_sp));
    auto self { shared_from_this() };

    logger(Level::debug) << "<" << m_runID << "> " << "do full read\n";
    http::async_read(m_socket, m_buffer, *requestString,
                     [this, self, requestString, rangeData](boost::system::error_code ec,
                     std::size_t bytes_transferred) {

        if (!ec) {
            logger(Level::debug) << "<" << m_runID << "> " << "finished ordinary read on target <" << requestString->get().target() << ">\n";
            boost::ignore_unused(ec, bytes_transferred);

            // See if it is a WebSocket Upgrade
            if( websocket::is_upgrade(requestString->get()) &&
                    requestString->get().target() == ServerConstant::AccessPoints::websocket)
            {
                logger(Level::debug) << "<" << m_runID << "> " << "request target is websocket upgrade\n";

                // read full request
                // if so, create a websocket_session by transferring the socket
                auto websocketSession = std::make_shared<WebsocketSession>(std::move(m_socket), m_sessionHandler);
                websocketSession->do_accept(requestString->release());

                return;
            }

            logger(Level::debug) << "<" << m_runID << "> " << "try to find the file <" << requestString->get().target() << "> on directory\n";

            handle_file_request(std::string(requestString->get().target()),
                                requestString->get().method(), requestString->get().version(),
                                requestString->get().keep_alive(), rangeData);

        }
        else {
            logger(Level::warning) << "<" << m_runID << "> " << "failed to read: "<< ec << "\n";
            // todo: answer with something
        }
    });

}

void Session::handle_regular_file_request(std::string target, http::verb method, uint32_t version, bool keep_alive, std::optional<http_range> rangeData) {

    std::string path;
    if (target.substr(0,7) == "file://") {
        path = target.substr(7);
    }
    else {
        path = m_filePath + '/' + target;
    }
    if(target.back() == '/')
        path.append("index.html");

    // Attempt to open the file
    boost::beast::error_code ec;
    http::file_body::value_type body;
    //http::file_body::value_type body;

#ifdef WITH_BOOST_BEAST_PARTIAL_FILE
    bool with_range { rangeData };
#else
#warning please apply boost beast partial file patch
    logger(LoggerFramework::Level::warning) << "please apply partial file patch to boost beast\n";
    bool with_range { false };
#endif

    body.open(path.c_str(), boost::beast::file_mode::scan, ec);
    // Cache the size since we need it after the move
    auto const file_size = body.size();

    if (rangeData && rangeData->to == 0)
        rangeData->to = file_size - 1;

    // Handle the case where the file doesn't exist
    if((ec == boost::system::errc::no_such_file_or_directory &&
        ( method == http::verb::get || method == http::verb::head))) {

        // this file is nowhere, send unknown file instead
        logger(Level::warning) << "requested regular file <"<< path <<"> not found\n";
        answer(generate_result_packet(http::status::not_found, target, version, keep_alive));
        return;
    }

    // Handle an unknown error on file search
    if(ec || ( method != http::verb::get && method != http::verb::head)) {
        logger(Level::warning) << "requested regular file <"<< path <<"> read failed\n";
        answer(generate_result_packet(http::status::internal_server_error, ec.message(), version, keep_alive));
        return;
    }

    // Respond to HEAD request
    if(method == http::verb::head)
    {
        logger(Level::debug) << "returning header for file <"<< path <<">\n";

        auto status = with_range?http::status::partial_content:http::status::ok;
        http::response<http::empty_body> res{status, version};
        if (with_range) {
            auto rangeString = "bytes "+std::to_string(rangeData->from)+"-"+std::to_string(rangeData->to)+"/"+std::to_string(file_size);
            res.set(http::field::content_range, rangeString);
            res.content_length(rangeData->to-rangeData->from + 1);
        }
        else {
            res.content_length(file_size);
        }
        res.set(http::field::accept_ranges, "bytes");
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.set(http::field::content_encoding, "identity");
        res.keep_alive(keep_alive);
        answer(std::move(res));
        return;
    }


    body.close();
    if (with_range) {
#ifdef WITH_BOOST_BEAST_PARTIAL_FILE
        body.open(path.c_str(), boost::beast::file_mode::read, ec, rangeData->from, rangeData->to);
#else
        body.open(path.c_str(), boost::beast::file_mode::read, ec);
#endif
    }
      else
        body.open(path.c_str(), boost::beast::file_mode::read, ec);

    auto const size = body.size();

    logger(Level::info) << "returning reguar file <"<< path <<">\n";
    // Respond to GET request
    auto status = with_range?http::status::partial_content:http::status::ok;

    std::shared_ptr<http::response<http::file_body>> res
            = std::make_shared<http::response<http::file_body>>(std::piecewise_construct,
                                                                   std::make_tuple(std::move(body)),
                                                                   std::make_tuple(status, version));


    if (with_range) {
        auto rangeString = "bytes "+std::to_string(rangeData->from)+"-"+std::to_string(rangeData->to)+"/"+std::to_string(file_size);
        logger(LoggerFramework::Level::info) << "range string <" << rangeString << ">\n";
        res->set(http::field::content_range, rangeString);
    }

    res->content_length(size);

    res->set(http::field::accept_ranges, "bytes");
    res->set(http::field::content_encoding, "identity");
    res->set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res->set(http::field::content_type, mime_type(path));
    res->keep_alive(keep_alive);
    answer(res);
    return;

}

void Session::handle_file_request(std::string target, http::verb method, uint32_t version, bool keep_alive, std::optional<http_range> rangeData) {

//    std::string path = m_filePath + '/' + target;
//    if(target.back() == '/')
//        path.append("index.html");

//    logger(Level::debug) << "<" << m_runID << "> " << "file request on: <"<< path << ">\n";

//    if (path.find("index.html") != std::string::npos) {
//        logger(Level::info) << "index.html requested\n";
//    }

    // try finding file within the virtual/cache filesystem
    if(method == http::verb::get || method == http::verb::head) {

        if ( auto virtualData = m_sessionHandler.getVirtualImage(target) ) {
            logger(Level::debug) << "virtual image file found as <"<<target<<">\n";
            // Cache the size since we need it after the move
            auto const size = virtualData->size();
            http::response<http::vector_body<char>> res {
                std::piecewise_construct,
                        std::make_tuple(std::move(*virtualData)),
                        std::make_tuple(http::status::ok, version)};
            res.set(http::field::content_type, mime_type(target));
            res.content_length(size);
            res.keep_alive(keep_alive);
            answer(std::move(res));
            return;
        }

        if (auto virtualData = m_sessionHandler.getVirtualAudio(target) ) {
            logger(Level::debug) << "virtual audio file found as <"<< *virtualData <<">\n";
            handle_regular_file_request(*virtualData, method, version, keep_alive, rangeData);
            return;
        }

        if (auto virtualData = m_sessionHandler.getVirtualPlaylist(target) ) {
            auto const size = virtualData->size();
            http::response<http::string_body> res { http::status::ok, version };
            res.set(http::field::content_type, mime_type(target));
            res.content_length(size);
            res.keep_alive(keep_alive);
            res.body() = *virtualData;
            answer(std::move(res));
            return;
        }

    }

    handle_regular_file_request(target, method, version, keep_alive, rangeData);

}


Session::Session(tcp::socket socket, SessionHandler& sessionHandler, std::string&& filePath, uint32_t runId)
        : m_socket(std::move(socket)), m_sessionHandler(sessionHandler), m_filePath(std::move(filePath)), m_runID(runId)
{
}

void Session::do_close() {

    boost::system::error_code ec;
    // Perform the shutdown
    m_socket.shutdown(boost::asio::socket_base::shutdown_both, ec);

    if(ec)
        return fail(ec, "shutdown");

}

