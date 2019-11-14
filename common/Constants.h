#ifndef SERVER_CONSTANTS_H
#define SERVER_CONSTANTS_H

#include <boost/beast.hpp>

namespace ServerConstant {
    static boost::beast::string_view base_path{"/usr/share/audioserver"};

    static boost::beast::string_view fileRootPath{"mp3"};
    static boost::beast::string_view playlistRootPath{"playlist"};
    static boost::beast::string_view coverRootPath{"html/img"};
    static boost::beast::string_view albumPlaylistDirectory{"tmp"};

    static boost::beast::string_view unknownCoverFile{"unknown"};
    static boost::beast::string_view unknownCoverExtension{".png"};

    namespace AccessPoints {
        static boost::beast::string_view playlist{"playlist"};
        static boost::beast::string_view database{"database"};
        static boost::beast::string_view play{"play"};

    }

    namespace Command {
        static boost::beast::string_view play{"play"};
        static boost::beast::string_view find{"find"};
        static boost::beast::string_view create{"create"};
        static boost::beast::string_view createAlbumList{"createAlbumList"};
        static boost::beast::string_view add{"add"};
        static boost::beast::string_view change{"change"};
        static boost::beast::string_view show{"show"};
        static boost::beast::string_view showLists{"showLists"};
        static boost::beast::string_view getAlbumList{"albumList"};
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
            static boost::beast::string_view imageFile{"cover"};
        }
        namespace Play {
            static boost::beast::string_view next{"next"};
            static boost::beast::string_view previous{"prev"};
            static boost::beast::string_view play{"play"};
            static boost::beast::string_view stop{"stop"};
            static boost::beast::string_view pause{"pause"};
        }
    }

    namespace Value {
        static boost::beast::string_view _true{"true"};
        static boost::beast::string_view _false{"false"};
    }


}

#endif //SERVER_CONSTANTS_H
