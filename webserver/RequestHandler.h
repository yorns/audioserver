#ifndef SERVER_REQUESTHANDLER_H
#define SERVER_REQUESTHANDLER_H

#include <string>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include "nlohmann/json.hpp"
#include "playerinterface/Player.h"
#include "database/SimpleDatabase.h"
#include "common/Constants.h"
#include "common/Extractor.h"
#include "common/mime_type.h"
#include "common/logger.h"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

extern std::unique_ptr<Player> player;
extern SimpleDatabase database;

class RequestHandler {

private:

public:

    RequestHandler(Session& session) : m_lambda(session) {}

    // Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
    std::string
    path_cat(
            boost::beast::string_view base,
            boost::beast::string_view path);


    http::response<http::string_body> generate_result_packet(http::status status,
                                                             boost::beast::string_view why,
                                                             uint32_t version,
                                                             bool keep_alive,
                                                             boost::beast::string_view mime_type = {"text/html"});

    std::string playerAccess(const utility::Extractor::UrlInformation& urlInfo);

    std::string databaseAccess(const utility::Extractor::UrlInformation& urlInfo);

    std::string convertToJson(const std::vector<Id3Info> list);

    std::string playlistAccess(const utility::Extractor::UrlInformation& urlInfo);

    void send(http::response<http::string_body>&& msg)
    {

    }

    void send(http::response<http::empty_body>&& msg)
    {

    }

    void send(http::response<http::file_body>&& msg)
    {

    }

    void handle_file_request(
            http::request_parser<http::string_body>& req);

    nlohmann::json handle_post_request(
            boost::beast::string_view doc_root,
            http::request_parser<http::string_body>& req );

    bool is_unknown_http_method(http::request_parser<http::string_body>& req);

    bool is_illegal_request_target(http::request_parser<http::string_body>& req);

    bool is_request(boost::beast::string_view doc_root,
                    boost::beast::string_view command,
                    http::request_parser<http::string_body>& req);

    void handle_request (
            http::request_parser<http::string_body>& req);


    void handle_put_result(){}

};


#endif //SERVER_REQUESTHANDLER_H
