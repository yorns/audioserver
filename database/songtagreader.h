#ifndef SONGTAGREADER_H
#define SONGTAGREADER_H

#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

#include "id3tagreader/idtag.h"
#include "common/filesystemadditions.h"

namespace Database {

class SongTagReader {

    struct TagItem {
        std::string fullSearchName;
        std::vector<Tag> tags;
        TagItem(std::string&& searchName, Tag&& tag)
            : fullSearchName(searchName) { tags.emplace_back(tag); }
    };

    typedef std::vector<TagItem> TagList;
    TagList m_tagList;

    void addTagId(std::string&& identifier, Tag&& tag);

    TagList::iterator find(const std::string& searchName);


public:
    void readSongTagFile();

    std::vector<Tag> findSongTagList(const std::string& albumName,
                                     const std::string& titleName,
                                     const std::string& performerName) const;

    std::string convert (Tag tag) const {
        return TagConverter::getTagName(tag);
    }

    std::string convert(std::vector<Tag> tagList) {
        std::stringstream tagNameList;
        for (auto elem : tagList) {
            tagNameList << convert((elem)) << " ";
        }
        return tagNameList.str();
    }
};

}

#endif // SONGTAGREADER_H
