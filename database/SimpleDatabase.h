#ifndef SERVER_SIMPLEDATABASE_H
#define SERVER_SIMPLEDATABASE_H

#include <vector>
#include <string>
#include <variant>
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
#include "common/generalPlaylist.h"

namespace Database {

class SimpleDatabase {

    PlaylistContainer m_playlistContainer;
    Id3Repository m_id3Repository;

public:

    SimpleDatabase(bool enableCache) :m_id3Repository(enableCache) {}

    void loadDatabase();
    bool writeChangedPlaylists();

    std::vector<Id3Info> searchAudioItems(const std::string &what, SearchItem item, SearchAction action);

    std::vector<Playlist> searchPlaylistItems(const std::string &what, SearchAction action = SearchAction::exact) {
        return  m_playlistContainer.searchPlaylists(what, action);
    }
    
    std::optional<std::string> createPlaylist(const std::string &name, Database::Persistent persistent);

    bool addToPlaylistName(const std::string &playlistName, std::string &&uniqueID);
    bool addToPlaylistUID(const std::string &playlistUniqueID, std::string&& uniqueID);
    bool addNewAudioFileUniqueId(const Common::FileNameType &uniqueID);

    std::optional<const std::vector<std::string>> getPlaylistByName(const std::string& playlistName) const;
    std::optional<const std::vector<std::string>> getPlaylistByUID(const std::string& playlistUniqueId) const;

    bool setCurrentPlaylistUniqueId(const std::string& uniqueID);
    std::optional<const std::string> getCurrentPlaylistUniqueID();

    std::vector<std::pair<std::string, std::string>> getAllPlaylists();

    std::optional<std::string> convertPlaylist(const std::string& name, NameType nameType);
    std::vector<Id3Info> getIdListOfItemsInPlaylistId(const std::string& uniqueId);
    Common::AlbumPlaylistAndNames getAlbumPlaylistAndNames();

    std::optional<std::vector<char>> getCover(const std::string& uid) {
        auto& coverElement = m_id3Repository.getCover(uid);
        if (coverElement.rawData.size()>0) {
            logger(LoggerFramework::Level::debug) << "found cover for cover id <" << uid << "> in database\n";
            return coverElement.rawData;
        }
        else
            logger(LoggerFramework::Level::debug) << "NO cover in database found for cover id <" << uid << ">\n";
        return std::nullopt;
    }

#ifdef WITH_UNITTEST
    bool testInsert(Id3Info&& info) { return m_id3Repository.add(std::move(info)); }
#endif

};

}

#endif //SERVER_SIMPLEDATABASE_H
