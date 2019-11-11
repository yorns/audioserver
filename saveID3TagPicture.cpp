#include<id3v2tag.h>
#include<mpegfile.h>
#include<id3v2frame.h>
#include<id3v2header.h>
#include<attachedpictureframe.h>
#include<cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace ServerConstant {
    static std::string fileRootPath{"mp3"};
    static std::string coverRootPath{"img"};
}


std::string extractCover(const std::string& uid) {

    static const char *IdPicture = "APIC";
    TagLib::ID3v2::Tag *id3v2tag;
    TagLib::ID3v2::FrameList Frame;
    TagLib::ID3v2::AttachedPictureFrame *PicFrame;

    std::stringstream mp3File;
    mp3File << ServerConstant::fileRootPath << "/" << uid << ".mp3";

    TagLib::MPEG::File mpegFile(mp3File.str().c_str());
    id3v2tag = mpegFile.ID3v2Tag();

    std::cout << "file is: "<< mp3File.str() << "\n";

    if (!id3v2tag || id3v2tag->frameListMap()[IdPicture].isEmpty()) {
        std::cout << "id3v2 not present\n";
        return "";
    }

    // picture frame
    Frame = id3v2tag->frameListMap()[IdPicture];

    for (auto &it : Frame) {
        PicFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(it);
        std::cout << "Type = " << PicFrame->type() << ", mimetype: " << PicFrame->mimeType() << "\n";

        if (PicFrame->picture().size() > 0) {

            std::string filetype(".jpg"); // default

            if (!PicFrame->mimeType().isEmpty()) {
                std::string tmp{PicFrame->mimeType().to8Bit()};
                filetype = "." + tmp.substr(tmp.find_last_of('/')+1);
            }
            std::stringstream coverName;
            coverName << ServerConstant::coverRootPath;
            coverName << "/" << uid;
            coverName << filetype;
            std::ofstream of(coverName.str(), std::ios_base::out | std::ios_base::binary);
            if (of.good()) {
                of.write(PicFrame->picture().data(), PicFrame->picture().size());
            }

            of.close();
            return coverName.str();
        }
    }

    return "";
}


int main(int argc, char* argv[])
{
    std::cout << "generate: " << extractCover(argv[1]) << "\n";
    return 0;
}

