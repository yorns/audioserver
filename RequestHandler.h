#ifndef SERVER_REQUESTHANDLER_H
#define SERVER_REQUESTHANDLER_H

#include <string>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <nlohmann/json.hpp>
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

    std::string playerAccess(const utility::Extractor::UrlInformation& urlInfo) {
        if ( urlInfo->parameter == ServerConstant::Command::play &&
             urlInfo->value == ServerConstant::Value::_true) {
            player->startPlay(ServerConstant::playlistRootPath.to_string() + "/" + currentPlaylist + ".m3u", "");
//            player->startPlay(ServerConstant::playlistRootPath.to_string() + "/" + urlInfo->value + ".m3u", "");
            return "ok";
        }

        if (urlInfo->parameter == ServerConstant::Parameter::Play::next &&
                urlInfo->value == ServerConstant::Value::_true) {
            player->next_file();
            return "ok";
        }

        if (urlInfo->parameter == ServerConstant::Parameter::Play::previous &&
                urlInfo->value == ServerConstant::Value::_true) {
            player->prev_file();
            return "ok";
        }

        if (urlInfo->parameter == ServerConstant::Parameter::Play::stop &&
                urlInfo->value == ServerConstant::Value::_true) {
            player->stop();
            return "ok";
        }
        return "player command unknown";
    }

    std::string playlistAccess(const utility::Extractor::UrlInformation& urlInfo) {

        if (urlInfo->parameter == ServerConstant::Command::create) {
            std::cout << "create playlist with name <" << urlInfo->value << ">";
            std::string ID = database.createPlaylist(urlInfo->value);
            if (!ID.empty()) {
                currentPlaylist = urlInfo->value;
                database.writeChangedPlaylists(ServerConstant::playlistRootPath.to_string());
                return "{\"result\": \"ok\"}";
            } else {
                return "{\"result\": \"playlist not new\"}";
            }
        }

        if (urlInfo->parameter == ServerConstant::Command::change) {

            if (database.isPlaylist(urlInfo->value)) {
                std::cout << "changed to playlist with name <" << urlInfo->value << ">\n";
                currentPlaylist = database.getNameFromHumanReadable(urlInfo->value);
                std::cout << "current playlist is <" << currentPlaylist << ">\n";
                return "{\"result\": \"ok\"}";
            } else {
                std::cout << "playlist with name <" << urlInfo->value << "> not found\n";
                return "{\"result\": \"playlist not found\"}";
            }
        }

        if (urlInfo->parameter == ServerConstant::Command::add) {
            if (!currentPlaylist.empty() && database.isPlaylistID(currentPlaylist)) {
                std::cout << "add title with name <" << urlInfo->value << "> to playlist <"<<currentPlaylist<<">\n";
                database.addToPlaylistID(currentPlaylist, urlInfo->value);
                database.writeChangedPlaylists(ServerConstant::playlistRootPath.to_string());
                return "{\"result\": \"ok\"}";
            } else {
                return "{\"result\": \"cannot add <\" + urlInfo->value + \"> to playlist <\" + currentPlaylist + \">}";
            }
        }

        if (urlInfo->parameter == ServerConstant::Command::show) {
            std::cout << "show playlist <" << urlInfo->value << ">\n";
            if ((urlInfo->value.empty() && !currentPlaylist.empty()) || database.isPlaylist(urlInfo->value)) {
                std::cout << "playlist is set\n";
                std::string playlistName(currentPlaylist);
                std::cout << "playlist is <"<<playlistName<<">\n";
                if (!urlInfo->value.empty())
                    playlistName = database.getNameFromHumanReadable(urlInfo->value);
                auto list = database.showPlaylist(playlistName);
                if (list.empty())
                    std::cout << "playlist is empty\n";
                nlohmann::json json;
                for(auto item : list) {
                    std::cerr << item <<"\n";
                    auto entry = database.findInDatabase(item,SimpleDatabase::DatabaseSearchType::uid);
                    if (entry.size() != 1) {
                        std::cerr << "ERROR: could not find playlist file information for <" << item << ">\n";
                        continue;
                    }
                    nlohmann::json jentry;
                    jentry[ServerConstant::Parameter::Database::uid.to_string()] = entry.front().uid;
                    jentry[ServerConstant::Parameter::Database::interpret.to_string()] = entry.front().performer_name;
                    jentry[ServerConstant::Parameter::Database::album.to_string()] = entry.front().album_name;
                    jentry[ServerConstant::Parameter::Database::titel.to_string()] = entry.front().titel_name;
                    jentry["trackNo"] = entry.front().track_no;
                    json.push_back(jentry);
                }
                std::cout << json.dump(2);
                return json.dump(2);
            } else {
                return "no playlist found for <" + urlInfo->value + ">";
            }
        }

        if (urlInfo->parameter == ServerConstant::Command::showLists) {
            std::cout << "show all playlists\n";
            std::vector<std::pair<std::string, std::string>> lists = database.showAllPlaylists();
            if (!lists.empty()) {
                std::cout << "playlists are \n";
                nlohmann::json json;
                nlohmann::json json1;
                try {
                    for (auto item : lists) {
                        std::cerr << item.first << " - " << item.second << "\n";
                        nlohmann::json jentry;
                        jentry[ServerConstant::Parameter::Database::uid.to_string()] = item.first;
                        jentry[ServerConstant::Parameter::Database::playlist.to_string()] = item.second;
                        json1.push_back(jentry);
                    }
                    std::cout << "finished loop\n";
                    json["playlists"] = json1;
                    json["actualPlaylist"] = database.getHumanReadableName(currentPlaylist);
                } catch (std::exception e) {
                    std::cerr << "error: " << e.what() << "\n";
                }
                return json.dump(2);
            } else {
                return "no playlist available";
            }
        }

        return "playlist command not found";
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
                std::cerr << "url extracted to: "<<urlInfo->command<< " - <"<<urlInfo->parameter<<"> = <"<<urlInfo->value<<">\n";

                if (player && urlInfo->command == "player") {
                    playerAccess(urlInfo);
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
            std::cerr << "get: "+path+"\n";
            auto urlInfo = utility::Extractor::getUrlInformation(path);

            if (urlInfo && urlInfo->command == ServerConstant::AccessPoints::playlist) {

               std::string result = playlistAccess(urlInfo);

                return send(generate_result_packet(http::status::ok, result, req, "application/json"));
            }

            if (urlInfo && urlInfo->command == ServerConstant::AccessPoints::database) {

                auto result = database.findInDatabaseToJson(path);

                std::cout << "database find request result: "<<(result?std::get<std::string>(*result):"nothing")<<"\n";

                if (result)
                    return send(generate_result_packet(http::status::ok, std::get<std::string>(*result), req, "application/json"));
                else
                    return send(generate_result_packet(http::status::ok, "[]", req, "application/json"));

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
