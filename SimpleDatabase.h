#ifndef SERVER_SIMPLEDATABASE_H
#define SERVER_SIMPLEDATABASE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <boost/beast.hpp>
#include "Id3Info.h"
#include "common/Extractor.h"
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>
#include "Id3Reader.h"
#include "common/Constants.h"
#include "common/NameGenerator.h"

namespace filesys =  boost::filesystem;
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

class SimpleDatabase {

    struct PlaylistContainer {
        std::string internalPlaylistName;
        std::vector<std::string> Playlist;
        bool changed{false};
    };

    std::vector<Id3Info> simpleDatabase;
    std::unordered_map<std::string, PlaylistContainer> playlist;

    std::vector<std::string> getAllFilesInDir(const std::string &dirPath) {

        // Create a vector of string
        std::vector<std::string> listOfFiles;
        try {
            // Check if given path exists and points to a directory
            if (filesys::exists(dirPath) && filesys::is_directory(dirPath)) {
                // Create a Directory Iterator object and points to the starting of directory
                filesys::directory_iterator iter(dirPath);

                // Create a Directory Iterator object pointing to end.
                filesys::directory_iterator end;

                // Iterate till end
                while (iter != end) {
                    // Check if current entry is a directory and if exists in skip list
                    if (!filesys::is_directory(iter->path())) {
                        // Add the name in vector
                        listOfFiles.push_back(iter->path().string());
                    }

                    boost::system::error_code ec;
                    // Increment the iterator to point to next entry in recursive iteration
                    iter.increment(ec);
                    if (ec) {
                        std::cerr << "Error While Accessing : " << iter->path().string() << " :: " << ec.message()
                                  << '\n';
                    }
                }
            }
        }
        catch (std::system_error &e) {
            std::cerr << "Exception :: " << e.what();
        }
        return listOfFiles;
    }

public:
    SimpleDatabase() {}

    enum class DatabaseSearchType {
        unknown,
        titel,
        album,
        interpret,
        overall
    };

    std::vector<Id3Info> findInDatabase(const std::string &what, DatabaseSearchType type) {

        std::vector<Id3Info> findData;

        std::for_each(std::begin(simpleDatabase), std::end(simpleDatabase),
                      [&what, &findData, type](const Id3Info &info) {
                          if (((type == DatabaseSearchType::titel || type == DatabaseSearchType::overall) &&
                               (info.titel_name.find(what) != std::string::npos)) ||
                              ((type == DatabaseSearchType::album || type == DatabaseSearchType::overall) &&
                               (info.album_name.find(what) != std::string::npos)) ||
                              ((type == DatabaseSearchType::interpret || type == DatabaseSearchType::overall) &&
                               (info.performer_name.find(what) != std::string::npos)))
                              findData.push_back(info);
                      });

        return findData;

    }

    void emplace_back(Id3Info &&info) {
        simpleDatabase.emplace_back(info);
    }

    boost::optional<std::tuple<std::string, DatabaseSearchType>> findInDatabaseToJson(const std::string &path) {

        boost::optional<std::tuple<std::string, DatabaseSearchType>> jsonList;

        /* is this a REST find request? */
        auto urlInfo = utility::Extractor::getUrlInformation(path);

        if (urlInfo && urlInfo->command == "find") {

            std::cerr << "find request with <" << urlInfo->parameter << "> of value <" << urlInfo->value << ">\n";
            boost::beast::error_code ec;
            http::string_body::value_type body;

            DatabaseSearchType type{DatabaseSearchType::unknown};

            if (urlInfo->parameter == "titel") {
                type = DatabaseSearchType::titel;
            }
            if (urlInfo->parameter == "album") {
                type = DatabaseSearchType::album;
            }
            if (urlInfo->parameter == "interpret") {
                type = DatabaseSearchType::interpret;
            }
            if (urlInfo->parameter == "overall") {
                type = DatabaseSearchType::overall;
            }

            if (type != DatabaseSearchType::unknown) {
                auto list = findInDatabase(urlInfo->value, type);

                // convert to json
                nlohmann::json json;
                nlohmann::json jsonArray;

                std::for_each(std::begin(list), std::end(list), [&jsonArray](const Id3Info &info) {
                    nlohmann::json jsonListEntry;
                    jsonListEntry["titel"] = info.titel_name;
                    jsonListEntry["album"] = info.album_name;
                    jsonListEntry["performer"] = info.performer_name;
                    jsonListEntry["track"] = info.track_no;
                    jsonListEntry["uuid"] = info.filename;
                    jsonArray.push_back(jsonListEntry);
                });

                json["list"] = jsonArray;
                jsonList = std::make_tuple(json.dump(2), type);
            } else {
                jsonList = std::make_tuple(std::string(), type);
            }
        }
        return jsonList;
    }


    void loadDatabase(const std::string &mp3Directory) {

        std::vector<std::string> filelist = getAllFilesInDir(mp3Directory);
        Id3Reader id3Reader;

        for (auto it{filelist.begin()}; it != filelist.end(); ++it) {
            std::cerr << " read <" << *it << ">\n";
            simpleDatabase.emplace_back(id3Reader.getInfo(*it));
        }
    }

    bool readPlaylist(std::string &filename) {
        std::vector<std::string> itemList;
        std::string line;
        std::string playlistName;
        std::ifstream stream(filename);

        playlistName = filename;

        if (!stream.good())
            return false;

        if (stream.good() && std::getline(stream, line)) {
            if (line.length() > 2 && line[0] == '#' && line[1] == ' ') {
                playlistName = line.substr(2);
            } else
                itemList.emplace_back(line);
        }

        while (stream.good() && std::getline(stream, line)) {
            itemList.emplace_back(line);
        }

        //PlaylistContainer container{};
        playlist[playlistName] = PlaylistContainer{.internalPlaylistName=playlistName, .Playlist = itemList};
        return true;
    }

    bool writeChangedPlaylists(const std::string &playlistDirectory) {

        bool success{true};
        for (const auto &elem : playlist) {
            if (elem.second.changed) {
                std::string filename = playlistDirectory + "/" + elem.first + ".m3u";

                std::ofstream ofs{filename};

                if (ofs.good()) {

                    ofs << "# " << elem.second.internalPlaylistName << "\n";

                    for (const auto &entry : elem.second.Playlist) {
                        ofs << ServerConstant::fileRootPath << "/" << entry << "\n";
                    }
                } else {
                    success = false;
                }
            }
        }

        return success;
    }

    void loadAllPlaylists(const std::string &playlistDirectory) {

        std::vector<std::string> filelist = getAllFilesInDir(playlistDirectory);

        for (auto it{filelist.begin()}; it != filelist.end(); ++it) {
            readPlaylist(*it);
        }

    }

    std::string createPlaylist(const std::string &name) {

        auto it = std::find_if(std::begin(playlist), std::end(playlist), [&name](const auto &it) {
            return (it.second.internalPlaylistName == name);
        });

        if (it != std::end(playlist))
            return "";

        std::string playlistName = NameGenerator::create("", "");
        playlist[playlistName] = PlaylistContainer{.internalPlaylistName=name, .Playlist=std::vector<std::string>{}, .changed=true};

        return playlistName;
    }

    bool isPlaylist(const std::string &playlistName) {

        auto item = std::find_if(std::begin(playlist), std::end(playlist), [&playlistName](const auto &elem) {
            return elem.second.internalPlaylistName == playlistName;
        });

        return item != playlist.end();
    }

    bool addToPlaylist(const std::string &playlistName, const std::string &name) {
        auto item = std::find_if(std::begin(playlist), std::end(playlist), [&playlistName](const auto &elem) {
            return elem.second.internalPlaylistName == playlistName;
        });
        if (item != playlist.end()) {
            item->second.Playlist.push_back(name);
            return true;
        }
        return false;
    }

    bool addToPlaylistID(const std::string &ID, std::string &name) {
        auto item = std::find_if(std::begin(playlist), std::end(playlist),
                                 [&ID](const auto &elem) { return elem.first == ID; });
        if (item != playlist.end()) {
            item->second.Playlist.emplace_back(name);
            return true;
        }
        return false;
    }

    std::vector<std::pair<std::string, std::string>> showAllPlaylists() {
        std::vector<std::pair<std::string, std::string>> list;

        std::for_each(std::begin(playlist), std::end(playlist), [&list](const auto &elem) {
            list.push_back(std::make_pair(elem.first, elem.second.internalPlaylistName));
        });

        return list;
    }

    std::vector<std::string> showPlaylist(const std::string& playlistName) {
        std::vector<std::string> list;

        auto item = std::find_if(std::begin(playlist), std::end(playlist),
                                 [&playlistName](const auto &elem) { return elem.second.internalPlaylistName == playlistName; });

        if (item != playlist.end()) {
            list = item->second.Playlist;
        }

        return list;
    }

};

#endif //SERVER_SIMPLEDATABASE_H
