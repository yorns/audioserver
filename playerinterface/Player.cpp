#include "Player.h"
#include <regex>

void BasePlayer::setPlayerEndCallBack(PlaylistEndCallback&& endfunc) {
    m_playlistEndCallback = std::move(endfunc);
}

void BasePlayer::setSongEndCB(SongEndCallback&& endfunc) {
    m_songEndCallback = std::move(endfunc);
}
