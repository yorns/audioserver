#include "playlist.h"
#include "common/logger.h"
#include "common/Constants.h"
#include <boost/filesystem.hpp>
#include "common/filesystemadditions.h"
#include <nlohmann/json.hpp>

using namespace Database;
using namespace LoggerFramework;
using namespace Common;

namespace filesys =  boost::filesystem;

std::string Playlist::getUniqueID() const
{
    return m_item.m_uniqueId;
}

void Playlist::setName(const std::string &name)
{
    m_item.m_name = name;
    m_item.m_name_lower = Common::str_tolower(name);
    setChanged(Changed::isChanged);
}

const std::vector<std::string> &Playlist::getUniqueIdPlaylist() const
{
    return m_playlist;
}

std::string Playlist::getName() const
{
    return m_item.m_name;
}

bool Playlist::addToList(std::string &&audioUID) {
    m_playlist.emplace_back(audioUID);
    setChanged(Changed::isChanged);
    return true;
}

bool Playlist::delFromList(const std::string &audioUID) {
    auto it = std::find_if(std::begin(m_playlist), std::end(m_playlist),
                           [&audioUID](const std::string& id){ return audioUID == id; });

    if (it != std::end(m_playlist)) {
        logger(Level::info) << "removing file <"<<*it<<">\n";
        m_playlist.erase(it);
        setChanged(Changed::isChanged);
        return true;
    }

    return false;
}

Playlist::Playlist(const std::string &uniqueId, ReadType readType, Persistent persistent, Changed changed)
    : m_item(uniqueId),
      m_changed(changed),
      m_persistent(persistent),
      m_readType(readType)
{
}

Playlist::Playlist(const std::string &uniqueId, std::vector<std::string> &&playlist, ReadType readType, Persistent persistent, Changed changed)
    : m_item(uniqueId), m_playlist(std::move(playlist)),
      m_changed(changed), m_persistent(persistent), m_readType(readType)
{
}

Changed Playlist::changed() const
{
    return m_changed;
}

void Playlist::setChanged(const Changed &changed)
{
    m_changed = changed;
}

Persistent Playlist::persistent() const
{
    return m_persistent;
}

void Playlist::setPersistent(const Persistent &persistent)
{
    m_persistent = persistent;
}

bool Playlist::readJson()
{
    std::string filename(Common::FileSystemAdditions::getFullQualifiedDirectory(FileType::PlaylistJson)+ "/" + m_item.m_uniqueId + ".json");
    std::vector<std::string> playlist;
    std::string playlistName;

    try {
        std::ifstream streamInfoFile(filename.c_str());
        nlohmann::json streamInfo = nlohmann::json::parse(streamInfoFile);
        playlistName = streamInfo.at("Name");
        auto items = streamInfo.at("Items");
        for (auto elem : items) {
            playlist.emplace_back(elem.at("Id"));
        }

    } catch (std::exception& ex) {
        logger(Level::warning) << "failed to read file: " << filename << ": " << ex.what() << "\n";
        playlist.clear();
    }

    if (playlist.size() > 0) {
        logger(LoggerFramework::Level::debug) << "stream playlist: <"<<playlist.size()<<" elements read\n";
        m_playlist = std::move(playlist);
        setName(playlistName);
        return true;
    }
    return false;
}

bool Playlist::readM3u()
{
    std::string fullFilename = FileSystemAdditions::getFullQualifiedDirectory(FileType::Playlist)+ '/' + getUniqueID() + ".m3u";

    logger(LoggerFramework::Level::debug) << "reading playlist file <"<<fullFilename<<">\n";

    std::ifstream stream(fullFilename);

    std::string line;
    std::string playlistName;

    if (!stream.good())
        return false;

    // read human readable playlist name or only first line
    if (stream.good() && std::getline(stream, line)) {
        if (line.length() > 2 && line[0] == '#' && line[1] == ' ') {
            setName(line.substr(2));
        } else {
            filesys::path fullName(line);
            std::string audioUniqueID { fullName.stem().string() };
            if (!audioUniqueID.empty()) {
                if (audioUniqueID.find("https://",0) == std::string::npos) {
                    addToList(std::move(audioUniqueID));
                    logger(LoggerFramework::Level::debug) << "  reading: <" << fullName.stem() << ">\n";
                }
                else {
                    // create uniqueID
                    // add uniqueId and url to list
                }
            }
        }
    }

    logger(LoggerFramework::Level::debug) << "playlist name: "<<getUniqueID()<<"\n";

    while (stream.good() && std::getline(stream, line)) {
        filesys::path fullName(line);
        if (!fullName.stem().empty()) {
            std::string audioUniqueID { fullName.stem().string() };
            addToList(std::move(audioUniqueID));
            logger(LoggerFramework::Level::debug) << "  reading: <" << fullName.stem() << ">\n";
        }
    }

    return true;

}

bool Playlist::write()
{
    bool success { false };
    if ( persistent() == Persistent::isPermanent &&
         changed() == Changed::isChanged ) {
        logger(LoggerFramework::Level::debug) << "writeChangedPlaylist <" << getUniqueID()
                                              << "> realname <" << getName() << ">"
                                              << " - try write ... \n";

        std::string filename = FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Playlist)
                + "/" + getUniqueID() + ".m3u";

        std::ofstream ofs ( filename, std::ios_base::out );

        if (ofs.good()) {
            ofs << "# " << getName() << "\n";

            for (const auto &entry : getUniqueIdPlaylist()) {
                ofs << "../" << ServerConstant::audioPath << "/" << entry << ".mp3\n";
            }
            success = true;
            logger(LoggerFramework::Level::debug) << " done \n";

        } else {
            logger(LoggerFramework::Level::warning) << "writing to file <" << filename << "> failed \n";
            success = false;
        }
    } else {
        success = true;
        if (persistent() == Persistent::isTemporal)
            logger(LoggerFramework::Level::debug) << "Playlist " << getName() << " is temporal and will not persisted\n";
        else
            logger(LoggerFramework::Level::debug) << "Playlist " << getName() << " has not been changed\n";
    }

    if (success)
        setChanged(Changed::isUnchanged);

    return success;
}

std::string Playlist::getCover() const
{
    return m_coverName;
}

//bool Playlist::rereadCover() {
//    if (m_coverType == CoverType::onPlaylistId) {
//        return false;
//    }

//    if (m_coverType == CoverType::onPlaylistItem || m_coverType == CoverType::none) {
//        auto coverList = Common::FileSystemAdditions::getAllFilesInDir(Common::FileType::Covers);

//        auto coverFile = std::find_if(std::begin(coverList), std::end(coverList),
//                                      [this](const Common::FileNameType& file){ return file.name.find(m_item.m_uniqueId) != std::string::npos; });

//        if ( coverFile != std::end(coverList)) {
//            m_item.m_cover = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::CoversRelative) + "/" +
//                    coverFile->name + coverFile->extension;
//            m_coverType = CoverType::onPlaylistId;
//            return true;
//        }
//    }

//    if (m_coverType == CoverType::none) {
//        auto coverList = Common::FileSystemAdditions::getAllFilesInDir(Common::FileType::Covers);

//        for (auto& item : m_playlist) {
//            auto coverFile = std::find_if(std::begin(coverList), std::end(coverList),
//                                          [&item](const Common::FileNameType& file){ return file.name.find(item) != std::string::npos; });

//            if ( coverFile != std::end(coverList)) {
//                m_item.m_cover = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::CoversRelative) + "/" +
//                        coverFile->name + coverFile->extension;
//                m_coverType = CoverType::onPlaylistItem;
//                return true;
//            }
//        }

//    }

//    m_item.m_cover = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::CoversRelative) + "/" +
//            std::string(ServerConstant::unknownCoverFile) + std::string(ServerConstant::unknownCoverExtension);

//    return true;
//}
