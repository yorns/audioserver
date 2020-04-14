#ifndef ID3REPOSITORY_H
#define ID3REPOSITORY_H

#include <vector>
#include <algorithm>
#include "searchitem.h"
#include "searchaction.h"
#include "id3tagreader/Id3Info.h"
#include "id3tagreader/id3TagReader.h"
#include "common/logger.h"

using namespace LoggerFramework;

namespace Database {

class Id3Repository
{
    std::vector<Id3Info> m_simpleDatabase;
    id3TagReader m_tagReader;

    std::vector<Id3Info> findAlbum(const std::string &what);

public:

    bool add(const std::string& uniqueID);
    bool remove(const std::string& uniqueID);

    void clear() {
        m_simpleDatabase.clear();
    }

    std::vector<std::tuple<Id3Info, std::vector<std::string>>> findDuplicates();

    std::optional<std::vector<Id3Info>> search(const std::string &what, SearchItem item,
                                               SearchAction action = SearchAction::exact);

    std::optional<Id3Info> getId3InfoByUid(const std::string& uniqueId) const;

    bool read();

#ifdef WITH_UNITTEST
    bool add(Id3Info&& info) { m_simpleDatabase.emplace_back(info); return true; }
#endif
};

}

#endif // ID3REPOSITORY_H
