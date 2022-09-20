#include "id3TagReader.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <fstream>
#include <fileref.h>
#include <mpegfile.h>
#include <id3v2tag.h>
#include <id3v2frame.h>
#include <id3v2header.h>
#include <id3v1tag.h>
#include <attachedpictureframe.h>
#include <mp4file.h>
#include <cctype>

#include "common/logger.h"
#include "common/Constants.h"
#include "common/NameGenerator.h"
#include "common/hash.h"
#include "common/base64.h"

#include "nlohmann/json.hpp"

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

std::optional<FullId3Information> id3TagReader::readJsonAudioInfo(const Common::FileNameType& file) {

    std::string streamFileName = FileSystemAdditions::getFullName(file);
    std::string uid = Common::NameGenerator::create("","").unique_id;

    if (fs::exists(streamFileName)) {
        FullId3Information fullInfo;
        try {
            std::ifstream streamInfoFile(streamFileName.c_str());
            nlohmann::json streamInfo = nlohmann::json::parse(streamInfoFile);

            Id3Info info;
            info.uid = boost::lexical_cast<boost::uuids::uuid>(std::string(streamInfo.at(ServerConstant::JsonField::uid)));
            info.informationSource = "file://" + streamFileName;
            info.title_name = streamInfo.at(ServerConstant::JsonField::title);
            info.album_name = streamInfo.at(ServerConstant::JsonField::album);
            info.performer_name = streamInfo.at(ServerConstant::JsonField::performer);
            info.track_no = streamInfo.at(ServerConstant::JsonField::trackNo);
            info.all_tracks_no = streamInfo.at(ServerConstant::JsonField::allTrackNo);
            info.urlAudioFile = streamInfo.at(ServerConstant::JsonField::url);
            info.coverFileExt = streamInfo.at(ServerConstant::JsonField::extension);
            info.albumCreation = streamInfo.at(ServerConstant::JsonField::albumCreation);

            if (streamInfo.find(ServerConstant::JsonField::image) != streamInfo.end()) {
                auto cover = utility::base64_decode(streamInfo.at(ServerConstant::JsonField::image));
                fullInfo.pictureAvailable = true;
                fullInfo.hash = Common::genHash(cover);
                fullInfo.data = std::move(cover);
                info.urlCoverFile =
                        Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::CoversRelative) +
                        "/" + boost::uuids::to_string(info.uid) + info.coverFileExt;
            }
            else {
                if (streamInfo.find(ServerConstant::JsonField::imageUrl) != streamInfo.end()) {

                    info.urlCoverFile = streamInfo.at(ServerConstant::JsonField::imageUrl);
                    fullInfo.pictureAvailable = true;
                    logger(Level::info) << "  - external cover url for <" << boost::uuids::to_string(info.uid) << "> is <"<<info.urlCoverFile<<">\n";

                }
                else {
                    fullInfo.pictureAvailable = false;
                    fullInfo.hash = 0;
                    info.urlCoverFile =
                            Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::CoversRelative) +
                            "/" + std::string(ServerConstant::unknownCoverFile) + std::string(ServerConstant::unknownCoverExtension);
                }
            }

            info.finishEntry();

            logger(LoggerFramework::Level::debug) << "reading <" << boost::uuids::to_string(info.uid) << "> <" << info.title_name << ">\n";

            fullInfo.info = std::move(info);

        } catch (std::exception& ex) {
            logger(Level::warning) << "failed to read file: " << streamFileName << ": " << ex.what() << "\n";
            return std::nullopt;
        }

        return fullInfo;
    }

    logger(Level::debug) << "file <"<<streamFileName<<"> does not exist\n";
    return std::nullopt;
}

std::optional<FullId3Information> id3TagReader::readMp3AudioInfo(const Common::FileNameType &fileName) {

    static const auto idPicture = "APIC";
    static const auto idDiskNo = "TPOS";

    std::string mp3File = FileSystemAdditions::getFullName(fileName);

    if (fs::exists(mp3File) && fileName.extension == ServerConstant::mp3Extension) {

        TagLib::MPEG::File mpegFile(mp3File.c_str());
        std::string uid = Common::NameGenerator::create("","").unique_id;
        std::string fileUrl = "file://" + mp3File;

        if (mpegFile.hasID3v2Tag()) {

            logger(Level::debug) << "extracting data from file <"<<mp3File<<"> is a id3v2\n";

            auto id3v2tag = mpegFile.ID3v2Tag();

            Id3Info info;
            try {
                info.uid = boost::lexical_cast<boost::uuids::uuid>(uid);
                info.title_name = id3v2tag->title().to8Bit(true);
                info.performer_name = id3v2tag->artist().to8Bit(true);
                info.album_name = id3v2tag->album().to8Bit(true);
                info.track_no = id3v2tag->track();
                info.all_tracks_no = 0; // no API to get overall number of tracks (even, when it is hold)
                // TRCK (Track number/Position in set): 14/14
                auto diskNoItem = id3v2tag->frameListMap().find(idDiskNo);
                if (diskNoItem != id3v2tag->frameListMap().end() && !diskNoItem->second.isEmpty()) {
                    auto frameTagDiskNo = diskNoItem->second.front();
                    std::stringstream(frameTagDiskNo->toString().to8Bit(true)) >> info.cd_no;
                }
                info.urlAudioFile = fileUrl;
                info.informationSource = fileUrl;
                info.finishEntry();     // help search by adding strings on lowercase

            } catch (std::exception& ex) {
                logger(LoggerFramework::Level::warning) << ex.what() << "\n";
            }
            info.urlCoverFile = ServerConstant::unknownCoverUrl;

            FullId3Information fullId3Info;
            fullId3Info.info = std::move(info);

            auto Frame = id3v2tag->frameListMap().find(idPicture);
            if (Frame != id3v2tag->frameListMap().end() && !Frame->second.isEmpty()) {

                const auto& frameTagPicture = Frame->second.front();

                auto picFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameTagPicture);
                if (picFrame) {
                    std::string filetype(".jpg");
                    if (!picFrame->mimeType().isEmpty()) {
                        std::string tmp{picFrame->mimeType().to8Bit()};
                        fullId3Info.info.coverFileExt = "." + tmp.substr(tmp.find_last_of('/')+1);
                    }

                    // generate entry for cover Table
                    std::vector<char> coverData;
                    coverData.resize(picFrame->picture().size());

                    std::copy_n(picFrame->picture().data(), picFrame->picture().size(), coverData.begin());

                    auto hash = Common::genHash(coverData);

                    fullId3Info.data = std::move(coverData);
                    fullId3Info.hash = hash;
                    fullId3Info.pictureAvailable = true;

                    // assume image is set correctly (later)
                    fullId3Info.info.urlCoverFile = std::string(ServerConstant::coverPathWeb) + "/"
                            + boost::lexical_cast<std::string>(uid) + fullId3Info.info.coverFileExt;

                    logger(Level::debug) << "image found for <" << fullId3Info.info.toString() << ">\n";
                }

            }

            if (!fullId3Info.pictureAvailable)
                logger(Level::info) << "image NOT found for <" << fullId3Info.info.toString() << ">\n";

            return fullId3Info;
        }

        if (mpegFile.hasID3v1Tag()) {

            logger(Level::debug) << "extracting data from file <"<<mp3File<<"> is a id3v1\n";

            auto id3v1tag = mpegFile.ID3v1Tag();

            Id3Info info;
            try {
                info.uid = boost::lexical_cast<boost::uuids::uuid>(uid);
                info.title_name = id3v1tag->title().to8Bit(true);
                info.performer_name = id3v1tag->artist().to8Bit(true);
                info.album_name = id3v1tag->album().to8Bit(true);
                info.track_no = id3v1tag->track();
                info.all_tracks_no = 0; // no API to get overall number of tracks (even, when it is hold)
                // TRCK (Track number/Position in set): 14/14
                // TODO read CD number information
                info.urlAudioFile = fileUrl;
                info.informationSource = fileUrl;
                info.urlCoverFile = ServerConstant::unknownCoverUrl;
                info.finishEntry();     // help search by adding strings on lowercase
            } catch (std::exception& ex) {
                logger(LoggerFramework::Level::warning) << ex.what() << "\n";
            }

            FullId3Information fullId3Info;
            fullId3Info.info = std::move(info);

            if (!fullId3Info.pictureAvailable)
                logger(Level::debug) << "image NOT found for <" << fullId3Info.info.toString() << ">\n";

            return fullId3Info;
        }
    }
    if (fs::exists(mp3File) && fileName.extension == ServerConstant::mp4Extension) {
        TagLib::MP4::File mpegFile(mp3File.c_str());

        if (mpegFile.hasMP4Tag()) {
            std::string uid = Common::NameGenerator::create("","").unique_id;
            std::string fileUrl = "file://" + mp3File;

            auto idData = mpegFile.tag();

            idData->title();
            Id3Info info;
            try {
                info.uid = boost::lexical_cast<boost::uuids::uuid>(uid);
                info.title_name = idData->title().to8Bit(true);
                info.performer_name = idData->artist().to8Bit(true);
                info.album_name = idData->album().to8Bit(true);
                info.track_no = idData->track();
                info.urlCoverFile = ServerConstant::unknownCoverUrl;
                info.all_tracks_no = 0; // no API to get overall number of tracks (even, when it is hold)
                // TRCK (Track number/Position in set): 14/14
                auto disk = idData->itemListMap().find("disk");
                if (disk != idData->itemListMap().end()) {
                    info.cd_no = static_cast<uint32_t>(disk->second.toInt());
                }
                info.informationSource = fileUrl;
                info.urlAudioFile = fileUrl;

                info.finishEntry();     // help search by adding strings on lowercase

            } catch (std::exception& ex) {
                logger(LoggerFramework::Level::warning) << ex.what() << "\n";
            }

            FullId3Information fullId3Info;
            fullId3Info.info = std::move(info);

            TagLib::MP4::ItemListMap itemsListMap = idData->itemListMap();
            TagLib::MP4::Item coverItem = itemsListMap[ServerConstant::id3::covr];
            TagLib::MP4::CoverArtList coverArtList = coverItem.toCoverArtList();

            if (!coverArtList.isEmpty() && coverArtList.front().data().size() > 0) {
                auto coverArt = coverArtList.front();

                switch (coverArt.format()) {
                case TagLib::MP4::CoverArt::PNG:
                    fullId3Info.info.coverFileExt = ServerConstant::pngExtension;
                    fullId3Info.pictureAvailable = true;
                    break;
                case TagLib::MP4::CoverArt::JPEG:
                    fullId3Info.info.coverFileExt = ServerConstant::jpegExtension;
                    fullId3Info.pictureAvailable = true;
                    break;
                default:
                    fullId3Info.pictureAvailable = false;
                    break;
                }

                if (fullId3Info.pictureAvailable == true) {
                    // generate entry for cover Table
                    std::vector<char> coverData;
                    coverData.resize(coverArt.data().size());

                    std::copy_n(coverArt.data().data(), coverArt.data().size(), coverData.begin());

                    auto hash = Common::genHash(coverData);

                    fullId3Info.data = std::move(coverData);
                    fullId3Info.hash = hash;
                    fullId3Info.pictureAvailable = true;

                    // assume image is set correctly (later)
                    fullId3Info.info.urlCoverFile = std::string(ServerConstant::coverPathWeb) + "/"
                            + boost::lexical_cast<std::string>(uid) + fullId3Info.info.coverFileExt;

                    logger(Level::debug) << "image found for <" << fullId3Info.info.toString() << ">\n";
                }

            }

            if (!fullId3Info.pictureAvailable)
                logger(Level::info) << "image NOT found for <" << fullId3Info.info.toString() << ">\n";

            return fullId3Info;
        }



    }

    if (fileName.extension == ".mp3" || fileName.extension == ".m4a")
        logger(Level::info) << "extracting data from file <"<<mp3File<<"> - no id3 available\n";

    return std::nullopt;
}
