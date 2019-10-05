#ifndef SERVER_SIMPLEDATABASE_H
#define SERVER_SIMPLEDATABASE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <boost/beast.hpp>
#include <boost/filesystem.hpp>
#include "common/Extractor.h"
#include "common/Constants.h"
#include "common/NameGenerator.h"
#include "Id3Info.h"
#include "id3TagReader.h"

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

    struct FileNameType {
        std::string name;
        std::string extension;
    };
    std::vector<FileNameType> getAllFilesInDir(const std::string &dirPath) {

        // Create a vector of string
        std::vector<FileNameType> listOfFiles;
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
                        listOfFiles.push_back({iter->path().stem().string(), iter->path().extension().string()});
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
    SimpleDatabase() = default;

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

    std::vector<Id3Info> findAlbum(const std::string& what) {
        // this only carries album name and picture ID

        std::vector<Id3Info> findData;

        auto isnew = [&findData](const std::string &albumName) -> bool {
            return std::find_if(std::begin(findData), std::end(findData),
                             [&albumName](const Id3Info &info) {
                                 return info.album_name == albumName;
                             })
                   == findData.end();
        };

        std::for_each(std::begin(simpleDatabase), std::end(simpleDatabase),
                      [&what, &findData, &isnew](const Id3Info &info) {
                          if (info.album_name.find(what) != std::string::npos)
                              if (isnew(info.album_name)) {
                                  Id3Info _info {info};
                                  _info.titel_name = "";
                                  _info.all_tracks_no = 0;
                                  _info.track_no = 0;
                                  findData.emplace_back(_info);
                              }
                      });

        return findData;
    }

    void emplace_back(Id3Info &&info) {
        simpleDatabase.emplace_back(info);
    }

    void addToDatabase(const std::string& uniqueId, const std::string& cover) {
        id3TagReader id3Reader;
        simpleDatabase.emplace_back(id3Reader.getInfo(uniqueId, cover));
    }

    void loadDatabase(const std::string &mp3Directory, const std::string imgDirectory) {

        std::vector<FileNameType> filelist = getAllFilesInDir(mp3Directory);
        std::vector<FileNameType> imglist = getAllFilesInDir(imgDirectory);

        auto getImageFileOf = [&imglist](const std::string& name) -> FileNameType {
            auto it = std::find_if(std::begin(imglist), std::end(imglist),
                                   [&name](const FileNameType& fileName){ return fileName.name == name; });
            if (it != imglist.end())
                return *it;
            return {ServerConstant::unknownCoverFile.to_string(),ServerConstant::unknownCoverExtension.to_string()};
        };

        for (auto& file : filelist) {
            auto imgFile = getImageFileOf(file.name);
            addToDatabase(file.name, imgDirectory + "/" + imgFile.name + imgFile.extension);
        }
    }

    bool readPlaylist(const FileNameType& filename) {
        std::vector<std::string> itemList;
        std::string line;
        std::string playlistNameHR;
        std::string playlistNameCRYPTIC;

        std::string fullFilename =
                ServerConstant::playlistRootPath.to_string() + "/" + filename.name + filename.extension;
        std::ifstream stream(fullFilename);

        playlistNameCRYPTIC = filename.name;

        std::cerr << "reading filename <"<<fullFilename<<">\n";

        if (!stream.good())
            return false;

        // read human readable playlist name
        if (stream.good() && std::getline(stream, line)) {
            if (line.length() > 2 && line[0] == '#' && line[1] == ' ') {
                playlistNameHR = line.substr(2);
            } else {
                filesys::path fullName(line);
                if (!fullName.stem().empty()) {
                    itemList.emplace_back(fullName.stem().string());
                    std::cerr << "  reading: <" << fullName.stem() << ">\n";
                }
            }
        }

        std::cerr << "playlist name: "<<playlistNameCRYPTIC<<"\n";

        while (stream.good() && std::getline(stream, line)) {
            filesys::path fullName(line);
            if (!fullName.stem().empty()) {
                itemList.emplace_back(fullName.stem().string());
                std::cerr << "  reading: <" << fullName.stem() << ">\n";
            }
        }

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

        std::vector<FileNameType> filelist = getAllFilesInDir(playlistDirectory);

        for (auto& it : filelist)
            readPlaylist(it);

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

    std::string getHumanReadableName(const std::string& crypticID) {

        auto item = std::find_if(std::begin(playlist), std::end(playlist), [&crypticID](const auto &elem) {
            return elem.first == crypticID;
        });

        if (item != playlist.end())
            return item->second.internalPlaylistName;

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
