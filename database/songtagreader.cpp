#include "songtagreader.h"
#include "common/logger.h"
#include <boost/algorithm/string/trim.hpp>

using namespace LoggerFramework;

void SongTagReader::readSongTagFile() {
    std::fstream inStream;
    auto tagFile = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Tag)+ "/tag.data" ;
    inStream.open(tagFile, std::ios_base::in);

    if (!inStream.good()) {
        logger(Level::warning) << "cannot open tag file\n";
        return;
    }

    for (std::string line; std::getline(inStream, line); ) {

        std::string identifier = line.substr(0, line.find('|'));
        std::string tagName = line.substr(line.find('|')+1);
        boost::trim(identifier);
        boost::trim(tagName);

        logger(Level::debug) << "reading in <"<<identifier<<"> with tag <"<<tagName<<">\n";

        auto tag = TagConverter::getTagId(tagName);

        if (tag == Tag::unknown)
            continue;

        auto entry = find(identifier);
        if (entry != m_tagList.end()) {
            logger(Level::debug) << " - entry is available\n";
            entry->tags.emplace_back(std::move(tag));
        }
        else {
            logger(Level::debug) << " - entry is new\n";
            addTagId(std::move(identifier), std::move(tag));
        }

    }

}

std::vector<Tag> SongTagReader::findSongTagList(const std::string &albumName, const std::string &titleName, const std::string &performerName) {

    std::vector<Tag> songTagList;

    std::string stage1 = performerName + ":" + albumName + ":" + titleName;
    std::string stage2 = performerName + ":" + albumName;
    std::string stage3 = performerName;

    for (const auto& elem : m_tagList) {
        // first find tags for title only
        if (stage1 == elem.fullSearchName) {
            songTagList.insert(songTagList.end(), std::begin(elem.tags), std::end(elem.tags));
        }
        else if (stage2 == elem.fullSearchName) {
            songTagList.insert(songTagList.end(), std::begin(elem.tags), std::end(elem.tags));
        }
        else if (stage3 == elem.fullSearchName) {
            songTagList.insert(songTagList.end(), std::begin(elem.tags), std::end(elem.tags));
        }
    }

    return songTagList;
}
