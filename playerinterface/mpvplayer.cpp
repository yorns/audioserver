#include "mpvplayer.h"

using namespace LoggerFramework;

void MpvPlayer::init_communication() {
    logger(Level::info) << "connecting mpv player at <" << m_accessPoint << ">\n";
    boost::system::error_code ec;
    m_socket.connect(m_accessPoint, ec);
    if (ec) {
        logger(Level::warning) << "connecting mpv player - FAILED: mpv peer not available\n";
        abort();
    }
    logger(Level::info) << "connecting mpv player - DONE\n";

    boost::asio::async_read_until(m_socket, boost::asio::dynamic_string_buffer(m_stringBuffer), '\n',
                                  [this](const boost::system::error_code& error, std::size_t bytes_transferred)
    { read_handler(error, bytes_transferred); });

    logger(Level::info) << "MPV player interface established\n";
}

void MpvPlayer::init_MpvCommandHandling() {

    /* set observers */
    if ( sendCommand({"observe_property", 1, "percent-pos"}) &&
         sendCommand({"observe_property", 2, "metadata"}) ) {
        logger(Level::info) << "all MPV observers set up\n";
    }

    m_propertiesHandler["percent-pos"] = [this](const nlohmann::json& msg) {
        m_actTime = msg.at("data");
        if (m_actTime > 1 )
            m_isPlaying = true;
    };

    m_propertiesHandler["metadata"] = [this](const nlohmann::json& msg) {
        nlohmann::json actmeta = msg.at("data");
        m_AudioFileName = actmeta["title"];
        m_AudioFileName += " - ";
        m_AudioFileName += actmeta["artist"];
    };
}

bool MpvPlayer::doPlayFile(const std::string &uniqueId) {
    std::string filename = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Audio) + '/' + uniqueId + ".mp3";
    logger(LoggerFramework::Level::debug) << "Send Command to play file <"<<filename<<">\n";
    m_isPlaying = true;
    return sendCommand({"loadfile", filename});
}

bool MpvPlayer::needsOnlyUnpause(const std::string &playlist){
    return m_PlaylistUniqueId == playlist && m_isPlaying && m_pause;
}

bool MpvPlayer::needsStop() {
    return m_isPlaying ;
}

MpvPlayer::MpvPlayer(boost::asio::io_context &context)
    : m_socket(context)
{
    init_communication();
    init_MpvCommandHandling();
}

bool MpvPlayer::startPlay(const std::vector<std::string> &list, const std::string &playlistUniqueId, const std::string &playlistName)
{

    // just unpause if player is in playing state and paused
    if (needsOnlyUnpause(playlistUniqueId))
        return pause_toggle();

    m_PlaylistName = playlistName;
    m_PlaylistUniqueId = playlistUniqueId;
    m_playlist_orig.clear();
    m_playlist_orig.insert(std::begin(m_playlist_orig), std::cbegin(list), std::cend(list));
    m_playlist.clear();
    m_playlist.insert(std::begin(m_playlist), std::cbegin(m_playlist_orig), std::cend(m_playlist_orig));
    bool shuffelingNeeded = m_shuffle;
    m_shuffle = false;

    if (m_playlist.empty()) {
        logger(LoggerFramework::Level::warning) << "playlist <"<<playlistUniqueId<<"> (" << playlistName <<") is empty\n";
        return false;
    }

    if (shuffelingNeeded) {
        logger(LoggerFramework::Level::debug) << "shuffeling enabled. New playlist needs shuffeling\n";
        doShuffle(true);
    }

    m_currentItemIterator = m_playlist.begin();

    sendCommand({"set_property", "pause", m_pause = false});

    if (needsStop()) {
        return sendCommand({"stop"});
        m_nextPlaylistDirection = MpvPlayer::NextPlaylistDirection::current;
    }
    else {
        doPlayFile(*m_currentItemIterator);
        m_nextPlaylistDirection = MpvPlayer::NextPlaylistDirection::next;
    }

    return true;

}

bool MpvPlayer::doPlayPreviousFileInList() {
    if (m_currentItemIterator == m_playlist.end() || m_isPlaying == false)
        return false;

    if (m_currentItemIterator != m_playlist.begin())
    {
        m_currentItemIterator--;
    }
    else
        return true; // keep current audio file

    return true;
}

bool MpvPlayer::doPlayNextFileInList() {
    if (m_currentItemIterator == m_playlist.end() || m_isPlaying == false)
        return false;

    if (++m_currentItemIterator == m_playlist.end())
    {
        logger(LoggerFramework::Level::debug) << "Playlist <"<<m_PlaylistUniqueId<<"> (" << m_PlaylistName << ") ends\n";
        if (m_doCycle) {
            logger(LoggerFramework::Level::debug) << "do cycle and play playlist from start again\n";
            if (m_shuffle) {
                doShuffle(true);
            }
            m_currentItemIterator = m_playlist.begin();
            logger(LoggerFramework::Level::debug) << "Playlist <"<<m_PlaylistUniqueId<<"> ("
                                                  << m_PlaylistName << ") playing first sond <"
                                                  << *m_currentItemIterator << ">\n";
            return  true;
        }

        return false;
    }
    else {
        logger(LoggerFramework::Level::debug) << "Playlist <"<<m_PlaylistUniqueId<<"> ("
                                              << m_PlaylistName << ") playing next song <"
                                              << *m_currentItemIterator << ">\n";
        return true;
    }
}

bool MpvPlayer::stop()
{
    m_nextPlaylistDirection = MpvPlayer::NextPlaylistDirection::stop;
    return sendCommand({"stop"});
}

bool MpvPlayer::seek_forward()
{
    return sendCommand({"seek",ServerConstant::seekForwardSeconds});
}

bool MpvPlayer::seek_backward()
{
    return sendCommand({"seek",ServerConstant::seekBackwardSeconds});
}

bool MpvPlayer::next_file()
{
    logger(LoggerFramework::Level::debug) << "user request: play next file\n";
    m_nextPlaylistDirection = MpvPlayer::NextPlaylistDirection::next;
    return sendCommand({"stop"});
}

bool MpvPlayer::prev_file()
{
    logger(LoggerFramework::Level::debug) << "user request: play prev file\n";
    m_nextPlaylistDirection = MpvPlayer::NextPlaylistDirection::previous;
    return sendCommand({"stop"});
}

bool MpvPlayer::pause_toggle()
{
    m_pause = !m_pause;
    return sendCommand({"set_property", "pause", m_pause});
}

bool MpvPlayer::isPlaying()
{
    return m_isPlaying;
}

bool MpvPlayer::stopPlayerConnection()
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

const std::string MpvPlayer::getSongName() const { return m_AudioFileName; }

std::string MpvPlayer::getSongID() { return (m_currentItemIterator != std::end(m_playlist))?*m_currentItemIterator:""; }

uint32_t MpvPlayer::getSongPercentage() { if (m_actTime > 100.0) m_actTime = 100.0; return static_cast<uint32_t>(m_actTime*100); }

bool MpvPlayer::jump_to_position(int percent) { return sendCommand({"seek", percent, "absolute-percent"}); }

bool MpvPlayer::jump_to_fileUID(const std::string &fileId)
{
    auto it = std::find_if(std::begin(m_playlist),
                           std::end(m_playlist),
                           [&fileId](const std::string& uniqueId){ return uniqueId == fileId; });
    if (it != std::end(m_playlist)) {
        logger(Level::info) << "change to file id from <" << *it << "> to <"<< fileId << ">\n";
        if (m_isPlaying) {
            if (m_songEndCallback) m_songEndCallback(*m_currentItemIterator);
            m_currentItemIterator = it;
            m_nextPlaylistDirection = MpvPlayer::NextPlaylistDirection::current;
            return sendCommand({"stop"});
        }
        else {
            m_currentItemIterator = it;
            return doPlayFile(*m_currentItemIterator);
            m_nextPlaylistDirection = MpvPlayer::NextPlaylistDirection::next;
        }
    }
    else {
        logger(Level::warning) << "file <" << fileId << "> not in playlist\n";
        return false;
    }
}

void MpvPlayer::read_handler(const boost::system::error_code &error, std::size_t bytes_transferred) {

    if (error)
        return;

    std::string line = m_stringBuffer.substr(0, bytes_transferred);
    m_stringBuffer.erase(0,bytes_transferred);

    //logger(Level::debug) << "line: "<<line.size()<<" - " << line;
    try {
        nlohmann::json returnData = nlohmann::json::parse(line);

        if (returnData.find("event") != returnData.end()) {

            std::string event { returnData.at("event") };

            if (event == "property-change") {
                auto it = m_propertiesHandler.find(returnData.at("name"));
                if (it != m_propertiesHandler.end()) {
                    it->second(returnData);
                }
            }
            else if (event == "end-file") {
                logger(Level::debug) << "End file event found on <" << *m_currentItemIterator << ">.\n";
                    switch (m_nextPlaylistDirection) {
                    case MpvPlayer::NextPlaylistDirection::next: {
                        if (doPlayNextFileInList()) {
                            if (m_songEndCallback) m_songEndCallback(*m_currentItemIterator);
                            doPlayFile(*m_currentItemIterator);
                        }
                        else {
                            if (m_isPlaying) {
                            logger(Level::debug) << "no next file given, playlist ended\n";
                            if (m_songEndCallback) m_songEndCallback(*m_currentItemIterator);
                            if (m_playlistEndCallback) m_playlistEndCallback();
                            m_isPlaying = false;
                            m_currentItemIterator = m_playlist.end();
                            }
                            else
                                logger(Level::info) << "player not in playing mode for <play next>\n";
                        }
                        break;
                    }
                    case MpvPlayer::NextPlaylistDirection::previous: {
                        if (doPlayPreviousFileInList()) {
                            if (m_songEndCallback) m_songEndCallback(*m_currentItemIterator);
                            doPlayFile(*m_currentItemIterator);
                        }
                        else {
                            if (m_isPlaying) {
                            logger(Level::debug) << "no previous file given, playlist ended\n";
                            if (m_songEndCallback) m_songEndCallback(*m_currentItemIterator);
                            if (m_playlistEndCallback) m_playlistEndCallback();
                            m_isPlaying = false;
                            m_currentItemIterator = m_playlist.end();
                            }
                            else
                                logger(Level::info) << "player not in playing mode for <play previous>\n";

                        }
                        break;
                    }
                    case MpvPlayer::NextPlaylistDirection::current: {
                        doPlayFile(*m_currentItemIterator);
                        break;
                    }
                    case MpvPlayer::NextPlaylistDirection::stop: {
                        if (m_isPlaying) {
                            if (m_songEndCallback) m_songEndCallback(*m_currentItemIterator);
                            if (m_playlistEndCallback) m_playlistEndCallback();
                            m_isPlaying = false;
                            m_currentItemIterator = m_playlist.end();
                        }
                        else
                            logger(Level::info) << "player not in playing mode for <play stop>\n";

                        break;
                    }
            }
            m_actTime = 0;
            m_nextPlaylistDirection = MpvPlayer::NextPlaylistDirection::next; // set default;
            }
            else if (event == "start-file") {
                logger(Level::debug) << "start file event found.\n";
                m_actTime = 0;
                m_isPlaying = true;
            }
            else  {
                logger(Level::debug) << "unknown event " << line;
            }
        }

        if (returnData.find("error") != returnData.end()) {
            std::string error = returnData.at("error");
            if (m_expectedAnswer.empty()) {
                logger(Level::warning) << "answer received that is not expected: " << error << "\n";
            } else {
                std::string& expected = m_expectedAnswer.front();
                if (error != expected) {
                    logger(Level::warning) << "received unexpected answer: " << error << "\n";
                }
                m_expectedAnswer.pop_front();
            }
        }

    } catch (nlohmann::json::exception& ex) {
        boost::ignore_unused(ex);
        //                    logger(Level::warning) << "cannot read json: "<< ex.what() << "\n";
        //                    logger(Level::debug) << "line: "<< line << "\n";
    }

    boost::asio::async_read_until(m_socket, boost::asio::dynamic_string_buffer(m_stringBuffer), '\n',
                                  [this](const boost::system::error_code& error, std::size_t bytes_transferred)
    { read_handler(error, bytes_transferred); });

}

bool MpvPlayer::sendData(std::string &&cmd) {

    logger(Level::debug) << "player interface sends "<<cmd<<"\n";
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

bool MpvPlayer::sendCommand(std::vector<std::variant<const char *, std::string, bool, int> > &&argumentList) {

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

