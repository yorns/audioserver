#ifndef SERVER_SIMPLEDATABASE_H
#define SERVER_SIMPLEDATABASE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <boost/beast.hpp>
#include <boost/filesystem.hpp>
#include "common/Extractor.h"
#include "common/Constants.h"
//#include "common/NameGenerator.h"
#include "Id3Info.h"
#include "id3TagReader.h"

namespace filesys =  boost::filesystem;
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

class SimpleDatabase {

    struct PlaylistContainer {
        std::string internalPlaylistName;
        std::vector<std::string> Playlist;
        bool changed{false};

        PlaylistContainer()
        {}
        PlaylistContainer(const std::string& _internalPlaylistName, bool _changed = false)
            : internalPlaylistName(_internalPlaylistName), changed(_changed)
        {}
        PlaylistContainer(const std::string& _internalPlaylistName, std::vector<std::string>&& _playlist, bool _changed = false)
            : internalPlaylistName(_internalPlaylistName), Playlist(std::move(_playlist)),changed(_changed)
        {}

    };

    std::vector<Id3Info> m_simpleDatabase;
    std::unordered_map<std::string, PlaylistContainer> m_playlist;
    std::unordered_map<std::string, PlaylistContainer> m_playlistAlbum;

    struct FileNameType {
        std::string name;
        std::string extension;
    };
    std::vector<FileNameType> getAllFilesInDir(const std::string &dirPath);

    bool writeChangedPlaylists(const std::string &playlistDirectory,
                               const std::unordered_map<std::string, PlaylistContainer>& playlist);

    bool isPlaylist(const std::string &playlistName,
                    const std::unordered_map<std::string, PlaylistContainer>& playlist);

    std::string getNameFromHumanReadable(const std::string& humanReadablePlaylist,
                                         const std::unordered_map<std::string, PlaylistContainer>& playlist);

    bool isPlaylistID(const std::string &playlistID,
                      const std::unordered_map<std::string, PlaylistContainer>& playlist);

    std::vector<std::string> showPlaylist(const std::string& playlistName,
                                          const std::unordered_map<std::string, PlaylistContainer>& playlist);

    void emplace_back(Id3Info &&info);

    boost::optional<Id3Info> getEntryOnId(const std::string& id);
public:
    SimpleDatabase() = default;

    enum class DatabaseSearchType {
        unknown,
        titel,
        album,
        interpret,
        uid,
        overall
    };

    std::string getHumanReadableName(const std::string& crypticID,
                                     const std::unordered_map<std::string, PlaylistContainer>& playlist) {

        auto item = std::find_if(std::begin(playlist), std::end(playlist), [&crypticID](const auto &elem) {
            return elem.first == crypticID;
        });

        if (item != std::end(playlist))
            return item->second.internalPlaylistName;

        return "";
    }

    std::string getHumanReadableName(const std::string& crypticID) {

        auto humanReadableName = getHumanReadableName(crypticID, m_playlist);
        if (!humanReadableName.empty())
            return humanReadableName;

        humanReadableName = getHumanReadableName(crypticID, m_playlistAlbum);
        if (!humanReadableName.empty())
            return humanReadableName;

        return "";
    }


    std::vector<Id3Info> findInDatabase(const std::string &what, DatabaseSearchType type);

    std::vector<Id3Info> findAlbum(const std::string& what);

    void addToDatabase(const std::string& uniqueId, const std::string& cover);

    void loadDatabase(const std::string &mp3Directory, const std::string imgDirectory);

    bool readPlaylist(const FileNameType& filename);

    bool writeChangedPlaylists();

    void loadAllPlaylists(const std::string &playlistDirectory);

    std::string createPlaylist(const std::string &name);

    std::string createAlbumPlaylistTmp(const std::string &album);

    std::string getNameFromHumanReadable(const std::string& humanReadablePlaylist);

    bool isPlaylist(const std::string &playlistName);

    bool addToPlaylist(const std::string &playlistName, const std::string &name);

    bool isPlaylistID(const std::string &playlistID);

    bool addToPlaylistID(const std::string &ID, const std::string &name);

    std::vector<std::pair<std::string, std::string>> showAllPlaylists();

    std::vector<std::string> showPlaylist(const std::string& playlistName);

};

#endif //SERVER_SIMPLEDATABASE_H
