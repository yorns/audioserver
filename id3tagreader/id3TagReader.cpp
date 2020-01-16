#include "id3TagReader.h"



Id3Info id3TagReader::getInfo(const std::string &uniqueId, const std::string &cover) {

    std::string mp3File = ServerConstant::base_path.to_string() + "/" + ServerConstant::audioPath.to_string() + "/" + uniqueId + ".mp3";

    logger(Level::debug) << "Read mp3 info from file <"<<mp3File<<">\n";

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
        logger(Level::warning) << "Error reading mp3 data\n";
        return Id3Info();
    }

    return info;

}
