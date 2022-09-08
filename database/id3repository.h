#ifndef ID3REPOSITORY_H
#define ID3REPOSITORY_H

#include <vector>
#include <algorithm>
#include <boost/uuid/uuid.hpp>

#include "searchitem.h"
#include "searchaction.h"
#include "id3tagreader/Id3Info.h"
#include "id3tagreader/id3TagReader.h"
#include "common/logger.h"
#include "common/albumlist.h"
#include "common/base64.h"
#include "common/filesystemadditions.h"
#include "nlohmann/json.hpp"
#include "songtagreader.h"
#include "common/hash.h"

using namespace LoggerFramework;

namespace Database {


enum class AddType {
    Id3,
    json
};

struct CoverElement {
    std::vector<boost::uuids::uuid> uidListForCover; //< list of uuid of songs with the same cover (identified by hash)
    std::vector<char> rawData; //< raw image binary
    std::size_t hash {0}; //< hash value for the cover image

    CoverElement() = default;

    bool isConnectedToUid(const boost::uuids::uuid& uid) const;

    bool hasEqualHash(const std::size_t otherHash) { return hash == otherHash; }

    bool insertNewUid(boost::uuids::uuid&& newUid) { uidListForCover.emplace_back(std::move(newUid)); return true; }

};

const std::vector<CoverElement> coverFromJson(const std::string &filename);
std::optional<nlohmann::json> coverToJson(const std::vector<CoverElement> &coverDb);
bool writeJson(nlohmann::json &&data, const std::string &filename);

class CoverDatabase {

    std::vector<CoverElement> m_simpleCoverDatabase;

public:
    bool addCover(std::vector<char>&& rawData, const boost::uuids::uuid& _uid);

    std::optional<std::reference_wrapper<const CoverElement>> getCover(const boost::uuids::uuid& uid) const {

        const auto it = std::find_if(std::cbegin(m_simpleCoverDatabase), std::cend(m_simpleCoverDatabase),
                            [&uid](const CoverElement& elem) { return elem.isConnectedToUid(uid); } );

        if (it != std::cend(m_simpleCoverDatabase)) {
            return *it;
        }

        return std::nullopt;
    }

    bool readCache(const std::string& coverCacheFile);

    bool writeCache(const std::string& coverCacheFile);

};


class Id3Repository
{
    std::vector<Id3Info> m_simpleDatabase;
    CoverDatabase m_simpleCoverDatabase;

    id3TagReader m_tagReader;

    const CoverElement emptyElement;

    bool m_cache_dirty { false };
    bool m_enableCache { false };

    std::optional<nlohmann::json> id3ToJson(const std::vector<Id3Info>::const_iterator& begin, const std::vector<Id3Info>::const_iterator& end) const;
    std::optional<nlohmann::json> id3ToJson(const std::vector<Id3Info>& id3Db) const;
    const std::vector<Id3Info> id3fromJson(const std::string& file) const ;
//    std::optional<nlohmann::json> coverToJson(const std::vector<CoverElement>& coverDb) const;
//    const std::vector<CoverElement> coverFromJson(const std::string& filename) const;

//    bool writeJson(nlohmann::json&& data, const std::string& filename) const;

    // add a new entry to the audio repository with all information
    std::optional<boost::uuids::uuid> add(std::optional<FullId3Information>&& audioItem);

    bool readCache();
    bool writeCacheInternal();

    bool isCached(const std::string& url) const;

    std::vector<Id3Info> upn(std::vector<std::string>& whatlist, SearchItem item);

public:

    Id3Repository(bool enableCache) : m_enableCache(enableCache) {}

    const CoverElement& getCover(const boost::uuids::uuid& coverUid) const;

    // entry point to add a new file
    std::optional<boost::uuids::uuid> add(const Common::FileNameType& file);
    bool addCover(boost::uuids::uuid&& uuid, std::vector<char>&& data);
    bool remove(const boost::uuids::uuid& uuid);

    void addTags(const SongTagReader& songTagReader);

    void clear();

    std::vector<std::tuple<Id3Info, std::vector<boost::uuids::uuid>>> findDuplicates();

    std::vector<Id3Info> search(const boost::uuids::uuid &what, SearchItem item,
                                               SearchAction action = SearchAction::exact);

    std::vector<Id3Info> search(const std::string &what, SearchItem item,
                                               SearchAction action = SearchAction::exact);

    std::vector<Common::AlbumListEntry> extractAlbumList();

    std::optional<Id3Info> getId3InfoByUid(const boost::uuids::uuid& uid) const;

    bool utf8_check_is_valid(const std::string& string) const;

    bool read();
    bool writeCache();

#ifdef WITH_UNITTEST
    bool add(Id3Info&& info) { m_simpleDatabase.emplace_back(info); return true; }
#endif
};

}

#endif // ID3REPOSITORY_H
