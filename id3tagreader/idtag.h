#ifndef IDTAG_H
#define IDTAG_H

#include <vector>
#include <string>
#include <sstream>
#include "common/logger.h"
#include "common/stringmanipulator.h"

enum class Tag {
    Children,
    German,
    Rock,
    Pop,
    Jazz,
    Playlist,
    Stream,
    AudioBooks,
    unknown
};

class TagConverter {
public:
    struct TagIdentifierElem {
        std::vector<std::string> identifier;
        Tag tag;
    };

private:
    typedef std::vector<TagIdentifierElem> TagIdentifier;

    static TagIdentifier m_tagIdentifier;

public:
    static Tag getTagId(const std::string& _tagName) {
        if (!_tagName.empty()) {
            std::string tagName {Common::str_tolower(_tagName)};
            for (const auto& id : m_tagIdentifier) {
                for (const auto& tagIdentifier : id.identifier) {
                    if (tagIdentifier.substr(0, tagName.length()) == tagName)
                        return id.tag;
                }
            }
        }
        return Tag::unknown;
    }

    static Tag getTagIdAlike(const std::string& _tagName) {
        std::string tagName {Common::str_tolower(_tagName)};
        for (const auto& id : m_tagIdentifier) {
            for (const auto& tagIdentifier : id.identifier) {
                if (tagIdentifier.substr(0,tagName.length()) == tagName)
                    return id.tag;
            }
        }
        return Tag::unknown;
    }

    static std::string getTagName(const std::vector<Tag>& tagList) {
        std::stringstream allString;
        for (const auto& id : tagList) {
            allString << getTagName(id) << ", ";
        }
        return allString.str();
    }

    static std::string getTagName(const Tag& tagId) {
        for (const auto& id : m_tagIdentifier) {
            if (tagId == id.tag)
                return id.identifier.front();
        }
        return "";
    }

};

#endif // IDTAG_H
