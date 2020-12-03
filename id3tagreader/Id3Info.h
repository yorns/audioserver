#ifndef SERVER_ID3INFO_H
#define SERVER_ID3INFO_H

#include <string>
#include <vector>
#include <sstream>
#include <cstdint>
#include <fstream>
#include <algorithm>
#include "common/stringmanipulator.h"
#include "common/filesystemadditions.h"
#include "common/logger.h"
#include <boost/uuid/uuid.hpp>

#include "idtag.h"


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
    std::vector<Tag> tags;
    bool albumCreation { true };

    std::string informationSource;

    std::string urlAudioFile;
    std::string audioFileExt;
    std::string urlCoverFile;
    std::string coverFileExt;

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

    void setTags(std::vector<Tag>&& tagList) {
        tags = std::move(tagList);
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

    bool isAlikeTag(const std::vector<std::string>& whatList) const {
        for (auto& part : whatList) {
            std::string tagName = Common::str_tolower(part);

            auto tag = TagConverter::getTagIdAlike(tagName);

            if (tag != Tag::unknown) {
                if (std::find_if(std::begin(tags), std::end(tags),
                                 [&tag](const Tag& elem) { return elem == tag; }
                                 ) != std::end(tags))
                    return true;
            }
        }
        return false;
    }

    std::string getNormalizedAlbum() const {
        return albumName_lower;
    }

    std::string getNormalizedTitle() const {
        return titleName_lower;
    }

    std::string getNormalizedPerformer() const {
        return performerName_lower;
    }
};


#endif //SERVER_ID3INFO_H
