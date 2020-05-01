#ifndef PLAYLISTCONTAINER_H
#define PLAYLISTCONTAINER_H

#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <cctype>
#include "common/logger.h"
#include "common/Constants.h"
#include "common/albumlist.h"
#include "playlist.h"
#include "NameType.h"
#include "searchaction.h"

namespace Database {

class PlaylistContainer {

    std::vector<Playlist> m_playlists;
    std::optional<std::string> m_currentPlaylist;

public:

    void addPlaylist(Playlist&& playlist);
    bool addItemToPlaylistName(const std::string& playlistName, std::string&& audioUniqueId);
    bool addItemToPlaylistUID(const std::string& playlistUniqueID, std::string&& audioUniqueId);

    bool removePlaylistUID(const std::string& playlistUniqueId);
    bool removePlaylistName(const std::string& playlistName);

    bool readPlaylistsM3U();
    bool readPlaylistsJson();
    bool insertAlbumPlaylists(const std::vector<Common::AlbumListEntry>& albumList);

    bool writeChangedPlaylists();

    std::optional<std::string> convertName(const std::string& name, NameType nametype);

    std::optional<const std::vector<std::string>> getPlaylistByName(const std::string& playlistName) const;
    std::optional<const std::vector<std::string>> getPlaylistByUID(const std::string& uid) const;

    std::optional<Playlist> getCurrentPlaylist();
    std::optional<const std::string> getCurrentPlaylistUniqueID() const;

    bool setCurrentPlaylist(const std::string &currentPlaylistUniqueId);
    void unsetCurrentPlaylist();

    bool isUniqueName(const std::string& name);

    bool insertAlbumPlaylist();

    std::vector<std::pair<std::string, std::string>> getAllPlaylists();

    std::vector<Playlist> searchPlaylists(const std::string& what, SearchAction action = SearchAction::exact);


};


}

#endif // PLAYLISTCONTAINER_H
