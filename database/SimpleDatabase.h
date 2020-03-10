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

typedef std::tuple<const std::vector<std::string>, const std::string, const std::string> GetAlbumPlaylistAndNamesType;

class SimpleDatabase {

    PlaylistContainer m_playlistContainer;
    Id3Repository m_id3Repository;

public:

    void loadDatabase();
    bool writeChangedPlaylists();

    std::optional<std::vector<Id3Info>> search(const std::string &what, SearchItem item,
                                               SearchAction action = SearchAction::exact,
                                               Common::FileType fileType = Common::FileType::Audio);

    std::optional<std::string> createPlaylist(const std::string &name, Database::Persistent persistent);
    bool addToPlaylistName(const std::string &playlistName, std::string &&uniqueID);
    bool addToPlaylistUID(const std::string &playlistUniqueID, std::string&& uniqueID);
    bool addNewAudioFileUniqueId(const std::string& uniqueID);

    std::optional<const std::vector<std::string>> getPlaylistByName(const std::string& playlistName) const;
    std::optional<const std::vector<std::string>> getPlaylistByUID(const std::string& playlistName) const;

    bool setCurrentPlaylistUniqueId(const std::string& uniqueID);
    std::optional<const std::string> getCurrentPlaylistUniqueID();

    std::vector<std::pair<std::string, std::string>> getAllPlaylists();

    std::optional<std::string> convertPlaylist(const std::string& name, NameType nameType);
    std::vector<Id3Info> getIdListOfItemsInPlaylistId(const std::string& uniqueId);
    GetAlbumPlaylistAndNamesType getAlbumPlaylistAndNames();

#ifdef WITH_UNITTEST
    bool testInsert(Id3Info&& info) { return m_id3Repository.add(std::move(info)); }
#endif

};

}

#endif //SERVER_SIMPLEDATABASE_H
