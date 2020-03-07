#ifndef SESSIONHANDLER_H
#define SESSIONHANDLER_H

#include <functional>
#include <string>
#include <map>
#include <memory>
#include <boost/beast.hpp>
#include "common/NameGenerator.h"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

class WebsocketSession;

enum class PathCompare {
    exact,
    prefix
};

class SessionHandler {

    using NameGeneratorFunction = std::function<const typename Common::NameGenerator::GenerationName (void)>;
    using RequestHandler = std::function<std::string(const http::request_parser<http::string_body>&)>;
    using UploadFinishedHandler = std::function<bool(const Common::NameGenerator::GenerationName&)>;

    using FileHandlerList = std::vector<std::tuple<std::string_view, http::verb, PathCompare, NameGeneratorFunction, UploadFinishedHandler>>;
    using StringHandlerList = std::vector<std::tuple<std::string_view, http::verb, PathCompare, RequestHandler>>;

    FileHandlerList pathToFileHandler;
    StringHandlerList pathToStringHandler;

    std::map<boost::asio::ip::tcp::endpoint, std::weak_ptr<WebsocketSession>> m_websocketSessionList;

    template<class A>
    typename A::const_iterator find(const A& a, const std::string_view& path, const http::verb& method) const {

        auto handlerIt = std::find_if(std::begin(a),
                                      std::end(a),
                                      [&path, &method] (auto& it)
        {
            const std::string_view& entryPath = std::get<std::string_view>(it);
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

    bool addUrlHandler(const std::string_view& path, http::verb method, PathCompare pathCompare, RequestHandler&& handler);
    bool addUploadHandler(const std::string_view& path, NameGeneratorFunction&& handler, UploadFinishedHandler&& finishHandler);

    [[nodiscard]] bool isUploadFile(const http::request_parser<http::empty_body>& requestHeader) const;
    [[nodiscard]] bool isRestAccesspoint(const http::request_parser<http::empty_body>& requestHeader) const;

    std::string callHandler(http::request_parser<http::string_body>& requestHeader) const;
    bool callFileUploadHandler(http::request_parser<http::file_body>& request, const Common::NameGenerator::GenerationName& name) const;

    Common::NameGenerator::GenerationName getName(http::request_parser<http::empty_body>& requestHeader) const;

    void addWebsocketConnection(std::weak_ptr<WebsocketSession> websocketSession, const boost::asio::ip::tcp::endpoint& endpoint);

    void removeWebsocketConnection(const boost::asio::ip::tcp::endpoint& endpoint);

    void broadcast(const std::string& message);

};

#endif // SESSIONHANDLER_H
