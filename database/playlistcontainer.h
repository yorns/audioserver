#ifndef PLAYLISTCONTAINER_H
#define PLAYLISTCONTAINER_H

#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "common/logger.h"
#include "common/Constants.h"
#include "common/albumlist.h"
#include "playlist.h"
#include "NameType.h"
#include "searchaction.h"

namespace Database {

class PlaylistContainer {

    std::vector<Playlist> m_playlists;
    std::optional<boost::uuids::uuid> m_currentPlaylist;

public:

    void addPlaylist(Playlist&& playlist);
    bool addItemToPlaylistName(const std::string& playlistName, boost::uuids::uuid&& audioUniqueId);
    bool addItemToPlaylistUID(const boost::uuids::uuid& playlistUniqueID, boost::uuids::uuid&& audioUniqueId);

    bool removePlaylistUID(const boost::uuids::uuid& playlistUniqueId);
    bool removePlaylistName(const std::string& playlistName);

    bool readPlaylistsM3U();
    bool readPlaylistsJson(Database::FindAlgo&& findUuidList, Database::InsertCover&& coverInsert);
    bool insertAlbumPlaylists(const std::vector<Common::AlbumListEntry>& albumList);

    std::optional<std::string> createvirtual_m3u(const boost::uuids::uuid& playlistUuid) const;

    bool writeChangedPlaylists();

    std::optional<std::string> convertName(const boost::uuids::uuid& name);
    std::optional<boost::uuids::uuid> convertName(const std::string& name);

    std::optional<const std::vector<boost::uuids::uuid>> getPlaylistByName(const std::string& playlistName) const;
    std::optional<const std::vector<boost::uuids::uuid>> getPlaylistByUID(const boost::uuids::uuid& uid) const;

    std::optional<const Playlist> getCurrentPlaylist() const;
    std::optional<const boost::uuids::uuid> getCurrentPlaylistUniqueID() const;

    bool setCurrentPlaylist(boost::uuids::uuid&& currentPlaylistUniqueId);
    void unsetCurrentPlaylist();

    bool isUniqueName(const std::string& name);

    bool insertAlbumPlaylist();

    std::vector<std::pair<std::string, boost::uuids::uuid>> getAllPlaylists();

    std::vector<Playlist> searchPlaylists(const std::string& what, SearchAction action = SearchAction::exact);
    std::vector<Playlist> searchPlaylists(const boost::uuids::uuid& what, SearchAction action = SearchAction::exact);


};


}

#endif // PLAYLISTCONTAINER_H
