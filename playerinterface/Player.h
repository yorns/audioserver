#ifndef SIMPLEMOVIEUI_PLAYER_H
#define SIMPLEMOVIEUI_PLAYER_H

#include <functional>
#include <string>
#include <fstream>
#include <vector>
#include "common/logger.h"
#include "common/generalPlaylist.h"
#include <algorithm>
#include <random>

using PlaylistEndCallback = std::function<void()>;
using SongEndCallback = std::function<void(const std::string&)>;
using OnUiChangeHandler = std::function<void( const std::string& songID, const std::string& playlistID, int position, bool doLoop, bool doShuffle)>;


class BasePlayer {

protected:

    PlaylistEndCallback m_playlistEndCallback;
    SongEndCallback m_songEndCallback;
    OnUiChangeHandler m_onUiChangeHandler;

    std::vector<Common::PlaylistItem> m_playlist;
    std::vector<Common::PlaylistItem> m_playlist_orig;
    std::string m_PlaylistUniqueId;
    std::string m_PlaylistName;

    std::vector<Common::PlaylistItem>::const_iterator m_currentItemIterator { m_playlist.end() };

    bool m_shuffle   { false };
    bool m_doCycle   { false };
    bool m_isPlaying { false };
    bool m_pause     { false };

    std::default_random_engine m_rng {};

    virtual void stopAndRestart() = 0;

    bool needsOnlyUnpause(const std::string &playlist);
    bool needsStop();
    bool isPause() const { return m_pause; }

    bool calculatePreviousFileInList();
    bool calculateNextFileInList();

    void updateUi() const;

public:

    BasePlayer() = default;
    BasePlayer(const BasePlayer& ) = default;
    BasePlayer& operator=(const BasePlayer&) = default;

    virtual ~BasePlayer() = default;

    bool doShuffle(bool shuffle);
    bool toggleShuffle();
    bool toogleLoop();
    bool getLoop() const { return m_doCycle; }
    bool getShuffle() const { return m_shuffle; }
    std::string getPlaylistID() const { return m_PlaylistUniqueId; }

    void setPlaylistEndCB(PlaylistEndCallback&& endfunc);
    void setSongEndCB(SongEndCallback&& endfunc);

    virtual bool startPlay(const Common::AlbumPlaylistAndNames& albumPlaylistAndNames, const std::string& songUID) = 0;
    virtual bool stop() = 0;
    virtual bool stopPlayerConnection() = 0;

    virtual bool seek_forward() = 0;
    virtual bool seek_backward() = 0;
    virtual bool next_file() = 0;
    virtual bool prev_file() = 0;
    virtual bool pause_toggle() = 0;

    virtual bool jump_to_position(int percent) = 0;
    virtual bool jump_to_fileUID(const std::string& uniqueId) = 0;

    bool isPlaying() const { return m_isPlaying; }

    virtual const std::string getSongName() const = 0;
    virtual std::string getSongID() const = 0;
    virtual int getSongPercentage() const = 0;

    void onUiChange(OnUiChangeHandler&& onUiChangeFunc) { m_onUiChangeHandler = std::move(onUiChangeFunc); }

};


#endif //SIMPLEMOVIEUI_PLAYER_H
