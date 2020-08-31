#ifndef SIMPLEMOVIEUI_PLAYER_H
#define SIMPLEMOVIEUI_PLAYER_H

#include <functional>
#include <string>
#include <fstream>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "common/logger.h"
#include "common/generalPlaylist.h"
#include <algorithm>
#include <random>

using PlaylistEndCallback = std::function<void()>;
using SongEndCallback = std::function<void(const boost::uuids::uuid&)>;
using OnUiChangeHandler = std::function<void( const boost::uuids::uuid& songID, const boost::uuids::uuid& playlistID, int position, bool doLoop, bool doShuffle)>;


class BasePlayer {

protected:

    PlaylistEndCallback m_playlistEndCallback;
    SongEndCallback m_songEndCallback;
    OnUiChangeHandler m_onUiChangeHandler;

    std::vector<Common::PlaylistItem> m_playlist;
    std::vector<Common::PlaylistItem> m_playlist_orig;
    boost::uuids::uuid m_playlistUniqueId;
    std::string m_playlistName;

    std::vector<Common::PlaylistItem>::const_iterator m_currentItemIterator { m_playlist.end() };

    bool m_shuffle   { false };
    bool m_doCycle   { false };
    bool m_isPlaying { false };
    bool m_pause     { false };
    uint32_t m_volume { 15 };
    std::default_random_engine m_rng {};

    virtual void stopAndRestart() = 0;

    bool needsOnlyUnpause(const boost::uuids::uuid &playlist);
    bool needsStop();

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
    std::string getPlaylistIDStr() const { return boost::uuids::to_string(m_playlistUniqueId); }
    boost::uuids::uuid getPlaylistID() const { return m_playlistUniqueId; }
    uint32_t getVolume() { return m_volume; }

    void setPlaylistEndCB(PlaylistEndCallback&& endfunc);
    void setSongEndCB(SongEndCallback&& endfunc);

    void resetPlayer() {

        m_shuffle = false;
        m_doCycle = false;
        m_isPlaying = false;
        m_pause = false;

        m_playlist.clear();
        m_playlist_orig.clear();
        m_currentItemIterator = m_playlist.end();
        m_playlistUniqueId = boost::uuids::uuid();
        m_playlistName = "";

    }

    bool setPlaylist(const Common::AlbumPlaylistAndNames& albumPlaylistAndNames) {

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

    bool startPlay() { return startPlay(boost::uuids::uuid()); }
    bool startPlay(const boost::uuids::uuid& songUID) { return startPlay(songUID, 0); }
    virtual bool startPlay(const boost::uuids::uuid& songUID, uint32_t position) = 0;
    virtual bool stop() = 0;
    virtual bool stopPlayerConnection() = 0;
    virtual bool setVolume(uint32_t volume) = 0;

    virtual bool seek_forward() = 0;
    virtual bool seek_backward() = 0;
    virtual bool next_file() = 0;
    virtual bool prev_file() = 0;
    virtual bool pause_toggle() = 0;

    virtual bool jump_to_position(int percent) = 0;
    virtual bool jump_to_fileUID(const boost::uuids::uuid& uniqueId) = 0;

    bool isPlaying() const { return m_isPlaying; }
    bool isPause() const { return m_pause; }
    void resetPause() { m_pause = false; }

    virtual const std::string getSongName() const = 0;
    virtual boost::uuids::uuid getSongID() const = 0;
    std::string getSongIDStr() const { return boost::uuids::to_string(getSongID()); }
    virtual int getSongPercentage() const = 0;
    virtual const std::string getTitle() const = 0;
    virtual const std::string getAlbum() const = 0;
    virtual const std::string getPerformer() const = 0;

    void onUiChange(OnUiChangeHandler&& onUiChangeFunc) { m_onUiChangeHandler = std::move(onUiChangeFunc); }

};


#endif //SIMPLEMOVIEUI_PLAYER_H
