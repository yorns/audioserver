#ifndef PLAYLISTCONTAINER_H
#define PLAYLISTCONTAINER_H

#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <cctype>
#include "common/logger.h"
#include "common/Constants.h"
#include "playlist.h"
#include "NameType.h"

namespace Database {

class PlaylistContainer {
    std::vector<Playlist> m_playlists;
    std::optional<std::string> m_currentPlaylist;

public:
    PlaylistContainer() = default;

    void addPlaylist(Playlist&& playlist);
    bool addItemToPlaylistName(const std::string& playlistName, std::string&& audioUniqueId);
    bool addItemToPlaylistUID(const std::string& playlistUniqueID, std::string&& audioUniqueId);

    bool removePlaylistUID(const std::string& playlistUniqueId);
    bool removePlaylistName(const std::string& playlistName);

    bool readPlaylists();
    bool writeChangedPlaylists();

    std::optional<std::string> convertName(const std::string& name, NameType nametype);

    std::optional<const std::vector<std::string>> getPlaylistByName(const std::string& playlistName) const;
    std::optional<const std::vector<std::string>> getPlaylistByUID(const std::string& uid) const;

    std::optional<Playlist> getCurrentPlaylist();
    std::optional<const std::string> getCurrentPlaylistUniqueID() const;

    bool setCurrentPlaylist(const std::string &currentPlaylistUniqueId);
    void unsetCurrentPlaylist();

    bool isUniqueName(const std::string& name);

    std::vector<std::pair<std::string, std::string>> getAllPlaylists();

};


}

#endif // PLAYLISTCONTAINER_H
