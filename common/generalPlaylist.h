#ifndef GENERALPLAYLIST_H
#define GENERALPLAYLIST_H
#include <string>
#include <vector>

namespace Common {

struct PlaylistItem {
      std::string m_uniqueId;
      std::string m_url;
};

struct AlbumPlaylistAndNames {
    std::vector<PlaylistItem> m_playlist;
    std::string m_playlistName;
    std::string m_playlistUniqueId;
};

}

#endif // GENERALPLAYLIST_H
