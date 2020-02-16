#ifndef SERVER_MIME_TYPE_H
#define SERVER_MIME_TYPE_H

#include <boost/beast/core/string.hpp>

extern boost::beast::string_view
mime_type(boost::beast::string_view path);

#endif //SERVER_MIME_TYPE_H
