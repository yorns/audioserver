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

using boost::asio::local::stream_protocol;

class MpvPlayer : public BasePlayer
{

    stream_protocol::socket m_socket;

    std::string m_stringBuffer;

    static constexpr std::string_view m_accessPoint {"/tmp/mpvsocket"};
    bool m_isPlaying {false};
    bool m_pause {false};
    double actTime { 0.0 };

    std::string actFile;
    std::string actHumanReadable;

    std::deque<std::string> m_expectedAnswer;

    std::unordered_map<std::string, std::function<void(const nlohmann::json& data)>> propertiesHandler;

    void read_handler(const boost::system::error_code& error, std::size_t bytes_transferred);
    bool sendData(std::string&& cmd);
    bool sendCommand(std::vector<std::variant<const char*, std::string, bool, int>>&& argumentList);
    void init_communication();
    void init_MpvCommandHandling();

public:
    explicit MpvPlayer(boost::asio::io_context& context)
        : m_socket(context)
    {
        init_communication();
        init_MpvCommandHandling();
    }

    MpvPlayer() = delete;

    MpvPlayer(const MpvPlayer& ) = delete;
    MpvPlayer(MpvPlayer&& ) = default;

    MpvPlayer& operator=(const MpvPlayer& ) = delete;
    MpvPlayer& operator=(MpvPlayer&& ) = default;

    virtual ~MpvPlayer() = default;

    bool startPlay(const std::string &url) final
    {
        if (m_isPlaying && !stop())
            return false;

        // unpause (just to be sure)
        return sendCommand({"set_property", "pause", m_pause = false}) &&
               sendCommand({"loadlist", url});

    }

    bool stop() final
    {
        logger(LoggerFramework::Level::info)<<"stopping player\n";
        m_isPlaying = false;
        return sendData({"stop"});
    }

    bool seek_forward() final
    {
        return sendCommand({"seek",ServerConstant::seekForwardSeconds});
    }

    bool seek_backward() final
    {
        return sendCommand({"seek",ServerConstant::seekBackwardSeconds});
    }

    bool next_file() final
    {
        return sendCommand({"playlist-next"});
    }

    bool prev_file() final
    {
        return sendCommand({"playlist-prev"});
    }

    bool pause_toggle() final
    {
        m_pause = !m_pause;
        return sendCommand({"set_property", "pause", m_pause});
    }

    bool isPlaying() final
    {
        return m_isPlaying;
    }

    bool stopPlayerConnection() final
    {
        m_isPlaying = false;
        if (m_socket.is_open()) {
            boost::system::error_code ec;
            m_socket.close(ec);
            if (ec) {
                logger(LoggerFramework::Level::warning) << "not able to close socket\n";
                return false;
            }
        }
        else {
            return false;
        }
        logger(LoggerFramework::Level::debug) << "player stopped\n";
        return true;
    }

    std::string getSong() final { return actHumanReadable; }
    std::string getSongID() final { return actFile; }
    uint32_t getSongPercentage() final { if (actTime > 100.0) actTime = 100.0; return static_cast<uint32_t>(actTime*100); }

    void selectPlaylistsEntry(uint32_t id) final
    {
    boost::ignore_unused(id);
    }

};

#endif // MPVPLAYER_H
