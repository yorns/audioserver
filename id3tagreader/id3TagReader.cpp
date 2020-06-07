#include "id3TagReader.h"
#include "nlohmann/json.hpp"
#include "common/hash.h"
#include "common/base64.h"

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

    std::string streamFileName = FileSystemAdditions::getFullQualifiedDirectory(FileType::AudioJson)
            + "/" + file.name + file.extension;
    std::string uid = Common::NameGenerator::create("","").unique_id;

    if (fs::exists(streamFileName)) {
        FullId3Information fullInfo;
        try {
            std::ifstream streamInfoFile(streamFileName.c_str());
            nlohmann::json streamInfo = nlohmann::json::parse(streamInfoFile);

            Id3Info info;
            info.uid = file.name;
            info.informationSource = "file://" + streamFileName;
            info.title_name = streamInfo.at("Title");
            info.album_name = streamInfo.at("Album");
            info.performer_name = streamInfo.at("Performer");
            info.track_no = streamInfo.at("TrackNo");
            info.all_tracks_no = streamInfo.at("AllTrackNo");
            info.url = streamInfo.at("Url");
            info.fileExtension = streamInfo.at("Extension");
            info.albumCreation = streamInfo.at("AlbumCreation");

            if (streamInfo.find("Image") != streamInfo.end()) {
                auto cover = utility::base64_decode(streamInfo.at("Image"));
                fullInfo.pictureAvailable = true;
                fullInfo.hash = Common::genHash(cover);
                fullInfo.data = std::move(cover);
            }
            else {
                fullInfo.pictureAvailable = false;
                fullInfo.hash = 0;
            }

            info.finishEntry();

            logger(LoggerFramework::Level::debug) << "reading <" << info.uid << "> <" << info.title_name << ">\n";

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

    std::string mp3File = FileSystemAdditions::getFullQualifiedDirectory(FileType::AudioMp3) + '/' + fileName.name + fileName.extension;

    if (fs::exists(mp3File)) {

        TagLib::MPEG::File mpegFile(mp3File.c_str());
        std::string uid = Common::NameGenerator::create("","").unique_id;
        std::string fileUrl = "file://" + mp3File;

        if (mpegFile.hasID3v2Tag()) {

            logger(Level::debug) << "extracting data from file <"<<mp3File<<"> is a id3v2\n";

            auto id3v2tag = mpegFile.ID3v2Tag();

            Id3Info info;
            info.uid = uid;
            info.title_name = id3v2tag->title().to8Bit(true);
            info.performer_name = id3v2tag->artist().to8Bit(true);
            info.album_name = id3v2tag->album().to8Bit(true);
            info.track_no = id3v2tag->track();
            info.all_tracks_no = 0; // no API to get overall number of tracks (even, when it is hold)
            // TRCK (Track number/Position in set): 14/14
            info.url = fileUrl;
            info.informationSource = fileUrl;
            info.finishEntry();     // help search by adding strings on lowercase

            FullId3Information fullId3Info;
            fullId3Info.info = std::move(info);

            if (!id3v2tag->frameListMap()[idPicture].isEmpty()) {

                auto Frame = id3v2tag->frameListMap()[idPicture];
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

                        logger(Level::debug) << "image found for <" << fullId3Info.info.toString() << ">\n";
                    }
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
            info.uid = uid;
            info.title_name = id3v1tag->title().to8Bit(true);
            info.performer_name = id3v1tag->artist().to8Bit(true);
            info.album_name = id3v1tag->album().to8Bit(true);
            info.track_no = id3v1tag->track();
            info.all_tracks_no = 0; // no API to get overall number of tracks (even, when it is hold)
            // TRCK (Track number/Position in set): 14/14
            info.url = fileUrl;
            info.informationSource = fileUrl;
            info.finishEntry();     // help search by adding strings on lowercase

            FullId3Information fullId3Info;
            fullId3Info.info = std::move(info);

            if (!fullId3Info.pictureAvailable)
                logger(Level::debug) << "image NOT found for <" << fullId3Info.info.toString() << ">\n";

            return fullId3Info;
        }

    }

    logger(Level::info) << "extracting data from file <"<<mp3File<<"> - no id3 available\n";

    return std::nullopt;
}
