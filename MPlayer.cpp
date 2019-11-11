#include "MPlayer.h"
#include <regex>
#include <boost/regex.hpp>
#include <iostream>

MPlayer::MPlayer(boost::asio::io_service& service, const std::string& configDB, const std::string &logFilePath) :
        Player(configDB, logFilePath), m_service(service)
{
}

void MPlayer::handleEnd()
{
    if (!m_stopTime.empty()) {
        // is there is stop position
        // is there an entry, renew it, else write new one
        auto it = std::find_if(m_stopInfoList.begin(), m_stopInfoList.end(),
                               [this](const StopInfoEntry& elem)
                               { return  elem.fileName == filename; });

        if (it != m_stopInfoList.end()) {
            it->stopTime=m_stopTime;
            it->valid=true;
        } else {
            m_stopInfoList.push_back({filename, m_stopTime, true});
        }
    }

    if (m_endfunc)
        m_service.post([this](){m_endfunc(m_stopTime);});

}



void MPlayer::readPlayerOutput(const boost::system::error_code &ec, std::size_t size) {

    if (size > 0) {
        std::istream is(&m_streambuf);
        std::string line;
        while (std::getline(is, line, '\r')) { /* would be nice to separate with \n as well, but ...*/

            if (line.empty())
                continue;
            // do not log
            if (line.substr(0,2) != "A:")
                log << "-> " << line << "\n" << std::flush;

            std::smatch match{};
            const std::regex pattern1{"A:[\\s]*(.*) V:"};
            if (std::regex_search(line, match, pattern1)) {
                uint32_t stopTime = std::atof(match[1].str().c_str());
                m_stopTime_tmp = std::to_string(stopTime/3600) +":"+std::to_string((stopTime/60)%60)+":"+std::to_string(stopTime%3600);
            }

            const std::regex pattern2{"Exiting... \\(Quit\\)"};
            if (std::regex_search(line, match, pattern2)) {
                log << "end tag found\n" << std::flush;
                m_stopTime = m_stopTime_tmp;
            }
        }
    }
    if (!ec)
        boost::asio::async_read_until(*(m_pipe.get()), m_streambuf,boost::regex("[\r|\n]"),
                                      [this](const boost::system::error_code &ec, std::size_t size){
                                          log << "read until called again\n"<<std::flush;
                                          readPlayerOutput(ec, size);
                                      });
    else {
        handleEnd();
        log<<"Error: "<<ec.message()<<"\n"<<std::flush;
        child->wait();

        child.reset();
        m_in.reset();
        m_pipe.reset();

        m_playing = false;
        if (m_startPending) {
            m_service.post([this]() { startPlay(playerStartInfo.url, playerStartInfo.playerInfo,
                                                playerStartInfo.fromLastStop); });
            m_startPending = false;
        }
        if (m_stopPending) {
            m_stopPending = false;
        }

    }
}

bool MPlayer::startPlay(const std::string &url, const std::string& playerInfo, bool fromLastStop) {
    if (m_playing) {
        if (m_stopPending) {
            playerStartInfo.url = url;
            playerStartInfo.playerInfo = playerInfo;
            playerStartInfo.fromLastStop = fromLastStop;
            m_startPending = true;
        }
        return false;
    }

    std::vector<std::string> parameter;

    m_stopTime.clear();
    // extract filename
    filename = extractName(url);

    parameter.push_back("-slave");

    if (!filename.empty() && fromLastStop) {
        auto it = std::find_if(m_stopInfoList.begin(), m_stopInfoList.end(),
                                      [this](const auto &elem) {
                                          return elem.fileName == filename;
                                      });
        if (it != m_stopInfoList.end() && it->valid) {
            parameter.push_back("-ss");
            parameter.push_back(it->stopTime);
            // as this file is started, the old start time is not valid any more
            it->valid = false;
        }
    }

    parameter.push_back("-playlist");

    std::cerr << "starting player -> " << playerName << " ";
    for (auto &i : parameter)
        std::cerr << i << " ";
    std::cerr << url << "\n" << std::flush;

    m_in = std::make_unique<boost::process::opstream>();
    m_pipe = std::make_unique<boost::process::async_pipe>(m_service);
    child = std::make_unique<boost::process::child>(playerName, parameter, url,
                                                    boost::process::std_out > *(m_pipe.get()),
                                                    boost::process::std_err > boost::process::null,
                                                    boost::process::std_in < *(m_in.get()), m_service);

    boost::asio::async_read_until(*(m_pipe.get()), m_streambuf, boost::regex("(\r|\n)"),
                                  [this](const boost::system::error_code &ec, std::size_t size) {
                                      log << "read until called\n"<<std::flush;
                                      readPlayerOutput(ec, size);
                                  });

    m_playing = true;
    return true;
}

bool MPlayer::seek_forward() {
    if (!m_playing)
        return false;
    *m_in.get() << "seek 30 0\n" << std::flush;
    return true;
}

bool MPlayer::seek_backward() {
    if (!m_playing)
        return false;
    *m_in.get() << "seek -30 0\n" << std::flush;
    return true;
}

bool MPlayer::next_file() {
    if (!m_playing)
        return false;
    *m_in.get() << "pt_step 1\n" << std::flush;
    return true;
}

bool MPlayer::prev_file() {
    if (!m_playing)
        return false;
    *m_in.get() << "pt_step -1\n" << std::flush;
    return false;
}

bool MPlayer::pause() {
    if (!m_playing)
        return false;
    *m_in.get() << "pause\n" << std::flush;
    return true;
}

bool MPlayer::stop() {
    if (!m_playing)
        return false;
    *m_in.get() << "quit\n" << std::flush;
    log << "waiting for player to stop\n"<<std::flush;
    m_stopPending = true;
    return true;
}

bool MPlayer::isPlaying() {
    return m_playing;
}

