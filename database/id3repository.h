#ifndef ID3REPOSITORY_H
#define ID3REPOSITORY_H

#include <vector>
#include <algorithm>
#include "searchitem.h"
#include "searchaction.h"
#include "id3tagreader/Id3Info.h"
#include "id3tagreader/id3TagReader.h"
#include "common/logger.h"
#include "common/albumlist.h"

using namespace LoggerFramework;

namespace Database {

enum class AddType {
    Id3,
    json
};

struct CoverElement {
    std::vector<std::string> uidListForCover;
    std::vector<char> rawData;
    std::size_t hash {0};

    CoverElement() = default;

    bool isConnectedToUid(const std::string& uid) const {
        return std::find_if(std::cbegin(uidListForCover), std::cend(uidListForCover),
                            [&uid](const std::string& elem) { return uid == elem; } ) != std::cend(uidListForCover);
    }

    bool hasEqualHash(const std::size_t otherHash) { return hash == otherHash; }

    bool insertNewUid(std::string&& newUid) { uidListForCover.emplace_back(std::move(newUid)); return true; }
};

class Id3Repository
{
    std::vector<Id3Info> m_simpleDatabase;
    std::vector<CoverElement> m_simpleCoverDatabase;

    id3TagReader m_tagReader;

    const CoverElement emptyElement;

    bool add(std::optional<FullId3Information>&& audioItem);

public:

    bool addCover(std::string uid, std::vector<char>&& data, std::size_t hash);

    bool add(const std::string& uid) {
        return add(m_tagReader.readJsonAudioInfo(uid));
    }
    bool remove(const std::string& uniqueID);

    void clear() {
        m_simpleDatabase.clear();
    }

    std::vector<std::tuple<Id3Info, std::vector<std::string>>> findDuplicates();

    std::vector<Id3Info> search(const std::string &what, SearchItem item,
                                               SearchAction action = SearchAction::exact);

    std::vector<Common::AlbumListEntry> extractAlbumList();

    std::optional<Id3Info> getId3InfoByUid(const std::string& uniqueId) const;

    bool read();

    const CoverElement& getCover(const std::string& coverUid) const {
        auto it = std::find_if(std::begin(m_simpleCoverDatabase),
                               std::end(m_simpleCoverDatabase),
                               [&coverUid](const CoverElement& elem){
            return elem.isConnectedToUid(coverUid);
        });
        if (it != std::end(m_simpleCoverDatabase)) {
            return *it;
        }
        return emptyElement;
    }

#ifdef WITH_UNITTEST
    bool add(Id3Info&& info) { m_simpleDatabase.emplace_back(info); return true; }
#endif
};

}

#endif // ID3REPOSITORY_H
