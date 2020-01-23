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

class id3TagReader {

public:
    std::optional<Id3Info> getInfo(const std::string& uniqueId, const std::string& cover);
    std::optional<std::string> extractCover(const std::string& uid);
};


#endif //SERVER_ID3TAGREADER_H
