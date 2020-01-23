#include "SimpleDatabase.h"
#include "common/NameGenerator.h"
#include "id3tagreader/id3TagReader.h"
#include <boost/filesystem.hpp>

namespace filesys =  boost::filesystem;

std::vector<SimpleDatabase::FileNameType> SimpleDatabase::getAllFilesInDir(const std::string &dirPath) {

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

bool SimpleDatabase::removeAllFilesInDir(const std::string &dirPath) {

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
                    filesys::remove(iter->path());
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
        return false;
    }
    return true;
}


bool SimpleDatabase::writeChangedPlaylists(const std::string &playlistDirectory,
                                           const std::unordered_map<std::string, SimpleDatabase::PlaylistContainer> &playlist) {

    bool success{true};
    for (const auto &elem : playlist) {
        std::cerr << "writeChangedPlaylist <" << elem.first << "> realname <" << elem.second.internalPlaylistName << ">";
        if (elem.second.changed) {
            std::cerr << " - try write ... ";
            std::string filename = playlistDirectory + "/" + elem.first + ".m3u";

            std::ofstream ofs{filename};

            if (ofs.good()) {
                ofs << "# " << elem.second.internalPlaylistName << "\n";

                for (const auto &entry : elem.second.Playlist) {
                    ofs << "../" << ServerConstant::audioPath << "/" << entry << ".mp3\n";
                }

                std::cerr << " done \n";

            } else {
                std::cerr << " failed \n";
                success = false;
            }
        } else {
            std::cerr << "not changed\n";
        }
    }

    system("sync");

    return success;
}

bool SimpleDatabase::isPlaylist(const std::string &playlistName,
                                const std::unordered_map<std::string, SimpleDatabase::PlaylistContainer> &playlist) {

    auto item = std::find_if(std::begin(playlist), std::end(playlist), [&playlistName](const auto &elem) {
        return elem.second.internalPlaylistName == playlistName;
    });

    return item != m_playlist.end();
}

std::string SimpleDatabase::getNameFromHumanReadable(const std::string &humanReadablePlaylist,
                                                     const std::unordered_map<std::string, SimpleDatabase::PlaylistContainer> &playlist) {

    auto item = std::find_if(std::begin(playlist), std::end(playlist), [&humanReadablePlaylist](const auto &elem) {
        return elem.second.internalPlaylistName == humanReadablePlaylist;
    });

    if (item != m_playlist.end())
        return item->first;

    return "";
}

bool SimpleDatabase::isPlaylistID(const std::string &playlistID,
                                  const std::unordered_map<std::string, SimpleDatabase::PlaylistContainer> &playlist) {

    auto item = std::find_if(std::begin(playlist), std::end(playlist), [&playlistID](const auto &elem) {
        return elem.first == playlistID;
    });

    return item != std::end(playlist);
}

std::vector<std::string> SimpleDatabase::showPlaylist(const std::string &playlistName,
                                                      const std::unordered_map<std::string, SimpleDatabase::PlaylistContainer> &playlist) {

    std::vector<std::string> list;

    auto item = std::find_if(std::begin(playlist), std::end(playlist),
                             [&playlistName](const auto &elem) {
                                 return elem.first == playlistName; });

    if (item != playlist.end()) {
        list = item->second.Playlist;
//        for (auto i : list)
//            std::cout << " -> " << i << "\n";
    }

    return list;
}

std::vector<Id3Info> SimpleDatabase::findInDatabase(const std::string &what, SimpleDatabase::DatabaseSearchType type) {

    std::vector<Id3Info> findData;

    if (type == DatabaseSearchType::uid) {
        std::for_each(std::begin(m_simpleDatabase), std::end(m_simpleDatabase),
                      [&what, &findData](const Id3Info &info) {
                          if (info.uid == what)
                              findData.push_back(info);
                      });
    }

    std::for_each(std::begin(m_simpleDatabase), std::end(m_simpleDatabase),
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

std::vector<Id3Info> SimpleDatabase::findAlbum(const std::string &what) {
    // this only carries album name and picture ID

    std::vector<Id3Info> findData;

    auto isnew = [&findData](const std::string &albumName) -> bool {
        return std::find_if(std::begin(findData), std::end(findData),
                            [&albumName](const Id3Info &info) {
                                return info.album_name == albumName;
                            })
               == findData.end();
    };

    std::for_each(std::begin(m_simpleDatabase), std::end(m_simpleDatabase),
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

    if (!what.empty()) {
        std::for_each(std::begin(m_simpleDatabase), std::end(m_simpleDatabase),
                      [&what, &findData, &isnew](const Id3Info &info) {
                          if (info.performer_name.find(what) != std::string::npos)
                              if (isnew(info.album_name)) {
                                  Id3Info _info {info};
                                  _info.titel_name = "";
                                  _info.all_tracks_no = 0;
                                  _info.track_no = 0;
                                  findData.emplace_back(_info);
                              }
                      });

    }
    return findData;
}

void SimpleDatabase::addToDatabase(const std::string &uniqueId, const std::string &cover) {
    id3TagReader id3Reader;
    if (auto id3info = id3Reader.getInfo(uniqueId, cover))
        m_simpleDatabase.emplace_back(*id3info);
}

void SimpleDatabase::addToDatabase(Id3Info&& info) {
   m_simpleDatabase.emplace_back(info);
}

void SimpleDatabase::loadDatabase(const std::string &mp3Directory, const std::string imgDirectory) {

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
        std::stringstream cover;
        cover << "/" << ServerConstant::coverPathWeb << "/" << imgFile.name << imgFile.extension;
        addToDatabase(file.name, cover.str());
    }
}

bool SimpleDatabase::readPlaylist(const SimpleDatabase::FileNameType &filename) {
    std::vector<std::string> itemList;
    std::string line;
    std::string playlistNameHR;
    std::string playlistNameCRYPTIC;

    std::string fullFilename =
            ServerConstant::playlistPath.to_string() + "/" + filename.name + filename.extension;
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

    m_playlist[playlistNameCRYPTIC] = PlaylistContainer(playlistNameHR, std::move(itemList));
    return true;
}

bool SimpleDatabase::writeChangedPlaylists() {
    std::stringstream playlistPath;
    std::stringstream albumListPath;
    playlistPath  << ServerConstant::base_path << "/" << ServerConstant::playlistPath;
    albumListPath << ServerConstant::base_path << "/" << ServerConstant::albumPlaylistDirectory;
    return (writeChangedPlaylists(playlistPath.str(), m_playlist) &&
            writeChangedPlaylists(albumListPath.str(), m_playlistAlbum));
}

void SimpleDatabase::removeTemporalPlaylists() {
    std::stringstream tmpListPath;
    tmpListPath  << ServerConstant::base_path << "/" << ServerConstant::albumPlaylistDirectory;

    removeAllFilesInDir(tmpListPath.str());
}

void SimpleDatabase::loadAllPlaylists(const std::string &playlistDirectory) {

    std::vector<FileNameType> filelist = getAllFilesInDir(playlistDirectory);

    for (auto& it : filelist)
        readPlaylist(it);

}

std::string SimpleDatabase::createPlaylist(const std::string &name) {

    auto it = std::find_if(std::begin(m_playlist), std::end(m_playlist), [&name](const auto &it) {
        return (it.second.internalPlaylistName == name);
    });

    if (it != std::end(m_playlist))
        return "";
    //    return it->first;

    auto playlistName = NameGenerator::create("", "");
    m_playlist[playlistName.unique_id] = PlaylistContainer(name, true);

    return playlistName.unique_id;
}

std::optional<Id3Info> SimpleDatabase::getEntryOnId(const std::string& id) {
    for(auto& i : m_simpleDatabase ) {
        if (i.uid == id)
            return i;
    }
    return std::nullopt;
}

std::string SimpleDatabase::createAlbumPlaylistTmp(const std::string &album) {

    auto it = std::find_if(std::begin(m_playlistAlbum), std::end(m_playlistAlbum), [&album](const auto &it) {
        return (it.second.internalPlaylistName == album);
    });

    std::string playlistName;

    // if exist, return existing list
    if (it == std::end(m_playlistAlbum)) {
        auto name = NameGenerator::create("", "");
        m_playlistAlbum[name.unique_id] = PlaylistContainer(album, true);
        playlistName = name.unique_id;
    }
    else {
        playlistName = it->first;
        it->second.Playlist.clear();
        it->second.changed = true; // do always write the list
    }

    std::vector<Id3Info> infoList = findInDatabase(album, DatabaseSearchType::album);

    for(auto i : infoList) {
        m_playlistAlbum[playlistName].Playlist.push_back(i.uid);
    }

    // sort by track no
    std::sort(std::begin(m_playlistAlbum[playlistName].Playlist),
              std::end(m_playlistAlbum[playlistName].Playlist),
              [this](const std::string& a, const std::string& b)
              {
        auto trackNoA = getEntryOnId(a);
        auto trackNoB = getEntryOnId(b);
        if ( trackNoA && trackNoB )
            return trackNoA->track_no < trackNoB->track_no;
        return false;
    });

    return playlistName;
}

std::string SimpleDatabase::getNameFromHumanReadable(const std::string &humanReadablePlaylist) {
    std::string id;
    id = getNameFromHumanReadable(humanReadablePlaylist, m_playlist);
    if (!id.empty())
        return id;
    id = getNameFromHumanReadable(humanReadablePlaylist, m_playlistAlbum);
    if (!id.empty())
        return id;
    return "";
}

bool SimpleDatabase::isPlaylist(const std::string &playlistName) {
    return isPlaylist(playlistName, m_playlist) || isPlaylist(playlistName, m_playlistAlbum);
}

bool SimpleDatabase::addToPlaylist(const std::string &playlistName, const std::string &name) {
    auto item = std::find_if(std::begin(m_playlist), std::end(m_playlist), [&playlistName](const auto &elem) {
        return elem.second.internalPlaylistName == playlistName;
    });
    if (item != m_playlist.end()) {
        item->second.Playlist.push_back(name);
        item->second.changed = true;
        std::cerr << "add <"<<name<<"> to playlist <"<<playlistName<<">\n";
        return true;
    }
    return false;
}

bool SimpleDatabase::isPlaylistID(const std::string &playlistID) {
    return isPlaylistID(playlistID, m_playlist) || isPlaylistID(playlistID, m_playlistAlbum);
}

bool SimpleDatabase::addToPlaylistID(const std::string &ID, const std::string &name) {
    auto item = std::find_if(std::begin(m_playlist), std::end(m_playlist),
                             [&ID](const auto &elem) { return elem.first == ID; });
    if (item != m_playlist.end()) {
        item->second.Playlist.emplace_back(name);
        item->second.changed = true;
        return true;
    }
    return false;
}

std::vector<std::pair<std::string, std::string>> SimpleDatabase::showAllPlaylists() {
    std::vector<std::pair<std::string, std::string>> list;

    std::for_each(std::begin(m_playlist), std::end(m_playlist), [&list](const auto &elem) {
        list.push_back(std::make_pair(elem.first, elem.second.internalPlaylistName));
    });

    std::for_each(std::begin(m_playlistAlbum), std::end(m_playlistAlbum), [&list](const auto &elem) {
        list.push_back(std::make_pair(elem.first, elem.second.internalPlaylistName));
    });

    return list;
}

std::vector<std::string> SimpleDatabase::showPlaylist(const std::string &playlistName) {
    auto list = showPlaylist(playlistName, m_playlist);
    if (!list.empty())
        return list;
    return showPlaylist(playlistName, m_playlistAlbum);
}

void SimpleDatabase::emplace_back(Id3Info &&info) {
    m_simpleDatabase.emplace_back(info);
}
