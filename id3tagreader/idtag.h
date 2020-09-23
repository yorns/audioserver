#ifndef IDTAG_H
#define IDTAG_H

#include <vector>
#include <string>
#include <sstream>
#include "common/logger.h"

enum class Tag {
    Children,
    German,
    Rock,
    Pop,
    Jazz,
    Playlist,
    Stream,
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
    static Tag getTagId(const std::string& tagName) {
        if (!tagName.empty()) {
            for (const auto& id : m_tagIdentifier) {
                for (const auto& tagIdentifier : id.identifier) {
                    logger(LoggerFramework::Level::debug) << "test input <"<<tagName<<"> against <"<<tagIdentifier<<"("<< tagIdentifier.substr(0, tagName.length()) <<")\n";
                    if (tagIdentifier.substr(0, tagName.length()) == tagName)
                        return id.tag;
                }
            }
        }
        return Tag::unknown;
    }

    static Tag getTagIdAlike(const std::string& tagName) {
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
