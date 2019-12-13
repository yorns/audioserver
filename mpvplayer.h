#ifndef MPVPLAYER_H
#define MPVPLAYER_H

#include "Player.h"
#include <boost/asio.hpp>
#include <string>
#include <nlohmann/json.hpp>
#include <boost/core/ignore_unused.hpp>

using boost::asio::local::stream_protocol;

class MpvPlayer : public Player
{
    boost::asio::io_context& m_context;

    std::string m_accessPoint {"/tmp/mpvsocket"};
    std::vector<char> m_readBuffer;

    stream_protocol::socket m_socket;

    bool m_pause {false};

    void write_handler( const boost::system::error_code& , std::size_t ) {

    }

    void read_handler(const boost::system::error_code& error, std::size_t bytes_transferred) {

        if (!error) {
            std::cout << "> " << std::string(m_readBuffer.data(),bytes_transferred) << "\n";
            m_socket.async_receive(boost::asio::buffer(m_readBuffer), 0,
                                   [this](const boost::system::error_code& error, std::size_t bytes_transferred)
            { read_handler(error, bytes_transferred); }
            );
        }
    }

    void set_command(std::string&& cmd) {

        cmd += "\n";
        std::cout << "player interface sends "<<cmd<<"\n";

        m_socket.async_send(boost::asio::buffer(cmd),
                            [this](const boost::system::error_code& error, std::size_t bytes_transferred)
        { write_handler(error, bytes_transferred); });

    }

public:
    explicit MpvPlayer(boost::asio::io_context& context, const std::string& configDB, const std::string &logFilePath)
        : Player(configDB, logFilePath), m_context(context), m_readBuffer(255), m_socket(m_context)
    {
        std::cerr << __FILE__ << ":" << __LINE__ << "> connecting mpv player at <"<<m_accessPoint << ">\n";
        m_socket.connect(m_accessPoint);

        std::cerr << __FILE__ << ":" << __LINE__ << "> establish mpv receiver\n";
        m_socket.async_receive(boost::asio::buffer(m_readBuffer), 0,
                               [this](const boost::system::error_code& error, std::size_t bytes_transferred)
        { read_handler(error, bytes_transferred); });
        std::cerr << __FILE__ << ":" << __LINE__ << "> MPV player Interface established\n";
    }

    MpvPlayer() = delete;

    virtual bool startPlay(const std::string &url, const std::string& playlist, bool fromLastStop = false)
    {
        boost::ignore_unused(fromLastStop);
        boost::ignore_unused(playlist);

        // TODO: readInformation "from last stop"
        nlohmann::json cmd;
        nlohmann::json cmdList;
        cmdList.emplace_back("loadlist");
        cmdList.emplace_back(url);
        cmd["command"] = cmdList;

        set_command(cmd.dump());

        return true;
    }

    virtual bool stop()
    {
        nlohmann::json cmd;
        nlohmann::json cmdList;
        cmdList.emplace_back("stop");
        cmd["command"] = cmdList;

        set_command(cmd.dump());
        return true;
    }

    virtual bool seek_forward()
    {
        return true;
    }

    virtual bool seek_backward()
    {
        return true;
    }

    virtual bool next_file()
    {
        nlohmann::json cmd;
        nlohmann::json cmdList;
        cmdList.emplace_back("playlist-next");
        cmd["command"] = cmdList;

        set_command(cmd.dump());
        return true;
    }

    virtual bool prev_file()
    {
        nlohmann::json cmd;
        nlohmann::json cmdList;
        cmdList.emplace_back("playlist-prev");
        cmd["command"] = cmdList;

        set_command(cmd.dump());
        return true;
    }

    virtual bool pause()
    {
        m_pause = !m_pause;
        nlohmann::json cmd;
        nlohmann::json cmdList;
        cmdList.emplace_back("set_property");
        cmdList.emplace_back("pause");
        cmdList.emplace_back(m_pause);
        cmd["command"] = cmdList;

        set_command(cmd.dump());
        return true;
    }

    virtual bool isPlaying()
    {
        return true;
    }

    void setPlayerEndCB(const std::function<void(const std::string& )>& endfunc);
};

#endif // MPVPLAYER_H
