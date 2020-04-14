#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <cctype>

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

class Playlist {
    std::string m_uniqueID;
    std::string m_humanReadableName;
    std::vector<std::string> m_playlist;
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
    void setUniqueID(const std::string& uniqueID) { m_uniqueID = uniqueID; setChanged(Changed::isChanged); }

    void setName(const std::string& name) { m_humanReadableName = name; setChanged(Changed::isChanged); }
    std::string getName() const;

    const std::vector<std::string>& getUniqueIdPlaylist() const;
    bool addToList(std::string&& audioUID);
    bool delFromList(const std::string& audioUID);

    bool readM3u();
    bool readJson();
    bool write();

};

}
#endif // PLAYLIST_H
