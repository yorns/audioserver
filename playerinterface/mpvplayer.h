#ifndef MPVPLAYER_H
#define MPVPLAYER_H

#include "Player.h"
#include <variant>
#include <string>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/core/ignore_unused.hpp>
#include "common/logger.h"

using boost::asio::local::stream_protocol;

class MpvPlayer : public Player
{
    boost::asio::io_context& m_context;
    boost::asio::streambuf m_readBuffer;
    stream_protocol::socket m_socket;

    std::string m_accessPoint {"/tmp/mpvsocket"};
    bool m_isPlaying {false};
    bool m_pause {false};
    float actTime { 0.0 };
    std::string actFile;
    std::string actHumanReadable;

    void write_handler( const boost::system::error_code& , std::size_t ) {

    }

    void read_handler(const boost::system::error_code& error, std::size_t bytes_transferred) {

        if (error)
            return;

        std::string line(boost::asio::buffer_cast<const char*>(m_readBuffer.data()), bytes_transferred);
        m_readBuffer.consume(bytes_transferred);

        logger(Level::debug) << "line: "<<line.size()<<" - " << line;
        try {
            nlohmann::json returnData;
            returnData = nlohmann::json::parse(line);

                if (returnData.at("event") == "property-change" && returnData.at("name") == "percent-pos") {
                    actTime = returnData.at("data");
                    if (actTime > 1 )
                        m_isPlaying = true;
                }
                else if (returnData.at("event") == "property-change" && returnData.at("name") == "filename/no-ext") {
                    actFile = returnData.at("data");
                }
                else if (returnData.at("event") == "property-change" && returnData.at("name") == "metadata") {
                    nlohmann::json actmeta = returnData.at("data");
                    actHumanReadable = actmeta["title"];
                    actHumanReadable += " - ";
                    actHumanReadable += actmeta["artist"];
                }
                else if (returnData.at("event") == "idle") {
                    m_isPlaying = false;
                    if (m_endfunc) m_endfunc(actFile);
                }
                else if (returnData.at("event") == "end-file") {
                    if (m_songEndfunc) m_songEndfunc();
                }
                else if (returnData.at("event") == "start-file") {
                    actTime = 0;
                    m_isPlaying = true;
                }
                else  {
                    logger(Level::debug) << "unknown event " << line << "\n";
                }

        } catch (nlohmann::json::exception& ex) {
            boost::ignore_unused(ex);
            //                    logger(Level::warning) << "cannot read json: "<< ex.what() << "\n";
            //                    logger(Level::debug) << "line: "<< line << "\n";
        }

        boost::asio::async_read_until(m_socket, m_readBuffer, "\n",
                                      [this](const boost::system::error_code& error, std::size_t bytes_transferred)
        { read_handler(error, bytes_transferred); });

    }

    bool set_command(std::string&& cmd) {

        logger(Level::debug) << "player interface sends "<<cmd<<"\n";
        cmd += "\n";

        if (m_socket.is_open()) {
            m_socket.async_send(boost::asio::buffer(cmd),
                                [this](const boost::system::error_code& error, std::size_t bytes_transferred)
            { write_handler(error, bytes_transferred); });
            return true;
        }

        return false;
    }

    bool sendCommand(std::vector<std::variant<const char*, std::string, bool>>&& argumentList){

        nlohmann::json cmd;
        nlohmann::json cmdList;
        for(auto& i : argumentList) {
            if (auto j = std::get_if<std::string>(&i))
                cmdList.emplace_back(*j);
            if (auto j = std::get_if<bool>(&i))
                cmdList.emplace_back(*j);
            if (auto j = std::get_if<const char*>(&i))
                cmdList.emplace_back(*j);
        }
        cmd["command"] = cmdList;

        return set_command(cmd.dump());

    }

public:
    explicit MpvPlayer(boost::asio::io_context& context, const std::string& configFile)
        : Player(configFile), m_context(context), m_socket(m_context)
    {
        logger(Level::info) << "connecting mpv player at <"<<m_accessPoint << ">\n";
        m_socket.connect(m_accessPoint);

        logger(Level::info) << "establish mpv receiver\n";
        boost::asio::async_read_until(m_socket, m_readBuffer, "\n",
                               [this](const boost::system::error_code& error, std::size_t bytes_transferred)
        { read_handler(error, bytes_transferred); });

        logger(Level::info) << "MPV player interface established\n";

        /* set observers */
//        std::string observerTime{"{ \"command\": [\"observe_property\", 1, \"percent-pos\"] }"};
        set_command("{ \"command\": [\"observe_property\", 1, \"percent-pos\"] }");

        std::string observerFilename{"{ \"command\": [\"observe_property\", 2, \"filename/no-ext\"] }"};
        set_command("{ \"command\": [\"observe_property\", 2, \"filename/no-ext\"] }"); // std::move(observerFilename));

        std::string observerMetadata{"{ \"command\": [\"observe_property\", 3, \"metadata\"] }"};
        set_command("{ \"command\": [\"observe_property\", 3, \"metadata\"] }"); // std::move(observerMetadata));

    }

    MpvPlayer() = delete;

    virtual ~MpvPlayer() = default;

    virtual bool startPlay(const std::string &&url, const std::string&& playlist, bool fromLastStop = false)
    {
        boost::ignore_unused(fromLastStop);
        boost::ignore_unused(playlist);

        if (m_isPlaying)
            stop();

        // unpause (just to be sure)
        sendCommand({"set_property", "pause", false});

        return sendCommand({"loadlist", std::move(url)});
    }

    virtual bool stop()
    {
        m_isPlaying = false;
        return sendCommand({"stop"});
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
        return sendCommand({"playlist-next"});
    }

    virtual bool prev_file()
    {
        return sendCommand({"playlist-prev"});
    }

    virtual bool pause()
    {
        m_pause = !m_pause;
        return sendCommand({"set_property", "pause", m_pause});
    }

    virtual bool isPlaying()
    {
        return m_isPlaying;
    }

    virtual bool stopPlayer() {
        m_isPlaying = false;
        if (m_socket.is_open()) {
            boost::system::error_code ec;
            m_socket.close(ec);
            if (ec) {
                logger(Level::warning) << "not able to close socket\n";
                return false;
            }
        }
        else {
            return false;
        }
        return true;
    }

    virtual std::string getSong() final { return actHumanReadable; }
    virtual std::string getSongID() final { return actFile; }
    virtual uint32_t getSongPercentage() final { if (actTime > 100.0) actTime = 100.0; return static_cast<uint32_t>(actTime*100); }

    virtual void selectPlaylistsEntry(uint32_t id) final {
    boost::ignore_unused(id);
    }

};

#endif // MPVPLAYER_H
