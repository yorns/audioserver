#include "SimpleDatabase.h"
#include "playlist.h"
#include <variant>

using namespace Database;

void SimpleDatabase::loadDatabase() {

    m_id3Repository.read();
    m_playlistContainer.readPlaylistsM3U();
    m_playlistContainer.readPlaylistsJson([this](std::string uid, std::vector<char>&& data, std::size_t hash){ m_id3Repository.addCover(uid, std::move(data), hash); });
    m_playlistContainer.insertAlbumPlaylists(m_id3Repository.extractAlbumList());

}


std::vector<Id3Info> SimpleDatabase::searchAudioItems(const std::string &what, SearchItem item, SearchAction action) {
        return m_id3Repository.search(what, item, action);
}

std::optional<std::string> SimpleDatabase::createPlaylist(const std::string &name, Persistent persistent) {

    if (!m_playlistContainer.isUniqueName(name))
        return std::nullopt;

    const auto newPlaylistUniqueID =
            Common::NameGenerator::create(Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::PlaylistM3u),".m3u");
    Playlist newPlaylist(newPlaylistUniqueID.unique_id, ReadType::isM3u, persistent);
    newPlaylist.setName(name);

    m_playlistContainer.addPlaylist(std::move(newPlaylist));

    return newPlaylistUniqueID.unique_id;
}

bool SimpleDatabase::addToPlaylistName(const std::string &playlistName, std::string &&uniqueID) {
    return m_playlistContainer.addItemToPlaylistName(playlistName, std::move(uniqueID));
}

bool SimpleDatabase::addToPlaylistUID(const std::string &playlistUniqueID, std::string &&uniqueID) {
    return m_playlistContainer.addItemToPlaylistUID(playlistUniqueID, std::move(uniqueID));
}

bool SimpleDatabase::writeChangedPlaylists() {
    return m_playlistContainer.writeChangedPlaylists();
}

std::optional<std::string> SimpleDatabase::convertPlaylist(const std::string &name, NameType nameType) {
    return m_playlistContainer.convertName(name, nameType);
}

std::optional<const std::vector<std::string> > SimpleDatabase::getPlaylistByName(const std::string &playlistName) const {
    return m_playlistContainer.getPlaylistByName(playlistName);
}

std::optional<const std::vector<std::string> > SimpleDatabase::getPlaylistByUID(const std::string &playlistName) const {
    return m_playlistContainer.getPlaylistByUID(playlistName);
}

bool SimpleDatabase::setCurrentPlaylistUniqueId(const std::string &uniqueID) {
    return m_playlistContainer.setCurrentPlaylist(uniqueID);
}

std::optional<const std::string> SimpleDatabase::getCurrentPlaylistUniqueID() {
    return m_playlistContainer.getCurrentPlaylistUniqueID();
}

std::vector<std::pair<std::string, std::string> > SimpleDatabase::getAllPlaylists() {
    std::vector<std::pair<std::string, std::string>> list;

    return m_playlistContainer.getAllPlaylists();
}

bool SimpleDatabase::addNewAudioFileUniqueId(const std::string &uniqueID) {
    if (m_id3Repository.add(uniqueID)) {
        m_playlistContainer.insertAlbumPlaylists(m_id3Repository.extractAlbumList());
        m_id3Repository.writeCache();
    }
    return true;
}

std::vector<Id3Info> SimpleDatabase::getIdListOfItemsInPlaylistId(const std::string &uniqueId) {
    std::vector<Id3Info> itemList;
    if (auto playlistNameOpt = m_playlistContainer.getPlaylistByUID(uniqueId)) {
        for (auto playlistItemUID : *playlistNameOpt) {
            auto id3 = m_id3Repository.search(playlistItemUID, SearchItem::overall, SearchAction::uniqueId);
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
