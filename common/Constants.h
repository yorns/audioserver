#ifndef SERVER_CONSTANTS_H
#define SERVER_CONSTANTS_H

#include <boost/beast.hpp>

namespace ServerConstant {
    using sv = std::string_view;
    extern sv base_path;

    static constexpr auto audioPath {sv("mp3")};
    static constexpr auto streamInfoPath {sv("stream")};
    static constexpr auto playlistPath {sv("playlist")};
    static constexpr auto htmlPath {sv("html")};
    static constexpr auto coverPath {sv("html/img")};
    static constexpr auto coverPathWeb {sv("img")};
    static constexpr auto coverPathWebAlien {sv("img_alien")};
    static constexpr auto tmpPath {sv("tmp")};
    static constexpr auto playerLogPath {sv("player_log")};

    static constexpr int seekForwardSeconds {20};
    static constexpr int seekBackwardSeconds {-20};

    static constexpr auto unknownCoverFile {sv("unknown")};
    static constexpr auto unknownCoverExtension {sv(".png")};

    namespace AccessPoints {
        static constexpr auto playlist {sv("/playlist")};
        static constexpr auto database {sv("/database")};
        static constexpr auto player {sv("/player")};
        static constexpr auto upload {sv("/upload")};
        static constexpr auto websocket {sv("/dynamic")};
    }

    namespace Command {
        static constexpr auto play {sv("play")};
        static constexpr auto find {sv("find")};
        static constexpr auto create {sv("create")};
        static constexpr auto createAlbumList {sv("createAlbumList")};
        static constexpr auto add {sv("add")};
        static constexpr auto change {sv("change")};
        static constexpr auto show {sv("show")};
        static constexpr auto showLists {sv("showLists")};
        static constexpr auto getAlbumList {sv("albumList")};
    }

    namespace Parameter {
        namespace Database {
            static constexpr auto titel {sv("titel")};
            static constexpr auto album {sv("album")};
            static constexpr auto interpret {sv("performer")};
            static constexpr auto trackNo {sv("trackNo")};
            static constexpr auto uid {sv("uid")};
            static constexpr auto overall {sv("overall")};
            static constexpr auto playlist {sv("playlist")};
            static constexpr auto imageFile {sv("cover")};
        }
        namespace Player {
            static constexpr auto next {sv("next")};
            static constexpr auto previous {sv("prev")};
            static constexpr auto seek_forward {sv("seek_forward")};
            static constexpr auto seek_backward {sv("seek_backward")};
            static constexpr auto play {sv("play")};
            static constexpr auto stop {sv("stop")};
            static constexpr auto pause {sv("pause")};
            static constexpr auto fastForward {sv("fastForward")};
            static constexpr auto fastBackward {sv("fastBackward")};
            static constexpr auto toggleShuffle {sv("toggleShuffle")};
            static constexpr auto toggleLoop {sv("toggleLoop")};
            static constexpr auto select {sv("select")};
            static constexpr auto toPosition {sv("toPosition")};
        }
    }

    namespace Value {
        static constexpr auto _true {sv("true")};
        static constexpr auto _false {sv("false")};
    }


}

#endif //SERVER_CONSTANTS_H
