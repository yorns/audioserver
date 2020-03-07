#include "id3TagReader.h"
#include "common/filesystemadditions.h"

namespace fs = boost::filesystem;
using namespace LoggerFramework;
using namespace Common;

std::optional<Id3Info> id3TagReader::getInfo(const std::string &uniqueId, const std::string &cover) {

    std::string mp3FileName = FileSystemAdditions::getFullQualifiedDirectory(FileType::Audio)
            + "/" + uniqueId + ".mp3";

    Id3Info info;

    if (!fs::exists(mp3FileName)) {
       logger(Level::warning) << "file <"<<mp3FileName<<"> does not exist\n";
       return std::nullopt;
    }

    logger(Level::debug) << "Read mp3 info from file <"<<mp3FileName<<">\n";
    TagLib::FileRef f(mp3FileName.c_str());

    try {
        fs::path mp3File {mp3FileName};
        info.uid = mp3File.stem().string();

        info.title_name = f.tag()->title().to8Bit(true);
        info.track_no = f.tag()->track();
        info.album_name = f.tag()->album().to8Bit(true);
        info.performer_name = f.tag()->artist().to8Bit(true);
        info.all_tracks_no = 0;
        info.imageFile = cover;

    }
    catch(std::exception& ) {
        logger(Level::warning) << "Error reading mp3 data\n";
        return std::nullopt;
    }

    return std::move(info);

}

std::string id3TagReader::unknownCover() {
    return  std::string(ServerConstant::coverPathWeb) + "/" +
            std::string(ServerConstant::unknownCoverFile) +
            std::string(ServerConstant::unknownCoverExtension);
}

std::string id3TagReader::extractCover(const std::string &uid) {

    static const char *IdPicture = "APIC";
    TagLib::ID3v2::Tag *id3v2tag;
    TagLib::ID3v2::FrameList Frame;

    std::string mp3File = FileSystemAdditions::getFullQualifiedDirectory(FileType::Audio) + '/' + uid + ".mp3";

    TagLib::MPEG::File mpegFile(mp3File.c_str());
    id3v2tag = mpegFile.ID3v2Tag();

    if (!id3v2tag || id3v2tag->frameListMap()[IdPicture].isEmpty()) {
        logger(Level::warning) << "id3v2 not present\n";
        return unknownCover();
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

            return std::string(ServerConstant::coverPathWeb) + "/" + coverImageName.str();
        }
    }

    return  unknownCover();
}
