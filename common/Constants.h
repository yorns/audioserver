#ifndef SERVER_CONSTANTS_H
#define SERVER_CONSTANTS_H

#include <string_view>
#include <boost/beast.hpp>

namespace ServerConstant {
    using sv = std::string_view;
    extern sv base_path;

    static constexpr auto tagFile {sv("tag.data")};
    static constexpr auto credentialFile{sv("credential.data")};

    static constexpr auto audioPathMp3 {sv("audioMp3")};
    static constexpr auto audioPathJson {sv("audioJson")};
    static constexpr auto playlistPathJson {sv("playlistJson")};
    static constexpr auto playlistPathM3u {sv("playlistM3u")};
    static constexpr auto htmlPath {sv("html")};
    static constexpr auto coverPath {sv("html/img")};
    static constexpr auto coverPathWeb {sv("img")};
    static constexpr auto coverPathWebAlien {sv("img_alien")};
    static constexpr auto cachePath {sv("cache")};
    static constexpr auto tagPath {sv("tag")};
    static constexpr auto credentialPath {sv("credential")};
    static constexpr auto tmpPath {sv("tmp")};
    static constexpr auto playerLogPath {sv("player_log")};
    static constexpr auto mp3Extension{sv(".mp3")};


    static constexpr int seekForwardSeconds {20};
    static constexpr int seekBackwardSeconds {-20};

    static constexpr auto unknownCoverFile {sv("unknown")};
    static constexpr auto unknownCoverFileUid {sv("b7ac2645-1667-46be-0000-000000000000")};
    static constexpr auto unknownCoverExtension {sv(".png")};

    namespace AccessPoints {
        static constexpr auto playlist {sv("/playlist")};
        static constexpr auto database {sv("/database")};
        static constexpr auto player {sv("/player")};
        static constexpr auto wifi {sv("/wifi")};
        static constexpr auto upload {sv("/upload")};
        static constexpr auto websocket {sv("/dynamic")};
        namespace Virtual {
            static constexpr auto image {sv("/img")};
            static constexpr auto audio {sv("/audio")};
        }
    }

    namespace Command {
        static constexpr auto play {sv("play")};
        static constexpr auto readPlaylist {sv("readPlaylist")};
        static constexpr auto find {sv("find")};
        static constexpr auto create {sv("create")};
        static constexpr auto createAlbumList {sv("createAlbumList")};
        static constexpr auto add {sv("add")};
        static constexpr auto change {sv("change")};
        static constexpr auto show {sv("show")};
        static constexpr auto showLists {sv("showLists")};
        static constexpr auto currentPlaylistUID {sv("currentPlaylistUID")};
        static constexpr auto getAlbumList {sv("albumList")};
        static constexpr auto getAlbumUid {sv("albumUid")};
    }

    namespace Parameter {
        namespace Database {
            static constexpr auto title {sv("title")};
            static constexpr auto album {sv("album")};
            static constexpr auto performer {sv("performer")};
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
            static constexpr auto reset {sv("reset")};
            static constexpr auto pause {sv("pause")};
            static constexpr auto fastForward {sv("fastForward")};
            static constexpr auto fastBackward {sv("fastBackward")};
            static constexpr auto toggleShuffle {sv("toggleShuffle")};
            static constexpr auto toggleLoop {sv("toggleLoop")};
            static constexpr auto toggleSingle {sv("toggleSingle")};
            static constexpr auto select {sv("select")};
            static constexpr auto toPosition {sv("toPosition")};
            static constexpr auto volume {sv("volume")};
        }
        namespace Wifi {
        static constexpr auto ssid { sv("ssid") };
        static constexpr auto psk { sv("psk") };

        }
    }

    namespace Value {
        static constexpr auto _true {sv("true")};
        static constexpr auto _false {sv("false")};
    }


}

#endif //SERVER_CONSTANTS_H
