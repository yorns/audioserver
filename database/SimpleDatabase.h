#ifndef SERVER_SIMPLEDATABASE_H
#define SERVER_SIMPLEDATABASE_H

#include <vector>
#include <string>
#include "common/Extractor.h"
#include "common/Constants.h"
#include "common/filesystemadditions.h"
#include "common/NameGenerator.h"
#include "id3tagreader/Id3Info.h"
#include "database/playlistcontainer.h"
#include "database/NameType.h"
#include "common/FileType.h"
#include "searchitem.h"
#include "searchaction.h"
#include "id3repository.h"

namespace Database {

class SimpleDatabase {

    PlaylistContainer m_playlistContainer;
    Id3Repository m_id3Repository;

public:
    SimpleDatabase() = default;

    void loadDatabase( ) {
       m_id3Repository.read();
       m_playlistContainer.readPlaylists();
    }

    std::optional<std::vector<Id3Info>> search(const std::string &what, SearchItem item,
                                               SearchAction action = SearchAction::exact,
                                               Common::FileType fileType = Common::FileType::Audio) {
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

        case Common::FileType::Covers: {
            logger(LoggerFramework::Level::warning) << "no search for cover IDs available\n";
            return std::nullopt;
        }

        case Common::FileType::Html: {
            logger(LoggerFramework::Level::warning) << "no search for html data available\n";
            return std::nullopt;
        }

        }
        return std::nullopt;
    }

    std::optional<std::string> createPlaylist(const std::string &name, Database::Persistent persistent) {

        if (!m_playlistContainer.isUniqueName(name))
            return std::nullopt;

        const auto newPlaylistUniqueID =
                Common::NameGenerator::create(Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Playlist),".m3u");
        Playlist newPlaylist(newPlaylistUniqueID.unique_id, persistent);
        newPlaylist.setName(name);

        m_playlistContainer.addPlaylist(std::move(newPlaylist));

        return newPlaylistUniqueID.unique_id;
    }

    bool addToPlaylistName(const std::string &playlistName, std::string &&uniqueID) {
        return m_playlistContainer.addItemToPlaylistName(playlistName, std::move(uniqueID));
    }

    bool addToPlaylistUID(const std::string &playlistUniqueID, std::string&& uniqueID) {
        return m_playlistContainer.addItemToPlaylistUID(playlistUniqueID, std::move(uniqueID));
    }

    bool writeChangedPlaylists() {
        return m_playlistContainer.writeChangedPlaylists();
    }

    std::optional<std::string> convertPlaylist(const std::string& name, NameType nameType) {
        return m_playlistContainer.convertName(name, nameType);
    }

    std::optional<const std::vector<std::string>> getPlaylistByName(const std::string& playlistName) const {
        return m_playlistContainer.getPlaylistByName(playlistName);
    }

    std::optional<const std::vector<std::string>> getPlaylistByUID(const std::string& playlistName) const {
        return m_playlistContainer.getPlaylistByUID(playlistName);
    }

    bool setCurrentPlaylistUniqueId(const std::string& uniqueID) {
        return m_playlistContainer.setCurrentPlaylist(uniqueID);
    }

    std::optional<const std::string> getCurrentPlaylistUniqueID() {
        return m_playlistContainer.getCurrentPlaylistUniqueID();
    }

    std::vector<std::pair<std::string, std::string>> getAllPlaylists() {
        std::vector<std::pair<std::string, std::string>> list;

        return m_playlistContainer.getAllPlaylists();
    }

    bool addNewAudioFileUniqueId(const std::string& uniqueID) {
        return m_id3Repository.add(uniqueID);
    }

    typedef std::tuple<const std::vector<std::string>, const std::string, const std::string> GetAlbumPlaylistAndNamesType;

    std::vector<Id3Info> getIdListOfItemsInPlaylistId(const std::string& uniqueId) {
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

    GetAlbumPlaylistAndNamesType getAlbumPlaylistAndNames() {
        if (auto playlistNameOpt = m_playlistContainer.getCurrentPlaylist()) {
            std::string playlistUniqueId = playlistNameOpt->getUniqueID();
            std::string playlistName = playlistNameOpt->getName();
            std::vector<std::string> playlist = playlistNameOpt->getPlaylist();
            logger(Level::debug) << "Album playlist found for <" << playlistUniqueId
                                 << "> (" << playlistName << ") with " << playlist.size()
                                 << " elements\n";
            return {playlist, playlistUniqueId, playlistName};
        }

        return {{},"",""};

    }

#ifdef WITH_UNITTEST
    bool testInsert(Id3Info&& info) { return m_id3Repository.add(std::move(info)); }
#endif

};

}

#endif //SERVER_SIMPLEDATABASE_H
