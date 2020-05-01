#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <cctype>
#include <string>
#include "common/Constants.h"
#include "common/filesystemadditions.h"
#include "common/stringmanipulator.h"
#include "boost/filesystem.hpp"

namespace filesys =  boost::filesystem;

namespace Database {

enum class Changed {
    isChanged,
    isUnchanged,
    isConst
};

enum class Persistent {
    isTemporal,
    isPermanent
};

enum class ReadType {
    isM3u,
    isJson
};

enum class CoverType {
    onPlaylistId,
    onPlaylistItem,
    none
};

struct PlaylistItem {
    PlaylistItem(const std::string& uniqueId) : m_uniqueId (uniqueId) {}
    std::string m_uniqueId;
    std::string m_name;
    std::string m_name_lower;
    std::string m_performer;
};

class Playlist {
    PlaylistItem m_item;
    std::vector<std::string> m_playlist;
    std::string m_coverName;
    CoverType m_coverType { CoverType::none };
    Changed m_changed { Changed::isUnchanged };
    Persistent m_persistent { Persistent::isPermanent };
    ReadType m_readType { ReadType::isM3u };

public:

    Playlist() = delete;

    Playlist(const std::string& uniqueID,
             ReadType readType = ReadType::isM3u,
             Persistent persistent = Persistent::isPermanent,
             Changed changed = Changed::isUnchanged);

    Playlist(const std::string& uniqueID, std::vector<std::string>&& playlist,
             ReadType readType = ReadType::isM3u,
             Persistent persistent = Persistent::isPermanent,
             Changed changed = Changed::isChanged);

    Changed changed() const;
    void setChanged(const Changed &changed);

    Persistent persistent() const;
    void setPersistent(const Persistent &persistent);

    std::string getUniqueID() const;
    void setUniqueID(const std::string& uniqueID) { m_item.m_uniqueId = uniqueID; setChanged(Changed::isChanged); }

    void setName(const std::string& name);
    void setPerformer(const std::string& performer) { m_item.m_performer = performer; }

    std::string getName() const;
    std::string getPerformer() const { return m_item.m_performer; }

    const std::vector<std::string>& getUniqueIdPlaylist() const;
    bool addToList(std::string&& audioUID);
    bool delFromList(const std::string& audioUID);

    bool readM3u();
    bool readJson();
    bool insertAlbumList();
    bool write();

    std::string getCover() const;

    bool setCover(const std::string& coverId, const std::string& extension) {
        m_coverName = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::CoversRelative) + "/" +coverId+extension;
        return true;
    }

};

}
#endif // PLAYLIST_H
