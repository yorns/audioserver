#include "playlist.h"
#include "common/logger.h"
#include "common/Constants.h"
#include <boost/filesystem.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "common/filesystemadditions.h"
#include <nlohmann/json.hpp>
#include "common/base64.h"
#include "common/hash.h"

using namespace Database;
using namespace LoggerFramework;
using namespace Common;

namespace filesys =  boost::filesystem;

boost::uuids::uuid Playlist::getUniqueID() const
{
    return m_item.m_uniqueId;
}

void Playlist::setName(const std::string &name)
{
    m_item.m_name = name;
    m_item.m_name_lower = Common::str_tolower(name);
    setChanged(Changed::isChanged);
}

void Playlist::setPerformer(const std::string &performer)
{
    m_item.m_performer = performer;
    m_item.m_performer_lower = Common::str_tolower(performer);
    setChanged(Changed::isChanged);
}


const std::vector<boost::uuids::uuid> &Playlist::getUniqueAudioIdsPlaylist() const
{
    return m_playlist;
}

std::string Playlist::getName() const
{
    return m_item.m_name;
}

bool Playlist::addToList(boost::uuids::uuid&& audioUID) {
    m_playlist.emplace_back(audioUID);
    setChanged(Changed::isChanged);
    return true;
}

bool Playlist::delFromList(const boost::uuids::uuid &audioUID) {
    auto it = std::find_if(std::begin(m_playlist), std::end(m_playlist),
                           [&audioUID](const boost::uuids::uuid& id){ return audioUID == id; });

    if (it != std::end(m_playlist)) {
        logger(Level::info) << "removing file <"<<boost::uuids::to_string(*it)<<">\n";
        m_playlist.erase(it);
        setChanged(Changed::isChanged);
        return true;
    }

    return false;
}

std::vector<boost::uuids::uuid> Playlist::getPlaylist() {
    return m_playlist;
}


Playlist::Playlist(std::string filename, ReadType readType, Persistent persistent, Changed changed)
    : m_playlistFileName(std::move(filename)),
      m_changed(changed),
      m_persistent(persistent),
      m_readType(readType)
{
}

Playlist::Playlist(boost::uuids::uuid&& uniqueId, std::vector<boost::uuids::uuid> &&playlist, ReadType readType, Persistent persistent, Changed changed)
    : m_item(std::move(uniqueId)),
      m_playlist(std::move(playlist)),
      m_coverName("/img/unknown.png"),
//      m_coverType(CoverType::none),
      m_changed(changed),
      m_persistent(persistent),
      m_readType(readType)
{
    // set playlist filename
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

bool Playlist::readJson(FindAlgo&& findAlgo, std::function<void(boost::uuids::uuid&& uid, std::vector<char>&& data)>&& coverInsert)
{

    std::vector<boost::uuids::uuid> playlist;
    boost::uuids::uuid uid;
    std::string playlistName;
    std::string performerName;
    std::string extension;
    std::vector<char> coverData;
    std::string coverUrl;
    std::vector<Tag> tagList;

    try {
        std::ifstream streamInfoFile(m_playlistFileName.c_str());
        nlohmann::json streamInfo = nlohmann::json::parse(streamInfoFile);

        uid = boost::lexical_cast<boost::uuids::uuid>(std::string(streamInfo.at("Id")));
        playlistName = streamInfo.at("Name");
        performerName = streamInfo.at("Performer");
        extension = streamInfo.at("Extension");

        /* image handling */
        if (streamInfo.find("Image") != std::end(streamInfo)) {
            coverData = utility::base64_decode(streamInfo.at("Image"));
            auto coverUid = uid;
            logger(Level::info) << "inserting from json playlist cover id <"<<coverUid<<">\n";
            coverInsert(std::move(coverUid), std::move(coverData));
            coverUrl = std::string(ServerConstant::coverPathWeb) + "/"
                    + boost::lexical_cast<std::string>(uid) + extension;
            logger(Level::info) << "  - cover url is <"<<coverUrl<<">\n";
        }
        else {
            if (streamInfo.find("ImageUrl") != std::end(streamInfo)) {
                coverUrl = streamInfo.at("ImageUrl");
                logger(Level::info) << "  - external Image url is <"<<coverUrl<<">\n";
            }
            else {
                logger(LoggerFramework::Level::error) << "no image or image url given for playlist <" << playlistName <<">\n";
                coverUrl = std::string(ServerConstant::coverPathWeb) + "/"
                        + std::string(ServerConstant::unknownCoverFile)
                        + std::string(ServerConstant::unknownCoverExtension);
            }
        }

        if (streamInfo.find("Items") != streamInfo.end()) {
            auto items = streamInfo.at("Items");
            for (auto elem : items) {
                if (elem.find("Id") != elem.end()) {
                    auto audioItemList = findAlgo(elem.at("Id"), SearchItem::uid);
                    if (audioItemList.size() == 1)
                        playlist.emplace_back(audioItemList.at(0));
                }
                else if (elem.find("Album") != elem.end()) {
                    auto audioItemList = findAlgo(elem.at("Album"), SearchItem::album);
                    for (const auto& UuidItem : audioItemList) {
                        playlist.emplace_back(UuidItem);
                    }
                }
                else if (elem.find("Performer") != elem.end()) {
                    auto audioItemList = findAlgo(elem.at("Performer"), SearchItem::performer);
                    for (const auto& UuidItem : audioItemList) {
                        playlist.emplace_back(UuidItem);
                    }
                }
                else if (elem.find("Title") != elem.end()) {
                    auto audioItemList = findAlgo(elem.at("Title"), SearchItem::title);
                    for (const auto& UuidItem : audioItemList) {
                        playlist.emplace_back(UuidItem);
                    }
                }
            }
        }
        if (streamInfo.find("Tag") != streamInfo.end()) {
            auto tags = streamInfo["Tag"];
            for (auto elem : tags ) {
                auto tag = TagConverter::getTagId(elem);
                tagList.push_back(tag);
            }
        }

    } catch (std::exception& ex) {
        logger(Level::warning) << "failed to read file: " << m_playlistFileName << ": " << ex.what() << "\n";
        playlist.clear();
    }

    if (playlist.size() > 0) {
        logger(Level::debug) << "stream playlist: <" << uid <<"> (" << playlistName << ") <"<<playlist.size()<<"> elements read\n";
        setUniqueID(std::move(uid));        
        m_playlist = std::move(playlist);
        setName(std::move(playlistName));
        setPerformer(std::move(performerName));
        setTagList(tagList);
        setCover(coverUrl);
        return true;
    }
    return false;
}

bool Playlist::readM3u()
{
    return false;
    // read M3u is unavailable
//    std::string fullFilename = FileSystemAdditions::getFullQualifiedDirectory(FileType::PlaylistM3u)+ '/' +
//            boost::lexical_cast<std::string>(getUniqueID()) + ".m3u";

//    logger(LoggerFramework::Level::debug) << "reading playlist file <"<<fullFilename<<">\n";

//    std::ifstream stream(fullFilename);

//    std::string line;
//    std::string playlistName;

//    if (!stream.good())
//        return false;

//    // read human readable playlist name or only first line
//    if (stream.good() && std::getline(stream, line)) {
//        if (line.length() > 2 && line[0] == '#' && line[1] == ' ') {
//            setName(line.substr(2));
//        } else {
//            filesys::path fullName(line);
//            std::string audioUniqueID { fullName.stem().string() };
//            if (!audioUniqueID.empty()) {
//                if (audioUniqueID.find("https://",0) == std::string::npos) {
//                    addToList(std::move(audioUniqueID));
//                    logger(LoggerFramework::Level::debug) << "  reading: <" << fullName.stem() << ">\n";
//                }
//                else {
//                    // create uniqueID
//                    // add uniqueId and url to list
//                }
//            }
//        }
//    }

//    logger(LoggerFramework::Level::debug) << "playlist name: "<<getUniqueID()<<"\n";

//    while (stream.good() && std::getline(stream, line)) {
//        filesys::path fullName(line);
//        if (!fullName.stem().empty()) {
//            std::string audioUniqueID { fullName.stem().string() };
//            addToList(std::move(audioUniqueID));
//            logger(LoggerFramework::Level::debug) << "  reading: <" << fullName.stem() << ">\n";
//        }
//    }

//    return true;

}

bool Playlist::write()
{
    return false;

//    bool success { false };
//    if ( persistent() == Persistent::isPermanent &&
//         changed() == Changed::isChanged ) {
//        logger(LoggerFramework::Level::debug) << "writeChangedPlaylist <" << getUniqueID()
//                                              << "> realname <" << getName() << ">"
//                                              << " - try write ... \n";

//        std::string filename = FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::PlaylistM3u)
//                + "/" + getUniqueID() + ".m3u";

//        std::ofstream ofs ( filename, std::ios_base::out );

//        if (ofs.good()) {
//            ofs << "# " << getName() << "\n";

//            for (const auto &entry : getUniqueIdPlaylist()) {
//                ofs << "../" << ServerConstant::audioPathMp3 << "/" << entry << ".mp3\n";
//            }
//            success = true;
//            logger(LoggerFramework::Level::debug) << " done \n";

//        } else {
//            logger(LoggerFramework::Level::warning) << "writing to file <" << filename << "> failed \n";
//            success = false;
//        }
//    } else {
//        success = true;
//        if (persistent() == Persistent::isTemporal)
//            logger(LoggerFramework::Level::debug) << "Playlist " << getName() << " is temporal and will not persisted\n";
//        else
//            logger(LoggerFramework::Level::debug) << "Playlist " << getName() << " has not been changed\n";
//    }

//    if (success)
//        setChanged(Changed::isUnchanged);

//    return success;
}

std::string Playlist::getCover() const
{
    return m_coverName;
}

bool Playlist::setCover(std::string coverUrl) {
    if (coverUrl.empty())
        coverUrl = "img/unknown.png";

    logger(LoggerFramework::Level::debug) << "setCover: <"<<coverUrl<<">\n";
    m_coverName = coverUrl;
    return true;
}

