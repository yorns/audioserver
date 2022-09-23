#include "SimpleDatabase.h"
#include "playlist.h"
#include <variant>

using namespace Database;

void SimpleDatabase::loadDatabase() {

    SongTagReader songTagReader;

    // some helper lambdas
    auto addPlaylistCover = [this](boost::uuids::uuid uid, 
            std::vector<char>&& data)
    { m_id3Repository.addCover(std::move(uid), std::move(data)); };
    
    auto findAudioIds = [this](const std::string& what, SearchItem searchItem) {
        std::vector<boost::uuids::uuid> uuidsFound;
        auto list = m_id3Repository.search(what, searchItem, SearchAction::exact);
        for(const auto& i : list) {
            uuidsFound.push_back(i.uid);
        }
        return uuidsFound;
    };
    
    /* gather all information together */

    logger(Level::info) << "Read audio id3 data from available files \n";
    m_id3Repository.read();

    logger(Level::info) << "Read M3U playlists - unfinished \n";
    m_playlistContainer.readPlaylistsM3U();

    logger(Level::info) << "Read json playlists\n";
    m_playlistContainer.readPlaylistsJson(findAudioIds, addPlaylistCover);
    m_playlistContainer.addTags({Tag::Playlist});

    logger(Level::info) << "Auto create album playlists by audio items\n";
    m_playlistContainer.insertAlbumPlaylists(m_id3Repository.extractAlbumList());

    logger(Level::info) << "Read information from tag file\n";
    songTagReader.readSongTagFile();

    logger(Level::info) << "Add tags to relevant item\n";
    m_id3Repository.addTags(songTagReader);

    logger(Level::info) << "Lift up tags from items to playlists\n";
    m_playlistContainer.insertTagsFromItems(m_id3Repository);

    logger(Level::info) << "Read Credentials\n";
    m_credentials.read();

}


std::vector<Id3Info> SimpleDatabase::searchAudioItems(const std::string &what, SearchItem item, SearchAction action) {
        return m_id3Repository.search(what, item, action);
}

std::vector<Id3Info> SimpleDatabase::searchAudioItems(const boost::uuids::uuid &what, SearchItem item, SearchAction action) {
        return m_id3Repository.search(what, item, action);
}

std::vector<Id3Info> SimpleDatabase::searchAudioItems(std::string_view what, SearchItem item, SearchAction action) {
    return searchAudioItems(std::string(what), item, action);
}

std::vector<Id3Info> SimpleDatabase::searchAudioItems(const char *what, SearchItem item, SearchAction action) {
    return searchAudioItems(std::string(what), item, action);
}

std::vector<Playlist> SimpleDatabase::searchPlaylistItems(const std::string &what, SearchAction action) {
    return  m_playlistContainer.searchPlaylists(what, action);
}

std::vector<Playlist> SimpleDatabase::searchPlaylistItems(const std::string_view &what, SearchAction action) {
    return  m_playlistContainer.searchPlaylists(std::string(what), action);
}

std::vector<Playlist> SimpleDatabase::searchPlaylistItems(const boost::uuids::uuid &what, SearchAction action) {
    return  m_playlistContainer.searchPlaylists(what, action);
}


std::optional<boost::uuids::uuid> SimpleDatabase::createPlaylist(const std::string &name, Persistent ) {

    if (!m_playlistContainer.isUniqueName(name))
        return std::nullopt;
    return std::nullopt;
// TODO
//    const auto newPlaylistUniqueID =
//            Common::NameGenerator::create(Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::PlaylistM3u),".m3u");
//    Playlist newPlaylist(newPlaylistUniqueID.unique_id, ReadType::isM3u, persistent);
//    newPlaylist.setName(name);

//    m_playlistContainer.addPlaylist(std::move(newPlaylist));

//    return newPlaylistUniqueID.unique_id;
}

// create if not available
std::optional<boost::uuids::uuid> SimpleDatabase::getTemporalPlaylistByName(const Id3Info &info) {

    if (auto playlist = m_playlistContainer.getPlaylistByName(info.album_name)) {
        return playlist->get().getUniqueID();
    }

    Playlist newPlaylist(Common::NameGenerator::createUuid(), std::vector<boost::uuids::uuid>());
    newPlaylist.setName(info.album_name);
    newPlaylist.setPerformer(info.performer_name);
    newPlaylist.setChanged(Changed::isChanged);

    auto playlistUID = newPlaylist.getUniqueID();
    m_playlistContainer.addPlaylist(std::move(newPlaylist));

    return std::optional<boost::uuids::uuid>(playlistUID);

}

std::optional<std::string> SimpleDatabase::passwordFind(const std::string &name) {
    logger(Level::debug) << "SimpleDatabase::passwordFind\n";
    return m_credentials.passwordFind(name); }


bool SimpleDatabase::addToPlaylistName(const std::string &playlistName, std::string &&uniqueID) {
    boost::uuids::uuid uid;
    try {
        uid = boost::lexical_cast<boost::uuids::uuid>(uniqueID);
    } catch (std::exception& ex) {
        logger(LoggerFramework::Level::warning) << ex.what() << "\n";
        return false;
    }

    return m_playlistContainer.addItemToPlaylistName(playlistName, std::move(uid));
}

bool SimpleDatabase::addToPlaylistUID(const boost::uuids::uuid &playlistUid, boost::uuids::uuid &&uid) {

    return m_playlistContainer.addItemToPlaylistUID(playlistUid, std::move(uid));
}

bool SimpleDatabase::writeChangedPlaylists() {
    return m_playlistContainer.writeChangedPlaylists();
}

std::optional<boost::uuids::uuid> SimpleDatabase::convertPlaylist(const std::string &name) {
    return m_playlistContainer.convertName(name);
}

std::optional<std::string> SimpleDatabase::convertPlaylist(const boost::uuids::uuid &name) {
    return m_playlistContainer.convertName(name);
}

std::optional<Playlist> SimpleDatabase::getPlaylistByName(const std::string &playlistName) {
    auto list = m_playlistContainer.getPlaylistByName(playlistName);
    return list->get();
}

std::optional<std::vector<boost::uuids::uuid> > SimpleDatabase::getPlaylistByUID(const boost::uuids::uuid &playlistName) {
    auto playlist = m_playlistContainer.getPlaylistByUID(playlistName);
    if (playlist) {
        return playlist->get().getPlaylist();
    }
    return std::nullopt;
}

bool SimpleDatabase::setCurrentPlaylistUniqueId(boost::uuids::uuid uniqueID) {
    return m_playlistContainer.setCurrentPlaylist(std::move(uniqueID));
}


std::optional<const boost::uuids::uuid> SimpleDatabase::getCurrentPlaylistUniqueID() {
    return m_playlistContainer.getCurrentPlaylistUniqueID();
}

std::vector<std::pair<std::string, boost::uuids::uuid> > SimpleDatabase::getAllPlaylists() {
    std::vector<std::pair<std::string, boost::uuids::uuid>> list;

    return m_playlistContainer.getAllPlaylists();
}

bool SimpleDatabase::addNewAudioFileUniqueId(const Common::FileNameType &uniqueID) {

    if (auto entryUID = m_id3Repository.add(uniqueID)) {
        // find playlist or create one
        if (entryUID) {
            // if there is no album playlist, create one
            // add the new entry
            addSingleSongToAlbumPlaylist(*entryUID);
            m_id3Repository.writeCache();
        }
    }
    return true;

}

std::vector<Id3Info> SimpleDatabase::getIdListOfItemsInPlaylistId(const boost::uuids::uuid &uniqueId) {
    std::vector<Id3Info> itemList;
    if (auto playlistNameOpt = m_playlistContainer.getPlaylistByUID(uniqueId)) {
        logger(Level::info) << "playlist found for <"<<uniqueId<<"> (name:" << playlistNameOpt->get().getName()<<" | num:" << playlistNameOpt->get().getPlaylist().size() << ")\n";
        for (auto playlistItemUID : playlistNameOpt->get().getPlaylist()) {
            auto id3 = m_id3Repository.search(playlistItemUID, SearchItem::uid, SearchAction::uniqueId);
            if (id3.size() == 1) {
                itemList.push_back(id3.front());
            }
        }
    }
    return itemList;
}

Common::AlbumPlaylistAndNames SimpleDatabase::getAlbumPlaylistAndNames() {

    Common::AlbumPlaylistAndNames albumPlaylistAndNames;

    if (auto playlistNameOpt = m_playlistContainer.getCurrentPlaylist()) {
        albumPlaylistAndNames.m_playlistUniqueId = playlistNameOpt->getUniqueID();
        albumPlaylistAndNames.m_playlistName = playlistNameOpt->getName();
        const std::vector<boost::uuids::uuid> uniqueIdPlaylist = playlistNameOpt->getUniqueAudioIdsPlaylist();
        for (auto& uniqueId : uniqueIdPlaylist) {
            auto item = m_id3Repository.getId3InfoByUid(uniqueId);
            if (item) {
                Common::PlaylistItem playlistItem;
                playlistItem.m_url = item->urlAudioFile;
                playlistItem.m_uniqueId = uniqueId;
                albumPlaylistAndNames.m_playlist.emplace_back(playlistItem);
            }
        }

        logger(Level::debug) << "Album playlist found for <" << albumPlaylistAndNames.m_playlistUniqueId
                             << "> (" << albumPlaylistAndNames.m_playlistName << ") with "
                             << albumPlaylistAndNames.m_playlist.size() << " elements\n";

    } else {
        logger(Level::warning) << "no current playlist set\n";
    }

    return albumPlaylistAndNames;

}

std::optional<std::vector<boost::uuids::uuid> > SimpleDatabase::getSongInPlaylistByName
(const std::string &_songName, const boost::uuids::uuid &albumUuid) {

    std::vector<boost::uuids::uuid> songList;

    auto infoList = getIdListOfItemsInPlaylistId(albumUuid);
    std::string songName = Common::str_tolower(_songName);

    for (const auto& infoElem : infoList) {
        if (infoElem.getNormalizedTitle() == songName) {
            songList.push_back(infoElem.uid);
        }
    }

    if (songList.empty())
        return std::nullopt;

    return songList;
}

std::optional<std::vector<char> > SimpleDatabase::getCover(const boost::uuids::uuid &uid) {

    auto& coverElement = m_id3Repository.getCover(uid);
    if (coverElement.rawData.size()>0) {
        logger(LoggerFramework::Level::debug) << "found cover for cover id <" << boost::uuids::to_string(uid) << "> in database\n";
        return coverElement.rawData;
    }
    else
        logger(LoggerFramework::Level::debug) << "NO cover in database found for cover id <" << uid << ">\n";
    return std::nullopt;
}

std::optional<std::string> SimpleDatabase::getFileFromUUID(boost::uuids::uuid &uuid) {
    auto id3Info = m_id3Repository.search(uuid, SearchItem::uid, SearchAction::uniqueId);
    if (id3Info.size() == 1) {
        // test if this is a local file
        logger(Level::warning) << "requested uuid <" << uuid << "> found url: "<<id3Info[0].urlAudioFile <<"\n";
        auto filename = id3Info[0].urlAudioFile;
        if (filename.substr(0,ServerConstant::fileprefix.length()) == ServerConstant::fileprefix) {
            return std::optional<std::string>(filename);
        }
        if (filename.substr(0,ServerConstant::httpprefix.length()) == ServerConstant::httpprefix ||
                filename.substr(0,ServerConstant::httpsprefix.length()) == ServerConstant::httpsprefix) {
            return std::optional<std::string>(filename);
        }

        logger(Level::warning) << "requested uuid <" << uuid << "> has no file prefix\n";
        return std::nullopt;
    }
    logger(Level::warning) << "requested uuid <" << uuid << "> not found\n";
    return std::nullopt;
}

std::optional<std::string> SimpleDatabase::getM3UPlaylistFromUUID(boost::uuids::uuid &uuid) {
    return m_playlistContainer.createvirtual_m3u(uuid);
}

void SimpleDatabase::addSingleSongToAlbumPlaylist(const boost::uuids::uuid &songId) {
    if (auto info = m_id3Repository.getId3InfoByUid(songId)) {
        logger(Level::info) << "found file <"<<info->title_name << "/"<<info->album_name<<"> to add\n";
        if (auto pl_uid = getTemporalPlaylistByName(*info)) {
            if (auto pl_ref = m_playlistContainer.getPlaylistByUID(*pl_uid)) {
                auto& pl = pl_ref->get();
                logger(Level::info) << "playlist (" << pl.getUniqueID()
                                    << ") to add found/created: " << pl.getName()
                                    << "/" << pl.getPerformer() << "\n";
                auto uuid = info->uid;
                pl.setCover(info->urlCoverFile);
                pl.addToList(std::move(uuid));
            }

        } else {
            logger(Level::warning) << "no playlist with the album name found or cannot be generated\n";
        }
    } else {
        logger(Level::warning) << "no song with the given ID\n";
    }
}
