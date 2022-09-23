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
    static constexpr auto mp4Extension{sv(".m4a")};
    static constexpr auto pngExtension{sv(".png")};
    static constexpr auto jpegExtension{sv(".jpeg")};
    static constexpr auto imagePath{sv("img")};
    static constexpr auto audioPath{sv("audio")};
    static constexpr auto playlistPath{sv("pl")};
    static constexpr auto fileprefix{sv("file://")};
    static constexpr auto httpprefix{sv("http://")};
    static constexpr auto httpsprefix{sv("https://")};
    static constexpr auto indexfile{sv("index.html")};

    namespace id3 {
        static const std::string covr {"covr"};
    }

    namespace SNC {
        static const std::string songBroadcastMessage{"SongBroadcastMessage"};
        static const std::string commandMessage{"CmdMsg"};
        static const std::string ssidMessage{"SsidMessage"};
        static const std::string commandRequest{"Request"};
        static const std::string CommandReply{"Reply"};
        static const std::string commandStart{"Start"};
        static const std::string commandStop{"Stop"};
        static const std::string dataMessage{"Data"};
        static const std::string position{"Position"};
        static const std::string album{"Album"};
        static const std::string title{"Title"};
        static const std::string albumId{"AlbumID"};
        static const std::string titleId{"TitleID"};
    }

    namespace JsonField {
        static const std::string title{"Title"};
        static const std::string album{"Album"};
        static const std::string performer{"Performer"};
        static const std::string trackNo{"TrackNo"};
        static const std::string allTrackNo{"AllTrackNo"};
        static const std::string url{"Url"};
        static const std::string extension{"Extension"};
        static const std::string albumCreation{"AlbumCreation"};
        static const std::string image{"Image"};
        static const std::string imageUrl{"ImageUrl"};
        static const std::string audioUrl{"AudioUrl"};
        static const std::string uid{"Uid"};
        static const std::string cover{"Cover"};
        static const std::string infoSrc{"InfoSrc"};
        static const std::string disk{"Disk"};
        static const std::string coverUrl{"CoverUrl"};
        static const std::string name{"Name"};
        static const std::string items{"Items"};
        static const std::string tag{"Tag"};
        static const std::string hash{"Hash"};
        static const std::string hasCover{"HasCover"};
        static const std::string uidList{"UidList"};
        static const std::string playlist {"Playlist"};
        static const std::string playlists {"Playlists"};
        static const std::string currentPlaylist {"CurrentPlaylist"};
        static const std::string current {"Current"};
        namespace Config {
            static const std::string ipAddress {"IpAddress"};
            static const std::string port {"Port"};
            static const std::string basePath {"BasePath"};
            static const std::string enableCache {"EnableCache"};
            static const std::string logLevel {"LogLevel"};
            static const std::string amplify {"Amplify"};
            static const std::string audioInterface {"AudioInterface"};
            namespace PlayerType {
              static const std::string gstreamer {"gst"};
              static const std::string mpl {"mpl"};
            }
            namespace LogLevel {
                static const std::string info {"info"};
                static const std::string  warning {"warning"};
                static const std::string error {"error"};
            }
        }
        namespace Websocket {
            static const std::string songId {"SongID"};
            static const std::string playlistId {"PlaylistID"};
            static const std::string curPlaylistId {"CurPlaylistID"};
            //static const std::string song {"Song"};
            static const std::string playlist {"Playlist"};
            static const std::string position {"Position"};
            static const std::string loop {"Loop"};
            static const std::string shuffle {"Shuffel"};
            static const std::string playing {"Playing"};
            static const std::string paused {"Paused"};
            static const std::string volume {"Volume"};
            static const std::string single {"Single"};
            static const std::string title {"Title"};
            static const std::string album {"Album"};
            static const std::string performer {"Performer"};
            static const std::string cover {"Cover"};
        }
        namespace wifi {
        static const std::string ssid {"Ssid"};
        static const std::string psk {"Psk"};
        static const std::string networks {"Networks"};
        }
    }

    static constexpr int seekForwardSeconds {20};
    static constexpr int seekBackwardSeconds {-20};

    static constexpr auto unknownCoverFile {sv("unknown")};
    static constexpr auto unknownCoverFileUid {sv("b7ac2645-1667-46be-0000-000000000000")};
    static constexpr auto unknownCoverExtension {sv(".png")};
    static constexpr auto unknownCoverUrl {"img/unknown.png"};

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
            static constexpr auto url{sv("url")};
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
