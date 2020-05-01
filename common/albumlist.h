#ifndef ALBUMLIST_H
#define ALBUMLIST_H

#include <string>
#include <vector>
#include <tuple>

namespace Common {

struct AlbumListEntry {
    std::string m_name;
    std::string m_performer;
    std::string m_coverExtension;
    std::string m_coverId;
    std::vector<std::tuple<std::string, uint32_t>> m_playlist;
};


}


#endif // ALBUMLIST_H
