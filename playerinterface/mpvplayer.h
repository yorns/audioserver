#ifndef MPVPLAYER_H
#define MPVPLAYER_H

#include "Player.h"
#include <variant>
#include <string>
#include <deque>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/core/ignore_unused.hpp>
#include "common/logger.h"
#include "common/Constants.h"
#include "common/filesystemadditions.h"

using boost::asio::local::stream_protocol;

class MpvPlayer : public BasePlayer
{
public:
    enum class NextPlaylistDirection {
        next,
        previous,
        current,
        stop
    };

private:
    stream_protocol::socket m_socket;

    std::string m_stringBuffer;

    static constexpr std::string_view m_accessPoint {"/tmp/mpvsocket"};
    NextPlaylistDirection m_nextPlaylistDirection { NextPlaylistDirection::next };
    float m_actTime { 0.0 };

    std::string m_AudioFileName;

    std::deque<std::string> m_expectedAnswer;
    std::unordered_map<std::string, std::function<void(const nlohmann::json& data)>> m_propertiesHandler;

    void read_handler(const boost::system::error_code& error, std::size_t bytes_transferred);
    bool sendData(std::string&& cmd);
    bool sendCommand(std::vector<std::variant<const char*, std::string, bool, int>>&& argumentList);
    void init_communication();
    void init_MpvCommandHandling();

    bool doPlayFile(const Common::PlaylistItem& playlistItem);

    void stopAndRestart() final {
        if (needsStop()) {
            m_nextPlaylistDirection = MpvPlayer::NextPlaylistDirection::current;
            sendCommand({"stop"});
        }
        m_currentItemIterator = m_playlist.begin();
    }

public:
    MpvPlayer(boost::asio::io_context& context);

    MpvPlayer() = delete;

    MpvPlayer(const MpvPlayer& ) = delete;
    MpvPlayer(MpvPlayer&& ) = default;

    MpvPlayer& operator=(const MpvPlayer& ) = delete;
    MpvPlayer& operator=(MpvPlayer&& ) = default;

    virtual ~MpvPlayer() = default;

    bool startPlay(const Common::AlbumPlaylistAndNames& albumPlaylistAndNames, const boost::uuids::uuid& songUID, uint32_t position) final;
    bool stop() final;
    bool stopPlayerConnection() final;

    bool setVolume(uint32_t volume) final { m_volume = volume;
                                            return sendCommand({"volume",ServerConstant::seekForwardSeconds});
                                          }
    bool seek_forward() final;
    bool seek_backward() final;
    bool next_file() final;
    bool prev_file() final;
    bool pause_toggle() final;

    bool jump_to_position(int percent) final;
    bool jump_to_fileUID(const boost::uuids::uuid& filename) final;

    const std::string getSongName() const final;
    boost::uuids::uuid getSongID() const final;
    int getSongPercentage() const final;

    const std::string getTitle() const final { return ""; }
    const std::string getAlbum() const final { return ""; }
    const std::string getPerformer() const final { return ""; }

};

#endif // MPVPLAYER_H
