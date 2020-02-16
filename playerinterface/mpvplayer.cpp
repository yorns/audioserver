#include "mpvplayer.h"

void MpvPlayer::init_communication() {
    logger(LoggerFramework::Level::info) << "connecting mpv player at <" << m_accessPoint << ">\n";
    boost::system::error_code ec;
    m_socket.connect(m_accessPoint, ec);
    if (ec) {
        logger(LoggerFramework::Level::info) << "connecting mpv player - FAILED: mpv peer not available\n";
        abort();
    }
    logger(LoggerFramework::Level::info) << "connecting mpv player - DONE\n";

    boost::asio::async_read_until(m_socket, boost::asio::dynamic_string_buffer(m_stringBuffer), '\n',
                                  [this](const boost::system::error_code& error, std::size_t bytes_transferred)
    { read_handler(error, bytes_transferred); });

    logger(LoggerFramework::Level::info) << "MPV player interface established\n";
}

void MpvPlayer::init_MpvCommandHandling() {
    /* set observers */
    if ( sendData(R"({ "command": ["observe_property", 1, "percent-pos"] })") &&
         sendData(R"({ "command": ["observe_property", 2, "filename/no-ext"] })") &&
         sendData(R"({ "command": ["observe_property", 3, "metadata"] })")) {
        logger(LoggerFramework::Level::info) << "all MPV observers established\n";
    }

    propertiesHandler["percent-pos"] = [this](const nlohmann::json& msg) {
        actTime = msg.at("data");
        if (actTime > 1 )
            m_isPlaying = true;
    };

    propertiesHandler["filename/no-ext"] = [this](const nlohmann::json& msg) {
        actFile = msg.at("data");
    };

    propertiesHandler["metadata"] = [this](const nlohmann::json& msg) {
        nlohmann::json actmeta = msg.at("data");
        actHumanReadable = actmeta["title"];
        actHumanReadable += " - ";
        actHumanReadable += actmeta["artist"];
    };
}

void MpvPlayer::read_handler(const boost::system::error_code &error, std::size_t bytes_transferred) {

    if (error)
        return;

    std::string line = m_stringBuffer.substr(0, bytes_transferred);
    m_stringBuffer.erase(0,bytes_transferred);

    logger(LoggerFramework::Level::debug) << "line: "<<line.size()<<" - " << line;
    try {
        nlohmann::json returnData = nlohmann::json::parse(line);

        if (returnData.find("event") != returnData.end()) {

            std::string event { returnData.at("event") };

            if (event == "property-change") {
                auto it = propertiesHandler.find(returnData.at("name"));
                if (it != propertiesHandler.end()) {
                    it->second(returnData);
                }
            }
            else if (event == "idle") {
                m_isPlaying = false;
                if (m_playlistEndCallback) m_playlistEndCallback();
            }
            else if (event == "end-file") {
                if (m_songEndCallback) m_songEndCallback(actFile);
            }
            else if (event == "start-file") {
                actTime = 0;
                m_isPlaying = true;
            }
            else  {
                logger(LoggerFramework::Level::debug) << "unknown event " << line << "\n";
            }
        }

        if (returnData.find("error") != returnData.end()) {
            std::string error = returnData.at("error");
            if (m_expectedAnswer.empty()) {
                logger(LoggerFramework::Level::warning) << "answer received that is not expected: " << error << "\n";
            } else {
                std::string& expected = m_expectedAnswer.front();
                if (error != expected) {
                    logger(LoggerFramework::Level::warning) << "received unexpected answer: " << error << "\n";
                }
                m_expectedAnswer.pop_front();
            }
        }

    } catch (nlohmann::json::exception& ex) {
        boost::ignore_unused(ex);
        //                    logger(LoggerFramework::Level::warning) << "cannot read json: "<< ex.what() << "\n";
        //                    logger(LoggerFramework::Level::debug) << "line: "<< line << "\n";
    }

    boost::asio::async_read_until(m_socket, boost::asio::dynamic_string_buffer(m_stringBuffer), '\n',
                                  [this](const boost::system::error_code& error, std::size_t bytes_transferred)
    { read_handler(error, bytes_transferred); });

}

bool MpvPlayer::sendData(std::string &&cmd) {

    logger(LoggerFramework::Level::debug) << "player interface sends "<<cmd<<"\n";
    cmd += "\n";

    if (m_socket.is_open()) {
        m_socket.async_send(boost::asio::buffer(cmd),
                            [this](const boost::system::error_code& error, std::size_t bytes_transferred)
        {
            boost::ignore_unused(error);
            boost::ignore_unused(bytes_transferred);

            m_expectedAnswer.push_back("success");
        });
        return true;
    }

    return false;
}

bool MpvPlayer::sendCommand(std::vector<std::variant<const char *, std::string, bool, int> > &&argumentList){

    nlohmann::json cmd;
    nlohmann::json cmdList;
    for(auto& i : argumentList) {
        if (auto j = std::get_if<std::string>(&i))
            cmdList.emplace_back(*j);
        if (auto j = std::get_if<bool>(&i))
            cmdList.emplace_back(*j);
        if (auto j = std::get_if<const char*>(&i))
            cmdList.emplace_back(*j);
        if (auto j = std::get_if<int>(&i))
            cmdList.emplace_back(*j);
    }
    cmd["command"] = cmdList;

    return sendData(cmd.dump());

}

