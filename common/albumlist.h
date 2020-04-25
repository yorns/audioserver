#ifndef ALBUMLIST_H
#define ALBUMLIST_H

#include <string>
#include <vector>

namespace Common {

struct AlbumListEntry {
    std::string m_name;
    std::string m_performer;
    std::vector<std::string> m_playlist;
};


}


#endif // ALBUMLIST_H
