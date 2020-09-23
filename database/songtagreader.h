#ifndef SONGTAGREADER_H
#define SONGTAGREADER_H

#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

#include "id3tagreader/idtag.h"
#include "common/filesystemadditions.h"

class SongTagReader {

    struct TagItem {
        std::string fullSearchName;
        std::vector<Tag> tags;
        TagItem(std::string&& searchName, Tag&& tag)
            : fullSearchName(searchName) { tags.emplace_back(tag); }
    };

    typedef std::vector<TagItem> TagList;
    TagList m_tagList;

    void addTagId(std::string&& identifier, Tag&& tag) {
        m_tagList.emplace_back(TagItem(std::move(identifier), std::move(tag)));
    }

    TagList::iterator find(const std::string& searchName) {
        auto it = std::find_if(std::begin(m_tagList),
                             std::end(m_tagList),
                             [&searchName](const TagItem& item){ return item.fullSearchName == searchName; }
        );
        return it;
    }


public:
    void readSongTagFile();

    std::vector<Tag> findSongTagList(const std::string& albumName, const std::string& titleName, const std::string& performerName) const;

    std::string convert (Tag tag) {
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

#endif // SONGTAGREADER_H
