#ifndef ALBUMLIST_H
#define ALBUMLIST_H

#include <string>
#include <vector>
#include <tuple>
#include <boost/uuid/uuid.hpp>

namespace Common {

struct AlbumListEntry {
    std::string m_name;
    std::string m_performer;
    std::string m_coverExtension;
    boost::uuids::uuid m_coverId;
    std::vector<std::tuple<boost::uuids::uuid, uint32_t>> m_playlist;
};


}


#endif // ALBUMLIST_H
