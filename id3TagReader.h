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

class id3TagReader {

public:
    Id3Info getInfo(const std::string& uniqueId, const std::string& cover) {

        std::string mp3File = ServerConstant::base_path.to_string() + "/" + ServerConstant::audioPath.to_string() + "/" + uniqueId + ".mp3";

        std::cout << "Read mp3 info from <"<<mp3File<<">\n";

        TagLib::FileRef f(mp3File.c_str());

        Id3Info info;

        try {
            boost::filesystem::path path{mp3File};
            info.uid = path.stem().string();

            info.titel_name = f.tag()->title().to8Bit(true);
            info.track_no = f.tag()->track();
            info.album_name = f.tag()->album().to8Bit(true);
            info.performer_name = f.tag()->artist().to8Bit(true);
            info.all_tracks_no = 0;
            info.imageFile = cover;

        }
        catch(std::exception& exception) {
            std::cerr << "Error reading mp3 data\n";
            return Id3Info();
        }

        return info;

    }


    static std::string extractCover(const std::string& uid) {

        static const char *IdPicture = "APIC";
        TagLib::ID3v2::Tag *id3v2tag;
        TagLib::ID3v2::FrameList Frame;
        TagLib::ID3v2::AttachedPictureFrame *PicFrame;

        std::stringstream mp3File;
        mp3File << ServerConstant::base_path << "/" << ServerConstant::audioPath << "/" << uid << ".mp3";

//        std::cout << "try to open file <"<<mp3File.str()<<">\n";

        TagLib::MPEG::File mpegFile(mp3File.str().c_str());
        id3v2tag = mpegFile.ID3v2Tag();

        if (!id3v2tag || id3v2tag->frameListMap()[IdPicture].isEmpty()) {
            std::cout << "id3v2 not present\n";
            return "";
        }
        // picture frame
        Frame = id3v2tag->frameListMap()[IdPicture];

        for (auto &it : Frame) {
            PicFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(it);
            //std::cout << "Type = " << PicFrame->type() << ", mimetype: " << PicFrame->mimeType() << "\n";

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
