#include "Player.h"
#include <regex>

using namespace LoggerFramework;

bool BasePlayer::doShuffle(bool shuffleRequest) {
    if (shuffleRequest) {
        if (m_shuffle)
            logger(Level::debug) << "Shuffeling playlist <"<<m_PlaylistUniqueId<<"> (" << m_PlaylistName<<") again\n";
        else {
            logger(Level::debug) << "Shuffeling playlist <"<<m_PlaylistUniqueId<<"> (" << m_PlaylistName<<")\n";
            m_playlist.clear();
            m_playlist.insert(std::begin(m_playlist), std::cbegin(m_playlist_orig), std::cend(m_playlist_orig));
        }
        m_shuffle = true;
        std::shuffle(std::begin(m_playlist), std::end(m_playlist), m_rng);

        logger(LoggerFramework::Level::debug) << "Nr) /t original \t->  shuffled\n";
        for(uint32_t i{0}; i < m_playlist.size(); ++i) {
            logger(LoggerFramework::Level::debug) << i << ") /t" << m_playlist_orig[i] <<" \t-> " << m_playlist[i] << "\n";
        }

        return true;
    }
    else if (!shuffleRequest && m_shuffle) {
        logger(Level::debug) << "Reset Shuffling for playlist <"<<m_PlaylistUniqueId<<"> (" << m_PlaylistName<<")\n";
        m_shuffle = false;
        m_playlist.clear();
        m_playlist.insert(std::begin(m_playlist), std::cbegin(m_playlist_orig), std::cend(m_playlist_orig));
        return true;
    }
    logger(Level::warning) << "Shuffling request for playlist <"<<m_PlaylistUniqueId<<"> (" << m_PlaylistName<<") failed (request: "
                           << (shuffleRequest?"doShuffle":"doUnshuffle") << " and state: "<< (m_shuffle?"shuffel":"normal") << "\n";
    return false;
}

bool BasePlayer::toggleShuffle() {
    return doShuffle(!m_shuffle);
}

bool BasePlayer::toogleLoop() {
    m_doCycle = !m_doCycle;
    return true;
}


void BasePlayer::setPlaylistEndCB(PlaylistEndCallback&& endfunc) {
    m_playlistEndCallback = std::move(endfunc);
}

void BasePlayer::setSongEndCB(SongEndCallback&& endfunc) {
    m_songEndCallback = std::move(endfunc);
}
