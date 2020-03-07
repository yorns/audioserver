#include "id3repository.h"
#include "common/Constants.h"
#include "common/filesystemadditions.h"

using namespace Database;
using namespace LoggerFramework;
using namespace Common;

std::vector<Id3Info> Id3Repository::findAlbum(const std::string &what) {
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
                _info.title_name = "";
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
                    _info.title_name = "";
                    _info.all_tracks_no = 0;
                    _info.track_no = 0;
                    findData.emplace_back(_info);
                }
        });

    }
    return findData;
}

bool Id3Repository::add(const std::string &uniqueID) {
    std::string coverName = m_tagReader.extractCover(uniqueID);
    auto audioItem = m_tagReader.getInfo(uniqueID, coverName);
    if (audioItem) {
        logger(Level::debug) << "adding audio file <" << uniqueID
                             << "> Title: "<<audioItem->title_name << " to database\n";
        m_simpleDatabase.emplace_back(std::move(*audioItem));
        return true;
    }
    logger(Level::warning)<< "adding file id <" << uniqueID << "> failed\n";
    return false;
}

bool Id3Repository::remove(const std::string &uniqueID) {
    auto it = std::find_if(std::begin(m_simpleDatabase),
                           std::end(m_simpleDatabase),
                           [&uniqueID](const Id3Info& info) { return info.uid == uniqueID; });
    if (it != std::end(m_simpleDatabase)) {
        logger(Level::debug) << "removing audio file id <" << uniqueID
                             << "> Title: "<<it->title_name << " from database\n";
        m_simpleDatabase.erase(it);
        return true;
    }
    logger(Level::warning)<< "removing file id <" << uniqueID << "> failed\n";
    return false;
}

std::vector<std::tuple<Id3Info, std::vector<std::string> > > Id3Repository::findDuplicates() {

    if (m_simpleDatabase.size() < 1)
        return {};

    std::vector<std::tuple<Id3Info, std::vector<std::string>>> dublicateList;

    for(auto it{ std::cbegin(m_simpleDatabase) }; it != std::cend(m_simpleDatabase); ++it) {
        /* find dublicats within following entries */
        for(auto dub { it + 1 }; dub != std::cend(m_simpleDatabase); ++dub) {
            if (*it == *dub) {
                auto dublicatList = std::find_if(std::begin(dublicateList),
                                                 std::end(dublicateList), [&it](const auto& elem)
                { return std::get<Id3Info>(elem) == *it; });
                if ( dublicatList != std::end(dublicateList)) {
                    std::tuple<Id3Info, std::vector<std::string>> entry {*dublicatList};
                    auto list { std::get<std::vector<std::string>>(entry) };
                    list.push_back(dub->uid);
                }
                else {
                    Id3Info info {*it};
                    auto entryIt = dublicateList.insert(std::cbegin(dublicateList),
                                                        std::make_tuple<Id3Info, std::vector<std::string>> (std::move(info), {}));
                    std::tuple<Id3Info, std::vector<std::string>>& entry {*entryIt};
                    auto list { std::get<std::vector<std::string>>(entry) };
                    list.push_back(dub->uid);
                }
            }
        }
    }
    return dublicateList;
}

std::optional<std::vector<Id3Info> > Id3Repository::search(const std::string &what, SearchItem item, SearchAction action) {

    std::optional<std::vector<Id3Info>> findData;

    if (action == SearchAction::alike) {
        findData = findAlbum(what);
    }
    else {

        findData = std::vector<Id3Info>{};

        if (item == SearchItem::uid) {
            std::for_each(std::begin(m_simpleDatabase), std::end(m_simpleDatabase),
                          [&what, &findData](const Id3Info &info) {
                if (info.uid == what)
                    findData->push_back(info);
            });
        }

        std::for_each(std::begin(m_simpleDatabase), std::end(m_simpleDatabase),
                      [&what, &findData, item](const Id3Info &info) {
            if (((item == SearchItem::titel || item == SearchItem::overall) &&
                 (info.title_name.find(what) != std::string::npos)) ||
                    ((item == SearchItem::album || item == SearchItem::overall) &&
                     (info.album_name.find(what) != std::string::npos)) ||
                    ((item == SearchItem::interpret || item == SearchItem::overall) &&
                     (info.performer_name.find(what) != std::string::npos)))
                findData->push_back(info);
        });
    }

    if (findData->size() == 0)
        return std::nullopt;

    return findData;
}

bool Id3Repository::read() {

    auto mp3Directory = FileSystemAdditions::getFullQualifiedDirectory(FileType::Audio);

    auto filelist = FileSystemAdditions::getAllFilesInDir(FileType::Audio);
    auto imglist = FileSystemAdditions::getAllFilesInDir(FileType::Covers);

    logger(::Level::debug) << "reading all audio files in <" << mp3Directory << ">\n";
    auto getImageFileOf = [&imglist](const std::string& name) -> FileNameType {
        auto it = std::find_if(std::begin(imglist), std::end(imglist),
                               [&name](const FileNameType& fileName){ return fileName.name == name; });
        if (it != imglist.end())
            return *it;
        return {std::string(ServerConstant::unknownCoverFile),std::string(ServerConstant::unknownCoverExtension)};
    };

    for (auto& file : filelist) {
        logger(::Level::debug) << "reading audio file <"<<file.name<<file.extension<<"\n";
        auto imgFile = getImageFileOf(file.name);

        // this is the webservers view
        std::string webserverCoverPath = std::string("/")
                + std::string(ServerConstant::coverPathWeb)
                + '/' + imgFile.name + imgFile.extension;

        if (auto id3info = m_tagReader.getInfo(imgFile.name, webserverCoverPath))
            m_simpleDatabase.emplace_back(*id3info);

    }

    return true;
}
