#ifndef SERVER_SESSION_H
#define SERVER_SESSION_H

#include <boost/optional/optional_io.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <boost/filesystem.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <tuple>
#include "nlohmann/json.hpp"
#include "common/Constants.h"
#include "common/logger.h"
#include "common/NameGenerator.h"
#include "common/mime_type.h"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

enum class PathCompare {
    exact,
    prefix
};

class SessionHandler {

    typedef std::function<const typename NameGenerator::GenerationName (void)> NameGeneratorFunction;
    typedef std::function<std::string(const http::request_parser<http::string_body>&)> RequestHandler;
    typedef std::function<bool(const NameGenerator::GenerationName&)> UploadFinishedHandler;

    typedef std::vector<std::tuple<boost::beast::string_view, http::verb, NameGeneratorFunction, UploadFinishedHandler>> FileHandlerList;
    typedef std::vector<std::tuple<boost::beast::string_view, http::verb, PathCompare, RequestHandler>>  StringHandlerList;

    FileHandlerList pathToFileHandler;
    StringHandlerList pathToStringHandler;

    template<class A>
    typename A::const_iterator find(A& a, const boost::beast::string_view& path, const http::verb& method) const {

        auto handlerIt = std::find_if(std::begin(a),
                                      std::end(a),
                                      [&path, &method] (auto& it)
        {
            const boost::beast::string_view& entryPath = std::get<boost::beast::string_view>(it);
            auto wipedPath = path.substr(0, path.find_first_of('?'));

            if (std::get<PathCompare>(it) == PathCompare::exact) {
                return entryPath == wipedPath &&
                       std::get<http::verb>(it) == method;
            }
            else {
                return wipedPath.size() >= entryPath.size() &&
                       wipedPath.substr(0,entryPath.size()) == entryPath &&
                       std::get<http::verb>(it) == method;
            }
        });

        return handlerIt;
    }


public:

    bool addUrlHandler(const boost::beast::string_view& path, http::verb method, PathCompare pathCompare, RequestHandler&& handler)
    {
        auto handlerIt = find(pathToStringHandler, path, method);

        if (handlerIt != pathToStringHandler.end() && pathCompare == std::get<PathCompare>(*handlerIt)) {
            logger(Level::warning) << "path is occupied and will be replaces (" << std::get<boost::beast::string_view>(*handlerIt).to_string() << ")\n";
        }

        auto indexTuple = std::make_tuple(path, method, pathCompare, std::move(handler));

        pathToStringHandler.emplace_back(indexTuple);
        return true;
    }

    bool addNameGeratorForUpload(const boost::beast::string_view& path, NameGeneratorFunction&& handler, UploadFinishedHandler&& finishHandler)
    {

        http::verb method { http::verb::put };

        auto handlerIt = find(pathToFileHandler, path, method);
        auto newTuple = std::make_tuple(path, method, std::move(handler), std::move(finishHandler));

        if (handlerIt != pathToFileHandler.end()) {
            logger(Level::warning) << "path is occupied and will be replaces (" << std::get<boost::beast::string_view>(*handlerIt).to_string() << ")\n";
            // TODO: replace needs to be implemented .. easy first
            pathToFileHandler.erase(handlerIt);
        }

        pathToFileHandler.emplace_back(newTuple);

        return true;
    }

    std::string callHandler(http::request_parser<http::string_body>& requestHeader) const {

        const boost::beast::string_view& path = requestHeader.get().target();
        const http::verb& method = requestHeader.get().method();

        auto handlerIt = find(pathToStringHandler, path, method);

        if (handlerIt != pathToStringHandler.end()) {
            auto& handler = std::get<RequestHandler>(*handlerIt);
            return handler(requestHeader);
        }

        return "";
    }

    bool isUploadFile(const http::request_parser<http::empty_body>& requestHeader) const {
        const boost::beast::string_view& path = requestHeader.get().target();
        const boost::beast::http::verb& method = requestHeader.get().method();
        return find(pathToFileHandler, path, method) != pathToFileHandler.end();
    }

    bool isRestAccesspoint(const http::request_parser<http::empty_body>& requestHeader) const {
        const boost::beast::string_view& path = requestHeader.get().target();
        const boost::beast::http::verb& method = requestHeader.get().method();
        return find(pathToStringHandler, path, method) != pathToStringHandler.end();
    }

    bool callFileUploadHandler(http::request_parser<http::file_body>& request, const NameGenerator::GenerationName& name) const {

        const boost::beast::string_view& path = request.get().target();
        const http::verb& method = request.get().method();

        auto handlerIt = find(pathToStringHandler, path, method);

        if (handlerIt != pathToStringHandler.end()) {
            auto& handler = std::get<UploadFinishedHandler>(*handlerIt);
            return handler(name);
        }

        return false;

    }

    NameGenerator::GenerationName getName(http::request_parser<http::empty_body>& requestHeader) const {

        const boost::beast::string_view& path = requestHeader.get().target();
        const boost::beast::http::verb& method = requestHeader.get().method();

        auto fileIt = find(pathToFileHandler, path, method);
        if (fileIt != pathToFileHandler.end()) {
            return std::get<NameGeneratorFunction>(*fileIt)();
        }

        return NameGenerator::GenerationName();
    }


};

// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session>
{

    // Report a failure
    void fail(boost::system::error_code ec, const std::string& what);

    tcp::socket m_socket;
    boost::beast::flat_buffer m_buffer;

    const SessionHandler& m_sessionHandler;
    const std::string m_filePath;

    void handle_file_request(const std::string& file_root, std::string target, http::verb method, uint32_t version, bool keep_alive);

    void returnMessage();

    void on_read_header(std::shared_ptr<http::request_parser<http::empty_body>> requestHandler, boost::system::error_code ec, std::size_t bytes_transferred);
    void on_read( boost::system::error_code ec, std::size_t bytes_transferred);
    void on_write( boost::system::error_code ec, std::size_t bytes_transferred, bool close);
    void do_close();

    bool is_unknown_http_method(http::request_parser<http::empty_body>& req) const;
    bool is_illegal_request_target(http::request_parser<http::empty_body>& req) const;

    http::response<http::string_body> generate_result_packet(http::status status,
                                                             boost::beast::string_view why,
                                                             uint32_t version,
                                                             bool keep_alive,
                                                             boost::beast::string_view mime_type = {"text/html"});

    template <class Body>
    void answer(http::response<Body>&& response) {
        auto self { shared_from_this() };
        // do answer synchron, so we do not need to keep the resonse message until end of connection
        http::write(m_socket, response);
        do_close();
        logger(Level::debug) << "resonse send without an error\n";
    }

public:

    explicit session( tcp::socket socket, const SessionHandler& sessionHandler, std::string&& filePath);
    session() = delete;

    session(const session& ) = delete;
    session(session&& ) = delete;

    virtual ~session() = default;

    void start();


};



#endif //SERVER_SESSION_H
