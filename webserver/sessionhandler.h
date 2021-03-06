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
    using VirtualImageHandler = std::function<std::optional<std::vector<char>>(const std::string_view&)>;
    using VirtualAudioHandler = std::function<std::optional<std::string>(const std::string_view&)>;
    using VirtualPlaylistHandler = std::function<std::optional<std::string>(const std::string_view&)>;

    using StringHandlerElement = std::tuple<std::string_view, http::verb, PathCompare, RequestHandler>;
    using FileHandlerList = std::vector<std::tuple<std::string_view, http::verb, PathCompare, NameGeneratorFunction, UploadFinishedHandler>>;
    using StringHandlerList = std::vector<StringHandlerElement>;

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

    VirtualImageHandler m_virtualImageHander;
    VirtualAudioHandler m_virtualAudioHander;
    VirtualPlaylistHandler m_virtualPlaylistHander;

public:

    /*!
     * \brief addUrlHandler gives the method to define an access point for a REST url
     * \param path is the url path of this request ( e.g. "/database" )
     * \param method specifies the access method in form of http::verb from boost::beast (e.g. boost::beast::http::verb::get)
     * \param pathCompare is the accesspoint defined to be fully exacted or are there any accesspoints below,
     * that will be extracted within the handler. (in case the "/database" is only accessed by type value pairs (?type=value),
     * the this pathCompare is _exact_. Else (e.g. "/database/mybase", "/database/yourbase") is handled with only one handler,
     * this PathCompare variable is set to prefix.
     * \param handler Handler to be invoked when REST accesspoint is met.
     * \return true, if REST accesspoint is met, false if not
     */
    bool addUrlHandler(const std::string_view& path, http::verb method, PathCompare pathCompare, RequestHandler&& handler);
    bool addUploadHandler(const std::string_view& path, NameGeneratorFunction&& handler, UploadFinishedHandler&& finishHandler);

    bool addVirtualImageHandler(VirtualImageHandler&& handler) { m_virtualImageHander = std::move(handler); return true; }
    bool addVirtualAudioHandler(VirtualAudioHandler&& handler) { m_virtualAudioHander = std::move(handler); return true; }
    bool addVirtualPlaylistHandler(VirtualPlaylistHandler&& handler) { m_virtualPlaylistHander = std::move(handler); return true; }

    [[nodiscard]] bool isUploadFile(const http::request_parser<http::empty_body>& requestHeader) const;
    [[nodiscard]] bool isRestAccesspoint(const http::request_parser<http::empty_body>& requestHeader) const;
    [[nodiscard]] std::optional<std::vector<char>> getVirtualImage(const std::string_view& target) const {
        if (!m_virtualImageHander)
            return std::nullopt;
        return m_virtualImageHander(target); // only delegation
    }
    [[nodiscard]] std::optional<std::string> getVirtualAudio(const std::string_view& target) const {
        if (!m_virtualAudioHander)
            return std::nullopt;
        return m_virtualAudioHander(target); // only delegation
    }
    [[nodiscard]] std::optional<std::string> getVirtualPlaylist(const std::string_view& target) const {
        if (!m_virtualPlaylistHander)
            return std::nullopt;
        return m_virtualPlaylistHander(target); // only delegation
    }

    std::string callHandler(http::request_parser<http::string_body>& requestHeader) const;
    bool callFileUploadHandler(http::request_parser<http::file_body>& request, const Common::NameGenerator::GenerationName& name) const;

    std::string generateRESTInterfaceDocumentation();

    Common::NameGenerator::GenerationName getName(http::request_parser<http::empty_body>& requestHeader) const;

    void addWebsocketConnection(std::weak_ptr<WebsocketSession> websocketSession, const boost::asio::ip::tcp::endpoint& endpoint);

    void removeWebsocketConnection(const boost::asio::ip::tcp::endpoint& endpoint);

    void broadcast(const std::string& message);

};

#endif // SESSIONHANDLER_H
