#ifndef SERVER_ID3INFO_H
#define SERVER_ID3INFO_H

#include <string>
#include <vector>
#include <sstream>
#include <cstdint>
#include "common/stringmanipulator.h"
#include <boost/uuid/uuid.hpp>

class Id3Info {

    std::string albumName_lower;
    std::string titleName_lower;
    std::string performerName_lower;

public:

    boost::uuids::uuid uid;
    std::string album_name;
    std::string title_name;
    std::string performer_name;
    uint32_t track_no {0};
    uint32_t all_tracks_no {0};
    uint32_t cd_no {0};
    uint32_t genreId;
    bool albumCreation { true };

    std::string informationSource;

    std::string fileExtension;
    std::string url;

    bool operator==(const Id3Info& info) const {
        return album_name == info.album_name &&
                title_name == info.title_name &&
                performer_name == info.performer_name;
    }

    bool operator<(const Id3Info& info) const {
        if (cd_no == info.cd_no)
            return uid < info.uid;
        return cd_no < info.cd_no;
    }

    Id3Info() = default;
    Id3Info(const Id3Info& info) = default;
    Id3Info(Id3Info&& info) = default;

    bool finishEntry() {
        albumName_lower = Common::str_tolower(album_name);
        performerName_lower = Common::str_tolower(performer_name);
        titleName_lower = Common::str_tolower(title_name);
        return true;
    }

    Id3Info& operator=(const Id3Info& info) = default;
    Id3Info& operator=(Id3Info&& info) = default;

    std::string toString() const {
        std::stringstream tmp;
        tmp << album_name << "("<<albumName_lower<<") / " << performer_name << " (" << performerName_lower <<")";
        return tmp.str();
    }

    bool isAlikeAlbum(const std::vector<std::string>& whatList) const {
        for (auto& part : whatList) {
            if ( albumName_lower.find(part) != std::string::npos ) {
                return true;
            }
        }
        return false;
    }

    bool isAlikePerformer(const std::vector<std::string>& whatList) const {
        for (auto& part : whatList) {
            if ( performerName_lower.find(part) != std::string::npos ) {
                return true;
            }
        }
        return false;
    }

    bool isAlikeTitle(const std::vector<std::string>& whatList) const {
        for (auto& part : whatList) {
            if ( titleName_lower.find(part) != std::string::npos ) {
                return true;
            }
        }
        return false;
    }

};


#endif //SERVER_ID3INFO_H
