#include "RequestHandler.h"

std::string RequestHandler::path_cat(boost::beast::string_view base, boost::beast::string_view path)
{
    char constexpr path_separator = '/';

    if ( path.starts_with(path_separator) )
        path = path.substr(1);

    if(base.empty())
        return std::string(1, path_separator) + path.to_string();

    std::string result = base.to_string();
    if(result.back() == path_separator)
        result.resize(result.size() - 1);

    result.append(1, path_separator);
    result.append(path.data(), path.size());

    return result;
}

//http::response<http::string_body> RequestHandler::generate_result_packet(http::status status, boost::beast::string_view why, http::request_parser<http::string_body> &req, boost::beast::string_view mime_type)
http::response<http::string_body> RequestHandler::generate_result_packet(http::status status, boost::beast::string_view why, uint32_t version, bool keep_alive, boost::beast::string_view mime_type)
{
    http::response<http::string_body> res{status, version}; //req.get().version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type.to_string());
    res.keep_alive(keep_alive); //req.get().keep_alive());
    res.body() = why.to_string();
    res.prepare_payload();
    return res;
}

std::string RequestHandler::playerAccess(const utility::Extractor::UrlInformation &urlInfo) {
    if ( urlInfo->parameter == ServerConstant::Command::play &&
         urlInfo->value == ServerConstant::Value::_true) {
        logger(Level::info) << "Play request\n";
        std::stringstream albumPlaylist;
        std::stringstream genericPlaylist;
        albumPlaylist << ServerConstant::base_path << "/" << ServerConstant::albumPlaylistDirectory << "/" << currentPlaylist << ".m3u";
        genericPlaylist << ServerConstant::base_path << "/" << ServerConstant::playlistPath << "/" << currentPlaylist << ".m3u";
        player->startPlay(albumPlaylist?albumPlaylist.str():genericPlaylist.str(), "");
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

    if (urlInfo->parameter == ServerConstant::Parameter::Play::pause &&
            urlInfo->value == ServerConstant::Value::_true) {
        player->pause();
        return "ok";
    }

    return "player command unknown";
}

std::string RequestHandler::databaseAccess(const utility::Extractor::UrlInformation &urlInfo) {

    logger(Level::debug) << "database access - parameter:"<<urlInfo->parameter<<" value:"<<urlInfo->value<<"\n";
    if (urlInfo->parameter == ServerConstant::Command::getAlbumList) {
        auto infoList = database.findAlbum(urlInfo->value);
        return convertToJson(infoList);
    }

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::overall )
        return convertToJson(database.findInDatabase(urlInfo->value, SimpleDatabase::DatabaseSearchType::overall));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::interpret )
        return convertToJson(database.findInDatabase(urlInfo->value, SimpleDatabase::DatabaseSearchType::interpret));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::titel )
        return convertToJson(database.findInDatabase(urlInfo->value, SimpleDatabase::DatabaseSearchType::titel));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::album )
        return convertToJson(database.findInDatabase(urlInfo->value, SimpleDatabase::DatabaseSearchType::album));

    return "[]";
}

std::string RequestHandler::convertToJson(const std::vector<Id3Info> list) {
    nlohmann::json json;
    for(auto item : list) {
        nlohmann::json jentry;
        jentry[ServerConstant::Parameter::Database::uid.to_string()] = item.uid;
        jentry[ServerConstant::Parameter::Database::interpret.to_string()] = item.performer_name;
        jentry[ServerConstant::Parameter::Database::album.to_string()] = item.album_name;
        jentry[ServerConstant::Parameter::Database::titel.to_string()] = item.titel_name;
        jentry[ServerConstant::Parameter::Database::imageFile.to_string()] = item.imageFile;
        jentry[ServerConstant::Parameter::Database::trackNo.to_string()] = item.track_no;
        json.push_back(jentry);
    }
    return json.dump(2);
}

std::string RequestHandler::playlistAccess(const utility::Extractor::UrlInformation &urlInfo) {

    logger(Level::debug) << "player request: <"<<urlInfo->parameter<<"> with value <"<<urlInfo->value<<">\n";

    if (urlInfo->parameter == ServerConstant::Command::create) {
        std::string ID = database.createPlaylist(urlInfo->value);
        albumPlaylist = false;
        logger(Level::info) << "create playlist with name <" << urlInfo->value << "> "<<"-> "<<ID<<" \n";
        if (!ID.empty()) {
            currentPlaylist = ID;
            albumPlaylist = false;
            database.writeChangedPlaylists();
            return R"({"result": "ok"})";
        } else {
            return R"({"result": "playlist not new"})";
        }
    }

    if (urlInfo->parameter == ServerConstant::Command::createAlbumList) {
        std::string ID = database.createAlbumPlaylistTmp(urlInfo->value);
        logger(Level::info) << "create album playlist with name <" << urlInfo->value << "> "<<"-> "<<ID<<" \n";
        if (!ID.empty()) {
            currentPlaylist = ID;
            albumPlaylist = true;
            database.writeChangedPlaylists();
            return R"({"result": "ok"})";
        } else {
            return R"({"result": "playlist not new"})";
        }
    }


    if (urlInfo->parameter == ServerConstant::Command::change) {

        if (database.isPlaylist(urlInfo->value)) {
            logger(Level::info) << "changed to playlist with name <" << urlInfo->value << ">\n";
            currentPlaylist = database.getNameFromHumanReadable(urlInfo->value);
            albumPlaylist = false;
            logger(Level::debug) << "current playlist is <" << currentPlaylist << ">\n";
            return R"({"result": "ok"})";
        } else {
            logger(Level::warning) << "playlist with name <" << urlInfo->value << "> not found\n";
            return R"({"result": "playlist not found"})";
        }
    }

    if (urlInfo->parameter == ServerConstant::Command::add) {
        if (!currentPlaylist.empty() && database.isPlaylistID(currentPlaylist)) {
            logger(Level::debug) << "add title with name <" << urlInfo->value << "> to playlist <"<<currentPlaylist<<">\n";
            database.addToPlaylistID(currentPlaylist, urlInfo->value);
            database.writeChangedPlaylists();
            return R"({"result": "ok"})";
        } else {
            return R"({"result": "cannot add <)" + urlInfo->value + "> to playlist <" + currentPlaylist + ">}";
        }
    }

    if (urlInfo->parameter == ServerConstant::Command::show) {
        try {
            if ((urlInfo->value.empty() && !currentPlaylist.empty()) || database.isPlaylist(urlInfo->value)) {
                std::string playlistName(currentPlaylist);
                if (!urlInfo->value.empty())
                    playlistName = database.getNameFromHumanReadable(urlInfo->value);
                logger(Level::debug) << "show playlist <" << playlistName << ">\n";
                auto list = database.showPlaylist(playlistName);
                std::vector<Id3Info> infoList;
                for (auto item : list) {
                    if (item.empty()) continue;
                    infoList.push_back(
                                database.findInDatabase(item, SimpleDatabase::DatabaseSearchType::uid).front());
                }
                return convertToJson(infoList);
            }
        } catch (...) {}
        return "[]";
    }

    if (urlInfo->parameter == ServerConstant::Command::showLists) {
        logger(Level::info) << "show all playlists\n";
        std::vector<std::pair<std::string, std::string>> lists = database.showAllPlaylists();
        if (!lists.empty()) {
            logger(Level::debug) << "playlists are \n";
            nlohmann::json json;
            nlohmann::json json1;
            try {
                for (auto item : lists) {
                    logger(Level::debug) << item.first << " - " << item.second << "\n";
                    nlohmann::json jentry;
                    jentry[ServerConstant::Parameter::Database::uid.to_string()] = item.first;
                    jentry[ServerConstant::Parameter::Database::playlist.to_string()] = item.second;
                    json1.push_back(jentry);
                }
                json["playlists"] = json1;
                json["actualPlaylist"] = database.getHumanReadableName(currentPlaylist);
            } catch (std::exception& e) {
                logger(Level::warning) << "error: " << e.what() << "\n";
            }
            return json.dump(2);
        } else {
            return "[]";
        }
    }

    return R"({"result": "cannot find parameter <)" + urlInfo->parameter + "> in playlist\"}";
}

template<class Session>
void RequestHandler::handle_file_request(http::request_parser<http::string_body> &req)
{
    // Build the path to the requested file
    std::string file_root (path_cat(ServerConstant::base_path, ServerConstant::htmlPath));

    std::string path = path_cat(file_root, req.get().target());
    if(req.get().target().back() == '/')
        path.append("index.html");

    logger(Level::debug) << "file request on: <"<< path << ">\n";

    // Attempt to open the file
    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if(ec == boost::system::errc::no_such_file_or_directory) {
        return send(generate_result_packet(http::status::not_found, req.get().target(), req.get().version(), req.get().keep_alive()));
    }
    // Handle an unknown error
    if(ec) {
        return send(generate_result_packet(http::status::internal_server_error, ec.message(), req.get().version(), req.get().keep_alive()));
    }

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

template<class Session>
nlohmann::json RequestHandler::handle_post_request(boost::beast::string_view doc_root, http::request_parser<http::string_body> &req) {

    std::string path = path_cat(doc_root, req.get().target());
    std::cerr << "posted: "+path+"\n";
    auto urlInfo = utility::Extractor::getUrlInformation(path);

    std::string result {"done"};
    if (urlInfo) {

        if (player && urlInfo->command == "player") {
            playerAccess(urlInfo);
        }

    }
    else {
        result = "request failed";
    }

    nlohmann::json json;
    json["result"] = result;

    return json;
}

template<class Session>
bool RequestHandler::is_unknown_http_method(http::request_parser<http::string_body> &req) {
    return req.get().method() != http::verb::get &&
            req.get().method() != http::verb::head &&
            req.get().method() != http::verb::post;
}

template<class Session>
bool RequestHandler::is_illegal_request_target(http::request_parser<http::string_body> &req) {
    return req.get().target().empty() ||
            req.get().target()[0] != '/' ||
            req.get().target().find("..") != boost::beast::string_view::npos;
}

template<class Session>
bool RequestHandler::is_request(boost::beast::string_view doc_root, boost::beast::string_view command, http::request_parser<http::string_body> &req) {
    auto urlInfo = utility::Extractor::getUrlInformation(path_cat(doc_root, req.get().target()));
    return req.get().method() == http::verb::get && urlInfo && urlInfo->command == command;
}

template<class Session>
void RequestHandler::handle_request(http::request_parser<http::string_body> &req) {

    boost::beast::string_view doc_root { ServerConstant::base_path };

    logger(Level::debug) << "main request: <"<< req.get().target() << ">\n";

    // Make sure we can handle the method
    if (is_unknown_http_method(req)) {
        return send(generate_result_packet(http::status::bad_request, "Unknown HTTP-method", req.get().version(), req.get().keep_alive()));
    }

    // Request path must be absolute and not contain "..".
    if (is_illegal_request_target(req)) {
        return send(generate_result_packet(http::status::bad_request, "Illegal request-target", req.get().version(), req.get().keep_alive()));
    }
    // has this been a post message
    if (req.get().method() == http::verb::post) {
        auto json = handle_post_request(doc_root, req);
        return send(generate_result_packet(http::status::ok, json.dump(2), req.get().version(), req.get().keep_alive(), "application/json"));
    }

    if (is_request(doc_root, ServerConstant::AccessPoints::playlist, req)) {

        logger(Level::info) << "playlist request: <"<< req.get().target() << ">\n";

        std::string result = playlistAccess(
                    utility::Extractor::getUrlInformation(path_cat(doc_root, req.get().target())));

        logger(Level::debug) << result << "\n";

        return send(generate_result_packet(http::status::ok, result, req.get().version(), req.get().keep_alive(), "application/json"));
    }

    if (is_request(doc_root, ServerConstant::AccessPoints::database, req)) {

        logger(Level::info) << "database request: <"<< req.get().target() << ">\n";

        std::string result = databaseAccess(
                    utility::Extractor::getUrlInformation(path_cat(doc_root, req.get().target())));

        return send(generate_result_packet(http::status::ok, result, req.get().version(), req.get().keep_alive(), "application/json"));

    }

    return handle_file_request(req);

}