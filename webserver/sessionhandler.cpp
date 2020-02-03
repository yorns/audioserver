#include "sessionhandler.h"
#include "common/logger.h"

bool SessionHandler::addUrlHandler(const boost::beast::string_view &path, http::verb method, PathCompare pathCompare, SessionHandler::RequestHandler &&handler)
{
    auto handlerIt = find(pathToStringHandler, path, method);

    if (handlerIt != pathToStringHandler.end() && pathCompare == std::get<PathCompare>(*handlerIt)) {
        logger(Level::warning) << "path is occupied and will be replaces (" << std::get<boost::beast::string_view>(*handlerIt).to_string() << ")\n";
    }

    auto indexTuple = std::make_tuple(path, method, pathCompare, std::move(handler));

    pathToStringHandler.emplace_back(indexTuple);
    return true;
}

bool SessionHandler::addUploadHandler(const boost::beast::string_view &path, SessionHandler::NameGeneratorFunction &&handler, SessionHandler::UploadFinishedHandler &&finishHandler)
{

    http::verb method { http::verb::post };

    auto handlerIt = find(pathToFileHandler, path, method);
    auto newTuple = std::make_tuple(path, method, PathCompare::exact, std::move(handler), std::move(finishHandler));

    if (handlerIt != pathToFileHandler.end()) {
        logger(Level::warning) << "path is occupied and will be replaces (" << std::get<boost::beast::string_view>(*handlerIt).to_string() << ")\n";
        // TODO: replace needs to be implemented .. easy first
        pathToFileHandler.erase(handlerIt);
    }

    pathToFileHandler.emplace_back(newTuple);
    return true;
}

std::string SessionHandler::callHandler(http::request_parser<http::string_body> &requestHeader) const {

    const boost::beast::string_view& path = requestHeader.get().target();
    const http::verb& method = requestHeader.get().method();

    auto handlerIt = find(pathToStringHandler, path, method);

    if (handlerIt != pathToStringHandler.end()) {
        auto& handler = std::get<RequestHandler>(*handlerIt);
        return handler(requestHeader);
    }

    return "";
}

bool SessionHandler::isUploadFile(const http::request_parser<http::empty_body> &requestHeader) const {
    const boost::beast::string_view& path = requestHeader.get().target();
    const boost::beast::http::verb& method = requestHeader.get().method();
    return find(pathToFileHandler, path, method) != pathToFileHandler.end();
}

bool SessionHandler::isRestAccesspoint(const http::request_parser<http::empty_body> &requestHeader) const {
    const boost::beast::string_view& path = requestHeader.get().target();
    const boost::beast::http::verb& method = requestHeader.get().method();
    return find(pathToStringHandler, path, method) != pathToStringHandler.end();
}

bool SessionHandler::callFileUploadHandler(http::request_parser<http::file_body> &request, const NameGenerator::GenerationName &name) const {

    const boost::beast::string_view& path = request.get().target();
    const http::verb& method = request.get().method();

    auto handlerIt = find(pathToFileHandler, path, method);


    if (handlerIt != pathToFileHandler.end()) {
        auto handler = std::get<UploadFinishedHandler>(*handlerIt);
        return handler(name);
    }

    return false;

}

NameGenerator::GenerationName SessionHandler::getName(http::request_parser<http::empty_body> &requestHeader) const {

    const boost::beast::string_view& path = requestHeader.get().target();
    const boost::beast::http::verb& method = requestHeader.get().method();

    auto fileIt = find(pathToFileHandler, path, method);
    if (fileIt != pathToFileHandler.end()) {
        return std::get<NameGeneratorFunction>(*fileIt)();
    }

    return NameGenerator::GenerationName();
}
