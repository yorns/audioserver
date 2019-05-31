#ifndef SERVER_REQUESTHANDLER_H
#define SERVER_REQUESTHANDLER_H

#include <string>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "Player.h"
#include "SimpleDatabase.h"
#include "common/Constants.h"
#include "common/Extractor.h"
#include "common/mime_type.h"

using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
namespace http = boost::beast::http;

extern std::unique_ptr<Player> player;
extern SimpleDatabase database;
extern std::string currentPlaylist;

class RequestHandler {

public:

    // Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
    std::string
    path_cat(
            boost::beast::string_view base,
            boost::beast::string_view path)
    {
        if(base.empty())
            return path.to_string();
        std::string result = base.to_string();
#if BOOST_MSVC
        char constexpr path_separator = '\\';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
        char constexpr path_separator = '/';
        if(result.back() == path_separator)
            result.resize(result.size() - 1);
        result.append(path.data(), path.size());
#endif
        return result;
    }


    http::response<http::string_body> generate_result_packet(http::status status,
                                                             boost::beast::string_view why,
                                                             http::request_parser<http::string_body>& req,
                                                             boost::beast::string_view mime_type = {"text/html"})
    {
        http::response<http::string_body> res{status, req.get().version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type.to_string());
        res.keep_alive(req.get().keep_alive());
        res.body() = why.to_string();
        res.prepare_payload();
        return res;
    }


// This function produces an HTTP response for the given
// req.get().est. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
    template<class Send>
    void handle_request (
            boost::beast::string_view doc_root,
            http::request_parser<http::string_body>& req,
            Send&& send)
    {

        // Make sure we can handle the method
        if( req.get().method() != http::verb::get &&
            req.get().method() != http::verb::head &&
            req.get().method() != http::verb::post)
            return send(generate_result_packet(http::status::bad_request, "Unknown HTTP-method", req));

        // Request path must be absolute and not contain "..".
        if( req.get().target().empty() ||
            req.get().target()[0] != '/' ||
            req.get().target().find("..") != boost::beast::string_view::npos)
            return send(generate_result_packet(http::status::bad_request, "Illegal request-target", req));

        // has this been a post message
        if(req.get().method() == http::verb::post) {
            std::string path = path_cat(doc_root, req.get().target());
            std::cerr << "posted: "+path+"\n";

            auto urlInfo = utility::Extractor::getUrlInformation(path);

            std::string result {"done"};
            if (urlInfo) {
                std::cerr << "url extracted to: "<<urlInfo->command<< " - "<<urlInfo->parameter<<"="<<urlInfo->value<<"\n";

                if (player && urlInfo->command == "player" && urlInfo->parameter == "file") {
                    player->startPlay(ServerConstant::fileRootPath.to_string()+urlInfo->value,"");
                }

                if (player && urlInfo->command == "player" && urlInfo->parameter == "next" && urlInfo->value == "true") {
                    player->next_file();
                }

                if (player && urlInfo->command == "player" && urlInfo->parameter == "prev" && urlInfo->value == "true") {
                    player->prev_file();
                }

                if (player && urlInfo->command == "player" && urlInfo->parameter == "stop" && urlInfo->value == "true") {
                    player->stop();
                }

                if (urlInfo->command == "playlist" && urlInfo->parameter == "create") {
                    std::cout << "create playlist with name <"<<urlInfo->value<<">";
                    if (!database.createPlaylist(urlInfo->value).empty()) {
                        currentPlaylist = urlInfo->value;
                    }
                    else {
                        result = "playlist not new";
                    }
                }

                if (urlInfo->command == "playlist" && urlInfo->parameter == "change") {
                    std::cout << "change to playlist with name <"<<urlInfo->value<<">";

                    if (database.isPlaylist(urlInfo->value)) {
                        currentPlaylist = urlInfo->value;
                    }
                    else {
                        result = "no playlist with name <"+urlInfo->value+">";
                    }
                }

                if (urlInfo->command == "playlist" && urlInfo->parameter == "add") {
                    if (!currentPlaylist.empty() && database.isPlaylist(currentPlaylist)) {
                        std::cout << "add title with name <" << urlInfo->value << "> to playlist";
                        database.addToPlaylist(currentPlaylist, urlInfo->value);
                    }
                    else {
                        result = "cannot add <"+ urlInfo->value +"> to playlist <"+currentPlaylist+">";
                    }
                }
            }
            else {
                result = "request failed";
            }

            nlohmann::json json;
            json["result"] = result;

            return send(generate_result_packet(http::status::ok, json.dump(2), req, "application/json"));
        }

        if(req.get().method() == http::verb::get) {
            std::string path = path_cat(doc_root, req.get().target());

            boost::optional<std::tuple<std::string, SimpleDatabase::DatabaseSearchType>> jsonList = database.findInDatabaseToJson(path);

            if (jsonList) {
                std::cerr << "Database find requested at " + path + "\n";

                if (std::get<SimpleDatabase::DatabaseSearchType>(jsonList.get()) !=
                    SimpleDatabase::DatabaseSearchType::unknown) {

                    return send(generate_result_packet(http::status::ok, std::get<std::string>(jsonList.get()),
                                                       req, "application/json"));
                } else {
                    return send(generate_result_packet(http::status::ok, " unknown find string ", req));
                }
            }
        }

        // Build the path to the requested file
        std::string path = path_cat(doc_root, req.get().target());
        if(req.get().target().back() == '/')
            path.append("index.html");


        // Attempt to open the file
        boost::beast::error_code ec;
        http::file_body::value_type body;
        body.open(path.c_str(), boost::beast::file_mode::scan, ec);

        // Handle the case where the file doesn't exist
        if(ec == boost::system::errc::no_such_file_or_directory)
            return send(generate_result_packet(http::status::not_found, req.get().target(), req));

        // Handle an unknown error
        if(ec)
            return send(generate_result_packet(http::status::internal_server_error, ec.message(), req));

        // Cache the size since we need it after the move
        auto const size = body.size();

        // Respond to HEAD request
        if(req.get().method() == http::verb::head)
        {
            http::response<http::empty_body> res{http::status::ok, req.get().version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, mime_type(path));
            res.content_length(size);
            res.keep_alive(req.get().keep_alive());
            return send(std::move(res));
        }

        // Respond to GET request
        http::response<http::file_body> res{
                std::piecewise_construct,
                std::make_tuple(std::move(body)),
                std::make_tuple(http::status::ok, req.get().version())};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.get().keep_alive());
        return send(std::move(res));
    }


    void handle_put_result(){}

};


#endif //SERVER_REQUESTHANDLER_H
