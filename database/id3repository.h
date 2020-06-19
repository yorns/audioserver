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

using namespace LoggerFramework;

namespace Database {

enum class AddType {
    Id3,
    json
};

struct CoverElement {
    std::vector<boost::uuids::uuid> uidListForCover;
    std::vector<char> rawData;
    std::size_t hash {0};

    CoverElement() = default;

    bool isConnectedToUid(const boost::uuids::uuid& uid) const {
        return std::find_if(std::cbegin(uidListForCover), std::cend(uidListForCover),
                            [&uid](const boost::uuids::uuid& elem) { return uid == elem; } ) != std::cend(uidListForCover);
    }

    bool hasEqualHash(const std::size_t otherHash) { return hash == otherHash; }

    bool insertNewUid(boost::uuids::uuid&& newUid) { uidListForCover.emplace_back(std::move(newUid)); return true; }
};

class Id3Repository
{
    std::vector<Id3Info> m_simpleDatabase;
    std::vector<CoverElement> m_simpleCoverDatabase;

    id3TagReader m_tagReader;

    const CoverElement emptyElement;

    bool m_cache_dirty { false };
    bool m_enableCache { false };

    std::optional<nlohmann::json> id3ToJson(const std::vector<Id3Info>& id3Db) const;
    const std::vector<Id3Info> id3fromJson(const std::string& file) const ;
    std::optional<nlohmann::json> coverToJson(const std::vector<CoverElement>& coverDb) const;
    const std::vector<CoverElement> coverFromJson(const std::string& filename) const;

    bool writeJson(nlohmann::json&& data, const std::string& filename) const;

    bool add(std::optional<FullId3Information>&& audioItem);

    bool readCache();
    bool writeCacheInternal();

    bool isCached(const std::string& url) {
        return std::find_if(std::cbegin(m_simpleDatabase), std::cend(m_simpleDatabase),
                            [&url](const Id3Info& elem) { return elem.informationSource == url; }) != std::cend(m_simpleDatabase);
    }

public:

    Id3Repository(bool enableCache) : m_enableCache(enableCache) {}

    const CoverElement& getCover(const boost::uuids::uuid& coverUid) const;

    bool add(const Common::FileNameType& file);
    bool addCover(boost::uuids::uuid&& uuid, std::vector<char>&& data, std::size_t hash);
    bool remove(const boost::uuids::uuid& uuid);

    void clear();

    std::vector<std::tuple<Id3Info, std::vector<boost::uuids::uuid>>> findDuplicates();

    std::vector<Id3Info> search(const boost::uuids::uuid &what, SearchItem item,
                                               SearchAction action = SearchAction::exact);

    std::vector<Id3Info> search(const std::string &what, SearchItem item,
                                               SearchAction action = SearchAction::exact);

    std::vector<Common::AlbumListEntry> extractAlbumList();

    std::optional<Id3Info> getId3InfoByUid(const boost::uuids::uuid& uid) const;

    bool read();
    bool writeCache() {
        return writeCacheInternal();
    }

#ifdef WITH_UNITTEST
    bool add(Id3Info&& info) { m_simpleDatabase.emplace_back(info); return true; }
#endif
};

}

#endif // ID3REPOSITORY_H
