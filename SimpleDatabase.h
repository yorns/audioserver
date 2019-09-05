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
        uid,
        overall
    };

    std::vector<Id3Info> findInDatabase(const std::string &what, DatabaseSearchType type) {

        std::vector<Id3Info> findData;

        if (type == DatabaseSearchType::uid) {
            std::for_each(std::begin(simpleDatabase), std::end(simpleDatabase),
                          [&what, &findData](const Id3Info &info) {
                              if (info.uid == what)
                                  findData.push_back(info);
                          });
        }

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

        if (urlInfo && urlInfo->command == ServerConstant::AccessPoints::database) {

            std::cerr << "find request with <" << urlInfo->parameter << "> of value <" << urlInfo->value << ">\n";
            boost::beast::error_code ec;
            http::string_body::value_type body;

            DatabaseSearchType type{DatabaseSearchType::unknown};

            if (urlInfo->parameter == ServerConstant::Parameter::Database::titel) {
                type = DatabaseSearchType::titel;
            }
            if (urlInfo->parameter == ServerConstant::Parameter::Database::album) {
                type = DatabaseSearchType::album;
            }
            if (urlInfo->parameter == ServerConstant::Parameter::Database::interpret) {
                type = DatabaseSearchType::interpret;
            }
            if (urlInfo->parameter == ServerConstant::Parameter::Database::overall) {
                type = DatabaseSearchType::overall;
            }
            if (urlInfo->parameter == ServerConstant::Parameter::Database::uid) {
                type = DatabaseSearchType::uid;
            }

            if (type != DatabaseSearchType::unknown) {
                auto list = findInDatabase(urlInfo->value, type);

                // convert to json
                nlohmann::json json;
                nlohmann::json jsonArray;

                std::for_each(std::begin(list), std::end(list), [&jsonArray](const Id3Info &info) {
                    nlohmann::json jsonListEntry;
                    jsonListEntry[ServerConstant::Parameter::Database::titel.to_string()] = info.titel_name;
                    jsonListEntry[ServerConstant::Parameter::Database::album.to_string()] = info.album_name;
                    jsonListEntry[ServerConstant::Parameter::Database::interpret.to_string()] = info.performer_name;
                    jsonListEntry[ServerConstant::Parameter::Database::trackNo.to_string()] = info.track_no;
                    jsonListEntry[ServerConstant::Parameter::Database::uid.to_string()] = info.uid;
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
        std::string playlistNameHR;
        std::string playlistNameCRYPTIC;
        std::ifstream stream(filename);

        filesys::path p {filename};
        playlistNameCRYPTIC = p.stem().string();

        std::cerr << "reading filename <"<<playlistNameCRYPTIC<<">\n";

        if (!stream.good())
            return false;

        if (stream.good() && std::getline(stream, line)) {
            if (line.length() > 2 && line[0] == '#' && line[1] == ' ') {
                playlistNameHR = line.substr(2);
            } else {
                filesys::path fullName(line);
                itemList.emplace_back(fullName.stem().string());
                std::cerr << "  reading: " << fullName.stem() << "\n";
            }
        }

        std::cerr << "playlist name: "<<playlistNameCRYPTIC<<"\n";

        while (stream.good() && std::getline(stream, line)) {
            filesys::path fullName(line);
            itemList.emplace_back(fullName.stem().string());
            std::cerr << "  reading: " << fullName.stem() << "\n";
        }

        //PlaylistContainer container{};
        playlist[playlistNameCRYPTIC] = PlaylistContainer{.internalPlaylistName=playlistNameHR, .Playlist = itemList};
        return true;
    }

    bool writeChangedPlaylists(const std::string &playlistDirectory) {

        bool success{true};
        for (const auto &elem : playlist) {
            std::cerr << "writeChangedPlaylist <"<<elem.first<< "> realname " << elem.second.internalPlaylistName;
            if (elem.second.changed) {
                std::cerr << " - try write ... ";
                std::string filename = playlistDirectory + "/" + elem.first + ".m3u";

                std::ofstream ofs{filename};

                if (ofs.good()) {
                    std::cerr << " done \n";

                    ofs << "# " << elem.second.internalPlaylistName << "\n";

                    for (const auto &entry : elem.second.Playlist) {
                        ofs << "../" << ServerConstant::fileRootPath << "/" << entry << ".mp3\n";
                    }
                } else {
                    std::cerr << " failed \n";
                    success = false;
                }
            }
            else {
                std::cerr << "not changed\n";
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

    std::string getNameFromHumanReadable(const std::string& humanReadablePlaylist) {

        auto item = std::find_if(std::begin(playlist), std::end(playlist), [&humanReadablePlaylist](const auto &elem) {
            return elem.second.internalPlaylistName == humanReadablePlaylist;
        });

        if (item != playlist.end())
            return item->first;

        return "";
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
            item->second.changed = true;
            return true;
        }
        return false;
    }

    bool isPlaylistID(const std::string &playlistID) {

        auto item = std::find_if(std::begin(playlist), std::end(playlist), [&playlistID](const auto &elem) {
            return elem.first == playlistID;
        });

        return item != playlist.end();
    }

    bool addToPlaylistID(const std::string &ID, const std::string &name) {
        auto item = std::find_if(std::begin(playlist), std::end(playlist),
                                 [&ID](const auto &elem) { return elem.first == ID; });
        if (item != playlist.end()) {
            item->second.Playlist.emplace_back(name);
            item->second.changed = true;
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
                                 [&playlistName](const auto &elem) {
                                     return elem.first == playlistName; });

        std::cout << "Playlist: " << playlistName <<"\n";
        if (item != playlist.end()) {
            list = item->second.Playlist;
            for (auto i : list)
                std::cout << " -> "<<i<<"\n";
        }
        else {
            std::cout << "empty\n";
        }



        return list;
    }

};

#endif //SERVER_SIMPLEDATABASE_H
