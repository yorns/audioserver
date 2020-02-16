#ifndef SIMPLEMOVIEUI_PLAYER_H
#define SIMPLEMOVIEUI_PLAYER_H

#include <functional>
#include <string>
#include <fstream>
#include <vector>
#include "common/logger.h"

typedef std::function<void()> PlaylistEndCallback;
typedef std::function<void(const std::string&)> SongEndCallback;

class BasePlayer {

protected:

    PlaylistEndCallback m_playlistEndCallback;
    SongEndCallback m_songEndCallback;

public:

    BasePlayer() = default;
    BasePlayer(const BasePlayer& ) = default;
    BasePlayer& operator=(const BasePlayer&) = default;

    virtual ~BasePlayer() = default;

    virtual bool startPlay(const std::string &url) = 0;
    virtual bool stop() = 0;
    virtual bool seek_forward() = 0;
    virtual bool seek_backward() = 0;
    virtual bool next_file() = 0;
    virtual bool prev_file() = 0;
    virtual bool pause_toggle() = 0;

    virtual bool isPlaying() = 0;

    virtual std::string getSong() = 0;
    virtual std::string getSongID() = 0;
    virtual uint32_t getSongPercentage() = 0;

    virtual void selectPlaylistsEntry(uint32_t id) = 0;

    void setPlayerEndCallBack(PlaylistEndCallback&& endfunc);
    void setSongEndCB(SongEndCallback&& endfunc);

    virtual bool stopPlayerConnection() = 0;
};


#endif //SIMPLEMOVIEUI_PLAYER_H
