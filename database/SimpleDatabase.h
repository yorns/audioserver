#ifndef SERVER_SIMPLEDATABASE_H
#define SERVER_SIMPLEDATABASE_H

#include <vector>
#include <string>
#include <variant>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
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
#include "credential.h"

namespace Database {

class SimpleDatabase {

    PlaylistContainer m_playlistContainer;
    Id3Repository m_id3Repository;
    Credential m_credentials;

public:

    SimpleDatabase(bool enableCache) :m_id3Repository(enableCache) {}

    void loadDatabase();
    bool writeChangedPlaylists();

    std::vector<Id3Info> searchAudioItems(const std::string &what, SearchItem item, SearchAction action);

    std::vector<Id3Info> searchAudioItems(const boost::uuids::uuid &what, SearchItem item, SearchAction action);

    std::vector<Id3Info> searchAudioItems(std::string_view what, SearchItem item, SearchAction action) {
        return searchAudioItems(std::string(what), item, action);
    }

    std::vector<Id3Info> searchAudioItems(const char* what, SearchItem item, SearchAction action) {
        return searchAudioItems(std::string(what), item, action);
    }

    std::vector<Playlist> searchPlaylistItems(const std::string &what, SearchAction action = SearchAction::exact) {
        return  m_playlistContainer.searchPlaylists(what, action);
    }

    std::vector<Playlist> searchPlaylistItems(const std::string_view &what, SearchAction action = SearchAction::exact) {
        return  m_playlistContainer.searchPlaylists(std::string(what), action);
    }

    std::vector<Playlist> searchPlaylistItems(const boost::uuids::uuid &what, SearchAction action = SearchAction::exact) {
        return  m_playlistContainer.searchPlaylists(what, action);
    }

    std::optional<boost::uuids::uuid> createPlaylist(const std::string &name, Database::Persistent persistent);

    bool addToPlaylistUID(const boost::uuids::uuid &playlistUid, boost::uuids::uuid &&uid);
    bool addToPlaylistName(const std::string &playlistName, std::string &&uniqueID);
    bool addNewAudioFileUniqueId(const Common::FileNameType &uniqueID);

    std::optional<Playlist> getPlaylistByName(const std::string& playlistName);
    std::optional<std::vector<boost::uuids::uuid>> getPlaylistByUID(const boost::uuids::uuid& playlistUniqueId);

    bool setCurrentPlaylistUniqueId(boost::uuids::uuid uniqueID);

    std::optional<const boost::uuids::uuid> getCurrentPlaylistUniqueID();

    std::vector<std::pair<std::string, boost::uuids::uuid>> getAllPlaylists();

    std::optional<boost::uuids::uuid> convertPlaylist(const std::string& name);
    std::optional<std::string> convertPlaylist(const boost::uuids::uuid& name);

    std::vector<Id3Info> getIdListOfItemsInPlaylistId(const boost::uuids::uuid& uniqueId);
    Common::AlbumPlaylistAndNames getAlbumPlaylistAndNames();

    std::optional<std::vector<boost::uuids::uuid>> getSongInPlaylistByName(const std::string& _songName, const boost::uuids::uuid& albumUuid);

    std::optional<std::vector<char>> getCover(const boost::uuids::uuid& uid);

    std::optional<std::string> getFileFromUUID(boost::uuids::uuid& uuid);

    std::optional<std::string> getM3UPlaylistFromUUID(boost::uuids::uuid& uuid) {
        return m_playlistContainer.createvirtual_m3u(uuid);
    }

    void addSingleSongToAlbumPlaylist(const boost::uuids::uuid& songId) {
        if (auto info = m_id3Repository.getId3InfoByUid(songId)) {
            logger(Level::info) << "found file <"<<info->title_name << "/"<<info->album_name<<"> to add\n";
            if (auto pl_uid = getTemporalPlaylistByName(*info)) {
                if (auto pl_ref = m_playlistContainer.getPlaylistByUID(*pl_uid)) {
                    auto& pl = pl_ref->get();
                    logger(Level::info) << "playlist (" << pl.getUniqueID()
                                        << ") to add found/created: " << pl.getName()
                                        << "/" << pl.getPerformer() << "\n";
                    auto uuid = info->uid;
                    //if (pl.getCover().empty())
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

    std::optional<boost::uuids::uuid> getTemporalPlaylistByName(const Id3Info &name);

#ifdef WITH_UNITTEST
    bool testInsert(Id3Info&& info) { return m_id3Repository.add(std::move(info)); }
#endif

    std::optional<std::string> passwordFind(const std::string& name) {
        logger(Level::info) << "SimpleDatabase::passwordFind\n";
        return m_credentials.passwordFind(name); }

};

}

#endif //SERVER_SIMPLEDATABASE_H
