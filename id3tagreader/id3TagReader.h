#ifndef SERVER_ID3TAGREADER_H
#define SERVER_ID3TAGREADER_H

#include "Id3Info.h"
#include <optional>
#include <vector>
#include "Id3Info.h"
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
