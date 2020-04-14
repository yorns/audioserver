#include "SimpleDatabase.h"
#include "common/generalPlaylist.h"

void Database::SimpleDatabase::loadDatabase() {
    m_id3Repository.read();
    m_playlistContainer.readPlaylists();
}

std::optional<std::vector<Id3Info> > Database::SimpleDatabase::search(const std::string &what, Database::SearchItem item, Database::SearchAction action, Common::FileType fileType) {
    switch(fileType) {

    case Common::FileType::Audio: {
        return m_id3Repository.search(what, item, action);
    }

    case Common::FileType::Playlist: {
        if (action == SearchAction::alike) {
            logger(LoggerFramework::Level::warning) << "searching playlist \"allike\" is not available\n";
        }
        auto uidList = m_playlistContainer.getPlaylistByName(what);
        std::optional<std::vector<Id3Info>> list = {};
        try {
            for(auto& elem : uidList.value()) {
                auto item = m_id3Repository.search(elem, SearchItem::uid);
                for(auto& single : item.value())
                    list->push_back(single);
            }
        } catch (std::exception& ) { return std::nullopt; }
        return list;
    }

    case Common::FileType::Covers:
    case Common::FileType::CoversRelative: {
        logger(LoggerFramework::Level::warning) << "no search for cover IDs available\n";
        return std::nullopt;
    }

    case Common::FileType::Html: {
        logger(LoggerFramework::Level::warning) << "no search for html data available\n";
        return std::nullopt;
    }

    case Common::FileType::Stream: {
        logger(LoggerFramework::Level::warning) << "TODO: no search for Streams available\n";
        return std::nullopt;
    }
    }
    return std::nullopt;
}

std::optional<std::string> Database::SimpleDatabase::createPlaylist(const std::string &name, Database::Persistent persistent) {

    if (!m_playlistContainer.isUniqueName(name))
        return std::nullopt;

    const auto newPlaylistUniqueID =
            Common::NameGenerator::create(Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Playlist),".m3u");
    Playlist newPlaylist(newPlaylistUniqueID.unique_id, persistent);
    newPlaylist.setName(name);

    m_playlistContainer.addPlaylist(std::move(newPlaylist));

    return newPlaylistUniqueID.unique_id;
}

bool Database::SimpleDatabase::addToPlaylistName(const std::string &playlistName, std::string &&uniqueID) {
    return m_playlistContainer.addItemToPlaylistName(playlistName, std::move(uniqueID));
}

bool Database::SimpleDatabase::addToPlaylistUID(const std::string &playlistUniqueID, std::string &&uniqueID) {
    return m_playlistContainer.addItemToPlaylistUID(playlistUniqueID, std::move(uniqueID));
}

bool Database::SimpleDatabase::writeChangedPlaylists() {
    return m_playlistContainer.writeChangedPlaylists();
}

std::optional<std::string> Database::SimpleDatabase::convertPlaylist(const std::string &name, Database::NameType nameType) {
    return m_playlistContainer.convertName(name, nameType);
}

std::optional<const std::vector<std::string> > Database::SimpleDatabase::getPlaylistByName(const std::string &playlistName) const {
    return m_playlistContainer.getPlaylistByName(playlistName);
}

std::optional<const std::vector<std::string> > Database::SimpleDatabase::getPlaylistByUID(const std::string &playlistName) const {
    return m_playlistContainer.getPlaylistByUID(playlistName);
}

bool Database::SimpleDatabase::setCurrentPlaylistUniqueId(const std::string &uniqueID) {
    return m_playlistContainer.setCurrentPlaylist(uniqueID);
}

std::optional<const std::string> Database::SimpleDatabase::getCurrentPlaylistUniqueID() {
    return m_playlistContainer.getCurrentPlaylistUniqueID();
}

std::vector<std::pair<std::string, std::string> > Database::SimpleDatabase::getAllPlaylists() {
    std::vector<std::pair<std::string, std::string>> list;

    return m_playlistContainer.getAllPlaylists();
}

bool Database::SimpleDatabase::addNewAudioFileUniqueId(const std::string &uniqueID) {
    return m_id3Repository.add(uniqueID);
}

std::vector<Id3Info> Database::SimpleDatabase::getIdListOfItemsInPlaylistId(const std::string &uniqueId) {
    std::vector<Id3Info> itemList;
    if (auto playlistNameOpt = m_playlistContainer.getPlaylistByUID(uniqueId)) {
        for (auto playlistItemUID : *playlistNameOpt) {
            auto id3 = m_id3Repository.search(playlistItemUID, Database::SearchItem::uid);
            if (id3 && id3->size() == 1) {
                itemList.push_back(id3->front());
            }
        }
    }
    return itemList;
}

Common::AlbumPlaylistAndNames Database::SimpleDatabase::getAlbumPlaylistAndNames() {

    Common::AlbumPlaylistAndNames albumPlaylistAndNames;

    if (auto playlistNameOpt = m_playlistContainer.getCurrentPlaylist()) {
        albumPlaylistAndNames.m_playlistUniqueId = playlistNameOpt->getUniqueID();
        albumPlaylistAndNames.m_playlistName = playlistNameOpt->getName();
        const std::vector<std::string> uniqueIdPlaylist = playlistNameOpt->getUniqueIdPlaylist();
        for (auto& uniqueId : uniqueIdPlaylist) {
            auto item = m_id3Repository.getId3InfoByUid(uniqueId);
            if (item) {
                Common::PlaylistItem playlistItem;
                playlistItem.m_url = item->url;
                playlistItem.m_uniqueId = uniqueId;
                albumPlaylistAndNames.m_playlist.emplace_back(playlistItem);
            }
        }

        logger(Level::debug) << "Album playlist found for <" << albumPlaylistAndNames.m_playlistUniqueId
                             << "> (" << albumPlaylistAndNames.m_playlistName << ") with "
                             << albumPlaylistAndNames.m_playlist.size() << " elements\n";

    }

    return albumPlaylistAndNames;

}
