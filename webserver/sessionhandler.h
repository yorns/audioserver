#ifndef SESSIONHANDLER_H
#define SESSIONHANDLER_H

#include <functional>
#include <string>
#include <boost/beast.hpp>
#include "common/NameGenerator.h"
#include "websocketsession.h"

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

    typedef std::vector<std::tuple<boost::beast::string_view, http::verb, PathCompare, NameGeneratorFunction, UploadFinishedHandler>> FileHandlerList;
    typedef std::vector<std::tuple<boost::beast::string_view, http::verb, PathCompare, RequestHandler>>  StringHandlerList;

    FileHandlerList pathToFileHandler;
    StringHandlerList pathToStringHandler;

    std::weak_ptr<WebsocketSession> m_websocketSession;

    template<class A>
    typename A::const_iterator find(const A& a, const boost::beast::string_view& path, const http::verb& method) const {

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

    bool addUrlHandler(const boost::beast::string_view& path, http::verb method, PathCompare pathCompare, RequestHandler&& handler);
    bool addUploadHandler(const boost::beast::string_view& path, NameGeneratorFunction&& handler, UploadFinishedHandler&& finishHandler);

    bool isUploadFile(const http::request_parser<http::empty_body>& requestHeader) const;
    bool isRestAccesspoint(const http::request_parser<http::empty_body>& requestHeader) const;

    std::string callHandler(http::request_parser<http::string_body>& requestHeader) const;
    bool callFileUploadHandler(http::request_parser<http::file_body>& request, const NameGenerator::GenerationName& name) const;

    NameGenerator::GenerationName getName(http::request_parser<http::empty_body>& requestHeader) const;

    void addWebsocketConnection(std::weak_ptr<WebsocketSession> websocketSession) {
        m_websocketSession = websocketSession;
    }

    void broadcast(const std::string& message) {

    }

};

#endif // SESSIONHANDLER_H
