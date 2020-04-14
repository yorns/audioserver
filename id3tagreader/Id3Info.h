#ifndef SERVER_ID3INFO_H
#define SERVER_ID3INFO_H

#include <string>
#include <cstdint>

class Id3Info {

public:

    std::string uid;
    std::string album_name;
    std::string title_name;
    std::string performer_name;
    uint32_t track_no {0};
    uint32_t all_tracks_no {0};
    std::string imageFile;
    std::string url;

    bool operator==(const Id3Info& info) const {
        return album_name == info.album_name &&
                title_name == info.title_name &&
                performer_name == info.performer_name;
    }

    bool operator<(const Id3Info& info) const {
        return uid < info.uid;
    }

    Id3Info() = default;
    Id3Info(const Id3Info& info) = default;
    Id3Info(Id3Info&& info) = default;

    Id3Info& operator=(const Id3Info& info) = default;
    Id3Info& operator=(Id3Info&& info) = default;
};


#endif //SERVER_ID3INFO_H
