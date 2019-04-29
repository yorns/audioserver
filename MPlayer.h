#ifndef SIMPLEMOVIEUI_MPLAYER_H
#define SIMPLEMOVIEUI_MPLAYER_H

#include <boost/process.hpp>
#include <boost/asio.hpp>
#include "Player.h"

class MPlayer : public Player {

    boost::asio::io_service& m_service;
    std::unique_ptr<boost::process::opstream> m_in;
    std::unique_ptr<boost::process::child> child;

    std::string m_stopTime_tmp;
    std::string m_stopTime;

    bool m_playing{false};
    const char *playerName{"/usr/bin/mplayer"};

    boost::asio::streambuf m_streambuf;

    std::unique_ptr<boost::process::async_pipe> m_pipe;

    void readPlayerOutput(const boost::system::error_code &ec, std::size_t size);

    std::string filename;

    void handleEnd();

    uint32_t position {0};

public:

    MPlayer(boost::asio::io_service& service, const std::string& configDB, const std::string &logFilePath);

    bool startPlay(const std::string &url, const std::string& playerInfo, bool fromLastStop = false) final;
    bool stop() final;
    bool seek_forward() final;
    bool seek_backward() final;
    bool next_file() final;
    bool prev_file() final;
    bool pause() final;

    bool isPlaying() final;

};


#endif //SIMPLEMOVIEUI_MPLAYER_H
