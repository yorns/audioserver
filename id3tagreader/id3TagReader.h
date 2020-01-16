#ifndef SERVER_ID3TAGREADER_H
#define SERVER_ID3TAGREADER_H

#include "Id3Info.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
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
    Id3Info getInfo(const std::string& uniqueId, const std::string& cover);


    static std::string extractCover(const std::string& uid) {

        static const char *IdPicture = "APIC";
        TagLib::ID3v2::Tag *id3v2tag;
        TagLib::ID3v2::FrameList Frame;

        std::stringstream mp3File;
        mp3File << ServerConstant::base_path << "/" << ServerConstant::audioPath << "/" << uid << ".mp3";

        TagLib::MPEG::File mpegFile(mp3File.str().c_str());
        id3v2tag = mpegFile.ID3v2Tag();

        if (!id3v2tag || id3v2tag->frameListMap()[IdPicture].isEmpty()) {
            logger(Level::warning) << "id3v2 not present\n";
            return "";
        }
        // picture frame
        Frame = id3v2tag->frameListMap()[IdPicture];

        for (auto &it : Frame) {
            auto PicFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(it);

            if (PicFrame->picture().size() > 0) {

                std::string filetype(".jpg");
                if (!PicFrame->mimeType().isEmpty()) {
                    std::string tmp{PicFrame->mimeType().to8Bit()};
                    filetype = "." + tmp.substr(tmp.find_last_of('/')+1);
                }
                std::stringstream coverImageName;
                std::stringstream coverName;

                coverImageName << uid << filetype;

                coverName << ServerConstant::base_path << "/" << ServerConstant::coverPath;
                coverName << "/" << coverImageName.str();

                std::ofstream of(coverName.str(), std::ios_base::out | std::ios_base::binary);
                if (of.good()) {
                    of.write(PicFrame->picture().data(), PicFrame->picture().size());
                }

                of.close();

                return ServerConstant::coverPathWeb.to_string() + "/" + coverImageName.str();
            }
        }

        return  ServerConstant::coverPathWeb.to_string() + "/" +
                ServerConstant::unknownCoverFile.to_string() +
                ServerConstant::unknownCoverExtension.to_string();
    }
};


#endif //SERVER_ID3TAGREADER_H
