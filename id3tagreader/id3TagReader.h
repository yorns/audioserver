#ifndef SERVER_ID3TAGREADER_H
#define SERVER_ID3TAGREADER_H

#include "Id3Info.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <optional>

#include <fstream>
#include <fileref.h>
#include <mpegfile.h>
#include <id3v2tag.h>
#include <id3v2frame.h>
#include <id3v2header.h>
#include <id3v1tag.h>
#include <attachedpictureframe.h>

#include "common/logger.h"
#include "common/Constants.h"
#include "common/NameGenerator.h"
#include "common/filesystemadditions.h"

struct FullId3Information {
    Id3Info info;
    bool pictureAvailable {false};
    std::vector<char> data;
    std::size_t hash {0};
};

class id3TagReader {

    std::string unknownCover();

public:
    std::optional<FullId3Information> readJsonAudioInfo(const Common::FileNameType& uid);
    std::optional<FullId3Information> readMp3AudioInfo(const Common::FileNameType& uid);

};


#endif //SERVER_ID3TAGREADER_H
