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
#include "common/logger.h"
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/lexical_cast.hpp>
#include "searchitem.h"
#include "id3tagreader/idtag.h"

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
    PlaylistItem(boost::uuids::uuid&& uniqueId) : m_uniqueId (std::move(uniqueId)) {}
    PlaylistItem() {}
    boost::uuids::uuid m_uniqueId;
    std::string m_name;
    std::string m_name_lower;
    std::string m_performer;
    std::string m_performer_lower;
    std::vector<Tag> m_tagList;

};

typedef std::function<std::vector<boost::uuids::uuid>(const std::string& what, SearchItem searchItem)> FindAlgo;
typedef std::function<void(boost::uuids::uuid&& uid, std::vector<char>&& data, std::size_t hash)> InsertCover;

class Playlist {
    std::string m_playlistFileName;
    PlaylistItem m_item;
    std::vector<boost::uuids::uuid> m_playlist;
    std::string m_coverName;
    CoverType m_coverType { CoverType::none };
    Changed m_changed { Changed::isUnchanged };
    Persistent m_persistent { Persistent::isPermanent };
    ReadType m_readType { ReadType::isM3u };

    bool findInTagList(Tag tagElement) {
        return std::find_if(std::begin(m_item.m_tagList),
                         std::end(m_item.m_tagList),
                         [&tagElement](const Tag& tag){ return tag == tagElement; })
                != std::end(m_item.m_tagList);
    }
public:

    Playlist() = delete;

    Playlist(std::string filename, ReadType readType = ReadType::isM3u,
             Persistent persistent = Persistent::isPermanent,
             Changed changed = Changed::isUnchanged);

    Playlist(boost::uuids::uuid&& uniqueID, std::vector<boost::uuids::uuid>&& playlist,
             ReadType readType = ReadType::isM3u,
             Persistent persistent = Persistent::isPermanent,
             Changed changed = Changed::isChanged);

    Changed changed() const;
    void setChanged(const Changed &changed);

    Persistent persistent() const;
    void setPersistent(const Persistent &persistent);

    boost::uuids::uuid getUniqueID() const;
    void setUniqueID(boost::uuids::uuid&& uniqueID) { m_item.m_uniqueId = std::move(uniqueID); setChanged(Changed::isChanged); }

    void setName(const std::string& name);
    void setPerformer(const std::string& performer);
    void setTagList(std::vector<Tag> tagList) {
        for (const auto& elem : tagList) {
            if (!findInTagList(elem)) {
                m_item.m_tagList.push_back(elem);
            }
        }
    }

    bool isTagAlike(const std::vector<std::string>& whatList ) const {
        if (whatList.size() > 0) {
            for (const auto& elem : whatList) {
                for (const auto& tag : m_item.m_tagList) {
                    if (TagConverter::getTagIdAlike(elem) == tag) {
                        return true;
                    }
                }

            }
        }
        return false;
    }

    std::string strTag() const {
        std::stringstream retStr;
        for(const auto& elem : m_item.m_tagList) {
            retStr << TagConverter::getTagName(elem);
        }
        return retStr.str();
    }

    std::string getName() const;
    std::string getPerformer() const { return m_item.m_performer; }

    std::string getNameLower() const { return m_item.m_name_lower; }
    std::string getPerformerLower() const { return m_item.m_performer_lower; }

    const std::vector<boost::uuids::uuid>& getUniqueIdPlaylist() const;
    bool addToList(boost::uuids::uuid&& audioUID);
    bool delFromList(const boost::uuids::uuid& audioUID);

    bool readM3u();
    bool readJson(FindAlgo&& findAlgo, InsertCover&& insertCover);
    bool insertAlbumList();
    bool write();

    std::string getCover() const;

    bool setCover(const boost::uuids::uuid& coverId, const std::string& extension) {
        try {
        m_coverName = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::CoversRelative) + "/"
                + boost::lexical_cast<std::string>(coverId) + extension;
        } catch (std::exception& ex) {
            logger(LoggerFramework::Level::warning) << ex.what() << "\n";
            return false;
        }

        return true;
    }

};

}
#endif // PLAYLIST_H
