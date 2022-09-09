#include "Player.h"
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <common/logger.h>
#include "common/Constants.h"

using namespace LoggerFramework;

bool BasePlayer::doShuffle(bool shuffleRequest) {
    if (shuffleRequest) {
        if (m_shuffle)
            try {
            logger(Level::debug) << "Shuffeling playlist <"
                                 << boost::lexical_cast<std::string>(m_playlistUniqueId) << "> ("
                                 << m_playlistName << ") again\n";
        } catch (std::exception& ex) {
            logger(LoggerFramework::Level::warning) << ex.what() << "\n";
        }

        else {
            try {
                logger(Level::debug) << "Shuffeling playlist <"
                                     <<   boost::lexical_cast<std::string>(m_playlistUniqueId) << "> ("
                                       << m_playlistName << ")\n";
            } catch (std::exception& ex) {
                logger(LoggerFramework::Level::warning) << ex.what() << "\n";
            }

            m_playlist.clear();
            m_playlist.insert(std::begin(m_playlist), std::cbegin(m_playlist_orig), std::cend(m_playlist_orig));
        }
        m_shuffle = true;
        std::shuffle(std::begin(m_playlist), std::end(m_playlist), m_rng);

        logger(LoggerFramework::Level::debug) << "Nr) /t original /t->  shuffled\n";
        for(uint32_t i{0}; i < m_playlist.size(); ++i) {
            try {
            logger(LoggerFramework::Level::debug) << i << ") /t"
                                                  << boost::lexical_cast<std::string>(m_playlist_orig[i].m_uniqueId)
                                                  << " \t-> "
                                                  << boost::lexical_cast<std::string>(m_playlist[i].m_uniqueId)
                                                  << "\n";
            } catch (std::exception& ex) {
                logger(LoggerFramework::Level::warning) << ex.what() << "\n";
            }

        }

        return true;
    }
    else if (!shuffleRequest && m_shuffle) {
        logger(Level::debug) << "Reset Shuffling for playlist <"
                             << boost::lexical_cast<std::string>(m_playlistUniqueId)
                             << "> (" << m_playlistName << ")\n";
        m_shuffle = false;
        m_playlist.clear();
        m_playlist.insert(std::begin(m_playlist), std::cbegin(m_playlist_orig), std::cend(m_playlist_orig));
        return true;
    }
    logger(Level::warning) << "Shuffling request for playlist <"
                           << boost::lexical_cast<std::string>(m_playlistUniqueId)
                           << "> (" << m_playlistName << ") failed (request: "
                           << (shuffleRequest?"doShuffle":"doUnshuffle") << " and state: "
                           << (m_shuffle?"shuffel":"normal") << "\n";
    return false;
}

void BasePlayer::updateUi() const {
    if (!m_onUiChangeHandler) {
        logger(Level::debug) << "Websocket playlist Update not set\n";
        return;
    }

    if (isPlaying()) {
        try {
        logger(Level::debug) << "Websocket playlist Update with SongID <"<< boost::lexical_cast<std::string>(getSongID()) << "> and playlistID <"<< boost::lexical_cast<std::string>(getPlaylistID())<<">\n";
        } catch (std::exception& ex) {
            logger(LoggerFramework::Level::warning) << ex.what() << "\n";
        }

        m_onUiChangeHandler(getSongID(), getPlaylistID(), getSongPercentage()/100, getLoop(), getShuffle());
    }
    else {
        try {
        logger(Level::debug) << "Websocket playlist Update with playlistID <"<<boost::lexical_cast<std::string>(getPlaylistID())<<">\n";
        } catch (std::exception& ex) {
            logger(LoggerFramework::Level::warning) << ex.what() << "\n";
        }
        boost::uuids::uuid unknown; // = boost::lexical_cast<boost::uuids::uuid>(std::string(ServerConstant::unknownCoverFileUid));
        m_onUiChangeHandler(unknown,
                            getPlaylistID(), 0, getLoop(), getShuffle());
    }
}

bool BasePlayer::needsOnlyUnpause(const boost::uuids::uuid &playlist){
    return m_playlistUniqueId == playlist && m_isPlaying && m_pause;
}

bool BasePlayer::needsStop() {
    return m_isPlaying ;
}

bool BasePlayer::calculatePreviousFileInList() {
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

bool BasePlayer::calculateNextFileInList() {
    if (m_currentItemIterator == m_playlist.end() || !m_isPlaying) {
        logger(LoggerFramework::Level::debug) << "player not in playing mode\n";
        return false;
    }

    if (m_single) {
        logger(LoggerFramework::Level::debug) << "playing single file ended\n";
        return false;
    }

    if (++m_currentItemIterator == m_playlist.end())
    {
        logger(LoggerFramework::Level::debug) << "Playlist <"
                                              << boost::uuids::to_string(m_playlistUniqueId)
                                              << "> (" << m_playlistName << ") ends\n";
        if (m_doCycle) {
            logger(LoggerFramework::Level::debug) << "do cycle and play playlist from start again\n";
            if (m_shuffle) {
                doShuffle(true);
            }
            m_currentItemIterator = m_playlist.begin();
            logger(LoggerFramework::Level::debug) << "Playlist <"
                                                  << boost::uuids::to_string(m_playlistUniqueId)
                                                  << "> (" << m_playlistName << ") playing first sond <"
                                                  << boost::uuids::to_string(m_currentItemIterator->m_uniqueId)
                                                  << ">\n";
            return  true;
        }
        else {
            // keep last audio item
            m_currentItemIterator--;
        }

        return false;
    }
    else {
        logger(LoggerFramework::Level::debug) << "Playlist <"<<boost::uuids::to_string(m_playlistUniqueId)<<"> ("
                                              << m_playlistName << ") playing next song <"
                                              << boost::uuids::to_string(m_currentItemIterator->m_uniqueId)
                                              << ">\n";
        return true;
    }
}

bool BasePlayer::toggleShuffle() {

    doShuffle(!m_shuffle);

    // in case, player is playing, end this song and use next song from
    if (isPlaying()) {
        stopAndRestart();
    }
    updateUi();
    return true;
}

bool BasePlayer::toggleLoop() {
    m_doCycle = !m_doCycle;
    updateUi();
    return true;
}

bool BasePlayer::toggleSingle() {
    m_single = !m_single;
    logger(LoggerFramework::Level::info) << "toggle single to <"<<(m_single?"true":"false")<<">\n";
    updateUi();
    return true;
}


void BasePlayer::setPlaylistEndCB(PlaylistEndCallback&& endfunc) {
    m_playlistEndCallback = std::move(endfunc);
}

void BasePlayer::setSongEndCB(SongEndCallback&& endfunc) {
    m_songEndCallback = std::move(endfunc);
}

void BasePlayer::resetPlayer() {

    m_shuffle = false;
    m_doCycle = false;
    m_isPlaying = false;
    m_pause = false;
    m_single = false;

    m_playlist.clear();
    m_playlist_orig.clear();
    m_currentItemIterator = m_playlist.end();
    m_playlistUniqueId = boost::uuids::uuid();
    m_playlistName = "";

}

bool BasePlayer::setPlaylist(const Common::AlbumPlaylistAndNames &albumPlaylistAndNames) {

    if (albumPlaylistAndNames.m_playlist.empty()) {
        logger(LoggerFramework::Level::warning) << "playing failed, no album list set\n";
        return false;
    }

    m_playlist = albumPlaylistAndNames.m_playlist;
    m_playlist_orig = albumPlaylistAndNames.m_playlist;
    m_currentItemIterator = m_playlist.begin();
    m_playlistUniqueId = albumPlaylistAndNames.m_playlistUniqueId;
    m_playlistName = albumPlaylistAndNames.m_playlistName;

    return true;
}
