#ifndef SIMPLEMOVIEUI_PLAYER_H
#define SIMPLEMOVIEUI_PLAYER_H

#include <functional>
#include <string>
#include <fstream>
#include <vector>
#include "common/logger.h"
#include <algorithm>
#include <random>

typedef std::function<void()> PlaylistEndCallback;
typedef std::function<void(const std::string&)> SongEndCallback;

class BasePlayer {

protected:

    PlaylistEndCallback m_playlistEndCallback;
    SongEndCallback m_songEndCallback;

    std::vector<std::string> m_playlist;
    std::vector<std::string> m_playlist_orig;
    std::string m_PlaylistUniqueId;
    std::string m_PlaylistName;

    bool m_shuffle { false };
    bool m_doCycle { false };

    std::default_random_engine m_rng {};

public:

    BasePlayer() = default;
    BasePlayer(const BasePlayer& ) = default;
    BasePlayer& operator=(const BasePlayer&) = default;

    virtual ~BasePlayer() = default;

    bool do_shuffle(bool doShuffle);
    bool do_cycle(bool doCycle) { m_doCycle = doCycle; return true; }

    void setPlayerEndCallBack(PlaylistEndCallback&& endfunc);
    void setSongEndCB(SongEndCallback&& endfunc);

    virtual bool startPlay(const std::vector<std::string>& list, const std::string& playlistUniqueId, const std::string& playlistName) = 0;
    virtual bool stop() = 0;
    virtual bool stopPlayerConnection() = 0;

    virtual bool seek_forward() = 0;
    virtual bool seek_backward() = 0;
    virtual bool next_file() = 0;
    virtual bool prev_file() = 0;
    virtual bool pause_toggle() = 0;

    virtual bool jump_to_position(int percent) = 0;
    virtual bool jump_to_fileUID(std::string& uniqueId) = 0;

    virtual bool isPlaying() = 0;

    virtual const std::string getSongName() const = 0;
    virtual std::string getSongID() = 0;
    virtual uint32_t getSongPercentage() = 0;

};


#endif //SIMPLEMOVIEUI_PLAYER_H
