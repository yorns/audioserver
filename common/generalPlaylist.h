#ifndef GENERALPLAYLIST_H
#define GENERALPLAYLIST_H
#include <string>
#include <vector>
#include <boost/uuid/uuid.hpp>

namespace Common {

struct PlaylistItem {
      boost::uuids::uuid m_uniqueId;
      std::string m_url;
};

struct AlbumPlaylistAndNames {
    std::vector<PlaylistItem> m_playlist;
    std::string m_playlistName;
    boost::uuids::uuid m_playlistUniqueId;
};

}

#endif // GENERALPLAYLIST_H
