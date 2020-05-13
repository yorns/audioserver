#include "id3repository.h"
#include "common/Constants.h"
#include "common/filesystemadditions.h"
#include "common/albumlist.h"

using namespace Database;
using namespace LoggerFramework;
using namespace Common;

bool Id3Repository::addCover(std::string uid, std::vector<char>&& data, std::size_t hash) {

    logger(Level::debug) << "add cover to database\n";
    auto coverIt = std::find_if(std::begin(m_simpleCoverDatabase), std::end(m_simpleCoverDatabase),
                                [&hash](const CoverElement& elem){ return elem.hash == hash; });

    if (coverIt == std::end(m_simpleCoverDatabase)) {
        logger(Level::debug) << " +++++ unknown cover adding (0x"<< std::hex << hash << ")\n";
        CoverElement coverElem;
        coverElem.insertNewUid(std::move(uid));
        coverElem.hash = hash;
        coverElem.rawData = std::move(data);
        m_simpleCoverDatabase.emplace_back(coverElem);
    }
    else {
        logger(Level::debug) << "    -----  cover known (0x"<< std::hex << hash << ")\n";
        coverIt->insertNewUid(std::move(uid));
    }

    return false;
}

bool Id3Repository::add(std::optional<FullId3Information>&& audioItem) {

    if (audioItem) {
        std::string uniqueID = audioItem->info.uid;
        logger(Level::debug) << "adding audio file <" << uniqueID
                             << "> ("<<audioItem->info.title_name <<"/" << audioItem->info.album_name << ") to database\n";
        m_simpleDatabase.emplace_back(std::move(audioItem->info));
        if (audioItem->pictureAvailable) {
            if (addCover(uniqueID, std::move(audioItem->data), audioItem->hash))
                logger(Level::debug) << "added audioItem <" << uniqueID << ">\n";
        }
        return true;
    }

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

std::vector<Id3Info> Id3Repository::search(const std::string &what, SearchItem item, SearchAction action) {

    std::vector<Id3Info> findData;

    if (action == SearchAction::uniqueId) {
        std::for_each(std::begin(m_simpleDatabase), std::end(m_simpleDatabase),
                      [&what, &findData](const Id3Info &info) {
            if (info.uid == what) {
                logger(Level::debug) << "found alike search: " << info.toString() <<"\n";
                findData.push_back(info);
            }
        });
    }
    else {
        auto whatList = Common::extractWhatList(what);

        std::for_each(std::begin(m_simpleDatabase), std::end(m_simpleDatabase),
                      [&whatList, &what, &findData, item, action](const Id3Info &info) {
            if (action == SearchAction::alike) {
                if ( info.isAlike(whatList) ) {
                    findData.push_back(info);
                }
            }
            else {

                if (((item == SearchItem::titel || item == SearchItem::overall) &&
                     (info.title_name.find(what) != std::string::npos)) ||
                        ((item == SearchItem::album || item == SearchItem::overall) &&
                         (info.album_name.find(what) != std::string::npos)) ||
                        ((item == SearchItem::interpret || item == SearchItem::overall) &&
                         (info.performer_name.find(what) != std::string::npos)))
                    findData.push_back(info);
            }
        });
    }
    return findData;
}

std::vector<Common::AlbumListEntry> Id3Repository::extractAlbumList() {
    std::vector<AlbumListEntry> albumList;

    for(auto it {std::cbegin(m_simpleDatabase)}; it != std::cend(m_simpleDatabase); ++it) {

        if (it->albumCreation) {

        auto albumIt = std::find_if(std::begin(albumList), std::end(albumList), [&it](const AlbumListEntry& albumListEntry)
        { return albumListEntry.m_name == it->album_name; });

        if (albumIt == std::cend(albumList)) {
            AlbumListEntry entry;
            entry.m_name = it->album_name;
            entry.m_performer = it->performer_name;
            if (!it->fileExtension.empty()) {
                entry.m_coverExtension = it->fileExtension;
                entry.m_coverId = it->uid;
            } else {
                entry.m_coverExtension = ServerConstant::unknownCoverExtension;
                entry.m_coverId = ServerConstant::unknownCoverFile;
            }
            entry.m_playlist.push_back(std::make_tuple(it->uid, it->track_no));
            albumList.emplace_back(entry);
        }
        else {
            if (it->performer_name != albumIt->m_performer)
                albumIt->m_performer = "multiple performer";
            if (albumIt->m_coverId == ServerConstant::unknownCoverFile && !it->fileExtension.empty()) {
                albumIt->m_coverExtension = it->fileExtension;
                albumIt->m_coverId = it->uid;
            }

            albumIt->m_playlist.push_back(std::make_tuple(it->uid, it->track_no));
        }
        }
    }
    logger(Level::info) << "extracted <"<<albumList.size()<<"> album playlists\n";
    for (auto& i : albumList) {
        logger(Level::debug) << "  - <"<<i.m_name<<">\n";

        std::sort(std::begin(i.m_playlist), std::end(i.m_playlist), [](const auto& t1, const auto& t2){ return std::get<uint32_t>(t1) < std::get<uint32_t>(t2); });

    }

    return albumList;
}

std::optional<Id3Info> Id3Repository::getId3InfoByUid(const std::string &uniqueId) const {
    auto it = std::find_if(std::begin(m_simpleDatabase),
                           std::end(m_simpleDatabase),
                           [&uniqueId](const Id3Info& info) { return info.uid == uniqueId; });

    if (it==std::end(m_simpleDatabase))
        return std::nullopt;
    else
        return *it;
}

bool Id3Repository::read() {

    logger(Level::info) << "start reading id3 information\n";
    auto filelist = FileSystemAdditions::getAllFilesInDir(FileType::AudioMp3);
    auto jsonList = FileSystemAdditions::getAllFilesInDir(FileType::AudioJson);

    logger(Level::info) << "read mp3 file information\n";
    for (auto& file : filelist) {
        logger(::Level::debug) << "list of id3 files - reading audio file <"<<file.name<<file.extension<<">\n";
        add(m_tagReader.readMp3AudioInfo(file.name));
    }

    logger(Level::info) << "read json file information\n";
    for (auto& file : jsonList) {
        logger(::Level::debug) << "list of json files - reading audio file <"<<file.name<<file.extension<<">\n";
        add(m_tagReader.readJsonAudioInfo(file.name));
    }

    logger(Level::info) << "database read completed\n";

    return true;
}
