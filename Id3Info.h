#ifndef SERVER_ID3INFO_H
#define SERVER_ID3INFO_H

#include <string>
#include <cstdint>

struct Id3Info {
    std::string uid;
    std::string album_name;
    std::string titel_name;
    std::string performer_name;
    uint32_t track_no;
    uint32_t all_tracks_no;
};


#endif //SERVER_ID3INFO_H
