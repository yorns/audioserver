#ifndef SERVER_ID3TAGREADER_H
#define SERVER_ID3TAGREADER_H

#include "Id3Info.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <optional>

#include <fstream>
#include <fileref.h>
#include <id3v2tag.h>
#include <mpegfile.h>
#include <id3v2frame.h>
#include <id3v2header.h>
#include <attachedpictureframe.h>

#include "common/Constants.h"
#include "common/logger.h"

struct FullId3Information {
    Id3Info info;
    bool pictureAvailable {false};
    std::vector<char> data;
    std::size_t hash {0};
};

class id3TagReader {

    std::string unknownCover();
    std::vector<Id3Info> readStreamInfo(const std::string& filename);

    TagLib::ID3v2::Frame* getAttachedFrame(TagLib::ID3v2::Tag *id3v2tag, const char* idName) {

        if (!id3v2tag || id3v2tag->frameListMap()[idName].isEmpty()) {
            logger(LoggerFramework::Level::warning) << "id3v2 tag for " << idName << " not present\n";
            return nullptr;
        }

        TagLib::ID3v2::FrameList Frame = id3v2tag->frameListMap()[idName];
        if (Frame.size() == 0)
            return nullptr;

        return Frame.front();
    }

public:
    //std::string extractCover(const std::string& uid);
    //std::optional<Id3Info> getInfo(const std::string& uniqueId, const std::string& cover);
    std::vector<Id3Info> getStreamInfo(const std::string& uniqueId);
    std::optional<FullId3Information> extractId3Info(const std::string &uid);

};


#endif //SERVER_ID3TAGREADER_H
