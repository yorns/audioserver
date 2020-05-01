#include "id3TagReader.h"
#include "common/filesystemadditions.h"
#include "nlohmann/json.hpp"
#include "common/hash.h"

namespace fs = boost::filesystem;
using namespace LoggerFramework;
using namespace Common;

char convertUnreadable(char& i) {
    if (65<static_cast<unsigned int>(i) && 122>i) {
        i++;
    }
    return i;
}

std::string makeUnreadable(const std::string& block) {
    std::string output { block };
    for(auto& i : output)
        convertUnreadable(i);
    return output;
}


//std::optional<Id3Info> id3TagReader::getInfo(const std::string &uniqueId, const std::string &cover) {

//    std::string mp3FileName = FileSystemAdditions::getFullQualifiedDirectory(FileType::Audio)
//            + "/" + uniqueId + ".mp3";

//    Id3Info info;

//    if (fs::exists(mp3FileName)) {

//        logger(Level::debug) << "Read mp3 info from file <"<<mp3FileName<<">\n";
//        TagLib::FileRef f(mp3FileName.c_str());

//        try {
//            fs::path mp3File {mp3FileName};
//            info.uid = mp3File.stem().string();
//#ifdef WITH_UNREADABLE
//            info.title_name = makeUnreadable(f.tag()->title().to8Bit(true));
//            info.album_name = makeUnreadable(f.tag()->album().to8Bit(true));
//            info.performer_name = makeUnreadable(f.tag()->artist().to8Bit(true));
//#else
//            info.title_name = f.tag()->title().to8Bit(true);
//            info.album_name = f.tag()->album().to8Bit(true);
//            info.performer_name = f.tag()->artist().to8Bit(true);
//#endif
//            info.track_no = f.tag()->track();
//            info.all_tracks_no = 0;
//            info.url = "file://" + mp3FileName;
//            info.imageFile = cover;

//        }
//        catch(std::exception& ) {
//            logger(Level::warning) << "Error reading mp3 data\n";
//            return std::nullopt;
//        }

//    }
//    info.finishEntry();
//    return std::move(info);
//}

std::vector<Id3Info> id3TagReader::getStreamInfo(const std::string &uniqueId) {

    std::string streamFileName = FileSystemAdditions::getFullQualifiedDirectory(FileType::PlaylistJson)
            + "/" + uniqueId + ".json";

    std::vector<Id3Info> infoList;

    if (fs::exists(streamFileName)) {

        logger(Level::debug) << "Read stream info from file <"<<streamFileName<<">\n";

        /* read json information for stream */
        infoList = readStreamInfo(streamFileName);

        return infoList;
    }

    infoList.clear();
    logger(Level::warning) << "file <"<<streamFileName<<"> does not exist\n";
    return infoList;

}

std::vector<Id3Info> id3TagReader::readStreamInfo(const std::string& filename) {

    std::vector<Id3Info> infoList;

    try {
        std::ifstream streamInfoFile(filename.c_str());
        nlohmann::json streamInfo = nlohmann::json::parse(streamInfoFile);

        auto items = streamInfo.at("Items");

        for (auto elem : items) {
            Id3Info info;
            info.uid = elem.at("Id");
            info.title_name = elem.at("Title");
            info.album_name = elem.at("Album");
            info.performer_name = elem.at("Performer");
            info.track_no = elem.at("TrackNo");
            info.all_tracks_no = elem.at("AllTrackNo");
            info.url = elem.at("Url");
            info.finishEntry();
            logger(LoggerFramework::Level::debug) << "reading <" << info.uid << "> <"<<info.title_name<<">\n";
            infoList.emplace_back(std::move(info));
        }
    } catch (std::exception& ex) {
        logger(Level::warning) << "failed to read file: " << filename << ": " << ex.what() << "\n";
    }

    logger(LoggerFramework::Level::debug) << "stream elements read: <"<<infoList.size() << ">\n";
    return infoList;
}


std::string id3TagReader::unknownCover() {
    return  std::string(ServerConstant::coverPathWeb) + "/" +
            std::string(ServerConstant::unknownCoverFile) +
            std::string(ServerConstant::unknownCoverExtension);
}



std::optional<FullId3Information> id3TagReader::extractId3Info(const std::string &uid) {

    static const auto idPicture = "APIC";

    TagLib::ID3v2::Tag *id3v2tag;
    TagLib::ID3v2::FrameList Frame;

    std::string mp3File = FileSystemAdditions::getFullQualifiedDirectory(FileType::Audio) + '/' + uid + ".mp3";

    logger(Level::debug) << "extracting data from file <"<<mp3File<<">\n";
    TagLib::MPEG::File mpegFile(mp3File.c_str());
    id3v2tag = mpegFile.ID3v2Tag();

    Id3Info info;
    info.uid = uid;

    info.title_name = id3v2tag->title().to8Bit(true);
    info.performer_name = id3v2tag->artist().to8Bit(true);
    info.album_name = id3v2tag->album().to8Bit(true);
    info.track_no = id3v2tag->track();
    info.all_tracks_no = 0; // no API to get overall number of tracks (even, when it is hold)
                            // TRCK (Track number/Position in set): 14/14
    info.url = "file://" + mp3File;
    info.finishEntry();     // help search by adding strings on lowercase

    FullId3Information fullId3Info;
    fullId3Info.info = std::move(info);

    if (!id3v2tag->frameListMap()[idPicture].isEmpty()) {

        TagLib::ID3v2::FrameList Frame = id3v2tag->frameListMap()[idPicture];

        auto frameTagPicture = Frame.front();
        if (frameTagPicture && frameTagPicture->size() > 0) {
            auto picFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameTagPicture);
            if (picFrame) {
                std::string filetype(".jpg");
                if (!picFrame->mimeType().isEmpty()) {
                    std::string tmp{picFrame->mimeType().to8Bit()};
                    fullId3Info.info.fileExtension = "." + tmp.substr(tmp.find_last_of('/')+1);
                }

                // generate entry for cover Table
                std::vector<char> coverData;
                coverData.resize(picFrame->picture().size());

                std::copy_n(picFrame->picture().data(), picFrame->picture().size(), coverData.begin());

                auto hash = Common::genHash(coverData);

                fullId3Info.data = std::move(coverData);
                fullId3Info.hash = hash;
                fullId3Info.pictureAvailable = true;

                logger(Level::debug) << "image found for <"<<fullId3Info.info.album_name<<">\n";
            }
        }
    }

    if (!fullId3Info.pictureAvailable)
        logger(Level::debug) << "image NOT found for <"<<fullId3Info.info.album_name<<">\n";

    return std::move(fullId3Info);
}
