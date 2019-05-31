#ifndef SERVER_CONSTANTS_H
#define SERVER_CONSTANTS_H

#include <boost/beast.hpp>

namespace ServerConstant {
    static boost::beast::string_view fileRootPath{"mp3"};
    static boost::beast::string_view playlistRootPath {"playlist"};
}

#endif //SERVER_CONSTANTS_H
