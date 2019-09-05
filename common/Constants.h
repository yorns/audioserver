#ifndef SERVER_CONSTANTS_H
#define SERVER_CONSTANTS_H

#include <boost/beast.hpp>

namespace ServerConstant {
    static boost::beast::string_view fileRootPath{"mp3"};
    static boost::beast::string_view playlistRootPath{"playlist"};

    namespace AccessPoints {
        static boost::beast::string_view playlist{"playlist"};
        static boost::beast::string_view database{"database"};
        static boost::beast::string_view play{"play"};

    }

    namespace Command {
        static boost::beast::string_view play{"play"};
        static boost::beast::string_view find{"find"};
        static boost::beast::string_view create{"create"};
        static boost::beast::string_view add{"add"};
        static boost::beast::string_view change{"change"};
        static boost::beast::string_view show{"show"};
        static boost::beast::string_view showLists{"showLists"};
    }

    namespace Parameter {
        namespace Database {
            static boost::beast::string_view titel{"titel"};
            static boost::beast::string_view album{"album"};
            static boost::beast::string_view interpret{"performer"};
            static boost::beast::string_view trackNo{"trackNo"};
            static boost::beast::string_view uid{"uid"};
            static boost::beast::string_view overall{"overall"};
            static boost::beast::string_view playlist{"playlist"};
        }
        namespace Play {
            static boost::beast::string_view next{"next"};
            static boost::beast::string_view previous{"prev"};
            static boost::beast::string_view play{"play"};
            static boost::beast::string_view stop{"stop"};
        }
    }

    namespace Value {
        static boost::beast::string_view _true{"true"};
        static boost::beast::string_view _false{"false"};
    }


}

#endif //SERVER_CONSTANTS_H
