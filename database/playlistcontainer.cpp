#include "playlistcontainer.h"

#include <boost/filesystem.hpp>

#include "common/filesystemadditions.h"
#include "common/logger.h"
#include "common/NameGenerator.h"

using namespace Database;
using namespace Common;
using namespace LoggerFramework;

void PlaylistContainer::addPlaylist(Playlist &&playlist) {
    m_playlists.emplace_back(playlist);
}

bool PlaylistContainer::addItemToPlaylistName(const std::string &playlistName, std::string &&audioUniqueId) {
    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists), [&playlistName](const Playlist& item) {
        return item.getName() == playlistName;
    });

    if (it != std::end(m_playlists)) {
        it->addToList(std::move(audioUniqueId));
        return true;
    }

    return false;
}

bool PlaylistContainer::addItemToPlaylistUID(const std::string &playlistUniqueID, std::string &&audioUniqueId) {
    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists), [&playlistUniqueID](const Playlist& item) {
        return item.getUniqueID() == playlistUniqueID;
    });

    if (it != std::end(m_playlists)) {
        it->addToList(std::move(audioUniqueId));
        return true;
    }

    return false;
}

bool PlaylistContainer::removePlaylistUID(const std::string &playlistUniqueId) {
    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists),
                           [&playlistUniqueId](const Playlist& pl)
    { return (pl.getUniqueID() == playlistUniqueId); });

    if (it != std::end(m_playlists) ) {
        m_playlists.erase(it);
        return FileSystemAdditions::removeFile(FileType::PlaylistM3u, playlistUniqueId);
    }
    return false;
}

bool PlaylistContainer::removePlaylistName(const std::string &playlistName) {
    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists),
                           [&playlistName](const Playlist& pl)
    { return (pl.getName() == playlistName); });

    if (it != std::end(m_playlists) ) {
        auto playlistUniqueId { it->getUniqueID() };
        m_playlists.erase(it);
        return FileSystemAdditions::removeFile(FileType::PlaylistM3u, playlistUniqueId);
    }
    return false;
}

bool PlaylistContainer::writeChangedPlaylists() {

    bool success{true};

    logger(Level::debug) << "writing playlists if necessary\n";

    for (auto &elem : m_playlists) {
        if (!elem.write()) {
            logger(Level::warning) << "playlist file <"<<elem.getUniqueID()<<".m3u> could not be saved";
        }
    }

    system("sync");

    return success;
}

std::optional<std::string> PlaylistContainer::convertName(const std::string &name, NameType nametype) {

    switch (nametype) {
    case NameType::realName: {
        auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists), [&name] (const Playlist& playlist) {
            return playlist.getName() == name;
        });
        if (it != std::end(m_playlists))
            return it->getUniqueID();
        else
            return std::nullopt;
    }
    case NameType::uniqueID: {
        auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists), [&name] (const Playlist& playlist) {
            return playlist.getUniqueID() == name;
        });
        if (it != std::end(m_playlists))
            return it->getName();
        else
            return std::nullopt;
    }
    case NameType::none:
        return std::nullopt;
    }

    return std::nullopt;
}

bool PlaylistContainer::readPlaylistsM3U() {

    auto fileList = FileSystemAdditions::getAllFilesInDir(FileType::PlaylistM3u);

    for (auto file : fileList) {
        logger(Level::debug) << "reading m3u playlist <"<<file.name<<">\n";
        std::string fileName = FileSystemAdditions::getFullQualifiedDirectory(FileType::PlaylistM3u) + '/';
        fileName += file.name + file.extension;
        if (file.extension == ".m3u") {
            Playlist playlist(file.name, ReadType::isM3u, Persistent::isPermanent, Changed::isUnchanged);
            if (playlist.readM3u()) {
                m_playlists.emplace_back(playlist);
            }
            else
                logger(Level::warning) << "reading file <"<<file.name+file.extension <<"> failed\n";
        }
    }

    return true;
}

bool PlaylistContainer::readPlaylistsJson(std::function<void(std::string uid, std::vector<char>&& data, std::size_t hash)>&& coverInsert) {
    auto streamList = FileSystemAdditions::getAllFilesInDir(FileType::PlaylistJson);

    for (auto file : streamList) {
        logger(Level::debug) << "reading json playlist <"<<file.name<<">\n";
        std::string fileName = FileSystemAdditions::getFullQualifiedDirectory(FileType::PlaylistJson) + '/';
        fileName += file.name + file.extension;
        if (file.extension == ".json") {
            Playlist playlist(file.name, ReadType::isJson, Persistent::isPermanent, Changed::isConst);
            if (playlist.readJson(std::move(coverInsert))) {
                m_playlists.emplace_back(playlist);
            }
            else
                logger(Level::warning) << "reading file <"<<file.name+file.extension <<"> failed\n";
        }
    }

    return true;
}

bool PlaylistContainer::insertAlbumPlaylists(const std::vector<AlbumListEntry> &albumList) {

    for(auto& elem : albumList) {
        auto uniqueID = NameGenerator::create("", "");
        Playlist playlist(uniqueID.unique_id, ReadType::isM3u, Persistent::isTemporal);
        playlist.setName(elem.m_name);
        playlist.setPerformer(elem.m_performer);
        playlist.setCover(elem.m_coverId, elem.m_coverExtension);
        for(auto& uid : elem.m_playlist) {
            playlist.addToList(std::string(std::get<std::string>(uid)));
        }
        m_playlists.emplace_back(playlist);
    }

    return true;
}

std::optional<const std::vector<std::string>> PlaylistContainer::getPlaylistByName(const std::string &playlistName) const {
    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists),
                           [playlistName](const Playlist& elem) { return elem.getName() == playlistName; });

    if (it != std::end(m_playlists)) {
        return it->getUniqueIdPlaylist();
    }
    return std::nullopt;

}

std::optional<const std::vector<std::string>> PlaylistContainer::getPlaylistByUID(const std::string &uid) const {
    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists),
                           [uid](const Playlist& elem) { return elem.getUniqueID() == uid; });

    if (it != std::end(m_playlists)) {
        return it->getUniqueIdPlaylist();
    }
    return std::nullopt;
}

std::optional<Playlist> PlaylistContainer::getCurrentPlaylist() {

    if (m_currentPlaylist) {
        auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists),
                               [this](const Playlist& elem) { return elem.getUniqueID() == m_currentPlaylist.value(); });

        if (it != std::end(m_playlists)) {
            return *it;
        }
    }
    return std::nullopt;
}

std::optional<const std::string> PlaylistContainer::getCurrentPlaylistUniqueID() const {
    return m_currentPlaylist;
}

bool PlaylistContainer::setCurrentPlaylist(const std::string &currentPlaylistUniqueId) {
    if (std::find_if(std::begin(m_playlists), std::end(m_playlists),
                     [&currentPlaylistUniqueId](const Playlist& playlist) {
                     return (playlist.getUniqueID() == currentPlaylistUniqueId);})
            != std::end(m_playlists)) {
        m_currentPlaylist = currentPlaylistUniqueId;
        return true;
    }

    logger(Level::warning) << "playlist <" << currentPlaylistUniqueId << "> not found\n";
    return false;
}

void PlaylistContainer::unsetCurrentPlaylist() { m_currentPlaylist.reset(); }

bool PlaylistContainer::isUniqueName(const std::string &name) {
    if (std::find_if(std::begin(m_playlists), std::end(m_playlists),
                     [&name](const Playlist& playlist) { return playlist.getName() == name; })
            != std::end(m_playlists)) {
        return false;
    }
    return true;
}

std::vector<std::pair<std::string, std::string> > PlaylistContainer::getAllPlaylists() {
    std::vector<std::pair<std::string, std::string>> list;
    for(auto& elem : m_playlists) {
        list.push_back(std::make_pair(elem.getName(), elem.getUniqueID()));
    }
    return list;
}

std::vector<Playlist> PlaylistContainer::searchPlaylists(const std::string &what, SearchAction action) {

    std::vector<Playlist> playlist;

    switch(action) {
    case SearchAction::exact: {
        logger(Level::debug) << "exact seaching for string <" << what << ">\n";
        for(auto playlistItem{std::cbegin(m_playlists)}; playlistItem != std::cend(m_playlists); ++playlistItem) {
            if ( playlistItem->getName() == what ||
                 playlistItem->getPerformer() == what ||
                 playlistItem->getUniqueID() == what) {
                playlist.push_back(*playlistItem);
            }
        }
        break;
    }
    case SearchAction::alike: {
        auto whatList = Common::extractWhatList(what);

        std::string seperator;
        std::stringstream tmp;
        for(auto whatElem : whatList) {
            tmp << seperator << whatElem;
            seperator = ", ";
        }
        logger(Level::debug) << "alike seaching for string <" << what << "> (" << tmp.str() << ")\n";

        for(auto playlistItem{std::cbegin(m_playlists)}; playlistItem != std::cend(m_playlists); ++playlistItem) {
            bool found {true};
            for(auto whatElem : whatList) {
                logger(Level::debug) << "check against <"<<playlistItem->getNameLower()<<"> / <" << playlistItem->getPerformerLower() <<">\n";
                if (playlistItem->getNameLower().find(whatElem) == std::string::npos &&
                        playlistItem->getPerformerLower().find(whatElem) == std::string::npos) {
                    found = false;
                    break;
                }
            }
            if (found)
                playlist.push_back(*playlistItem);
        }
        break;
    }
    case SearchAction::uniqueId : {
        auto playlist_it = std::find_if(std::begin(m_playlists), std::end(m_playlists), [&what](const Playlist& playlist){ return playlist.getUniqueID() == what; });
        if (playlist_it != std::end(m_playlists))
            playlist.push_back(*playlist_it);
        break;
    }
    }
    logger(Level::debug) << "searchPlaylist size <"<<playlist.size()<<">\n";
    return playlist;
}
