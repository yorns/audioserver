#include "id3repository.h"
#include "common/Constants.h"
#include "common/filesystemadditions.h"
#include "common/albumlist.h"

using namespace Database;
using namespace LoggerFramework;
using namespace Common;

namespace fs = boost::filesystem;

bool Id3Repository::addCover(std::string uid, std::vector<char>&& data, std::size_t hash) {

    logger(Level::debug) << "add cover to database\n";
    auto coverIt = std::find_if(std::begin(m_simpleCoverDatabase), std::end(m_simpleCoverDatabase),
                                [&hash](const CoverElement& elem){ return elem.hash == hash; });

    bool isUpdated { false };

    if (coverIt == std::end(m_simpleCoverDatabase)) {
        logger(Level::debug) << " +++++ unknown cover adding (0x"<< std::hex << hash << std::dec << ")\n";
        CoverElement coverElem;
        coverElem.insertNewUid(std::move(uid));
        coverElem.hash = hash;
        coverElem.rawData = std::move(data);
        m_simpleCoverDatabase.emplace_back(coverElem);
        isUpdated = true;
    }
    else {
        logger(Level::debug) << "    -----  cover known (0x"<< std::hex << hash << std::dec << ")\n";
        isUpdated = coverIt->insertNewUid(std::move(uid));
    }

    if (isUpdated)
        m_cache_dirty = true;

    return isUpdated;
}

bool Id3Repository::add(const std::string &uid) {
    if (add(m_tagReader.readJsonAudioInfo(uid))) {
        m_cache_dirty = true;
        return true;
    }
    if (add(m_tagReader.readMp3AudioInfo(uid))) {
        m_cache_dirty = true;
        return true;
    }
    return false;
}

std::optional<nlohmann::json> Id3Repository::toJson(const std::vector<Id3Info> &id3Db) const {

    nlohmann::json data;

    try {
        for (const auto& id3Elem : id3Db) {
            nlohmann::json jsonElem;
            jsonElem["Uid"] = id3Elem.uid;
            jsonElem["Title"] = id3Elem.title_name;
            jsonElem["Album"] = id3Elem.album_name;
            jsonElem["Performer"] = id3Elem.performer_name;
            jsonElem["TrackNo"] = id3Elem.track_no;
            jsonElem["AllTrackNo"] = id3Elem.all_tracks_no;
            jsonElem["Extension"] = id3Elem.fileExtension;
            jsonElem["AlbumCreation"] = id3Elem.albumCreation;
            jsonElem["Url"] = id3Elem.url;
            data.push_back(jsonElem);
        }

    } catch (std::exception& ex) {
        logger(Level::warning) << "error in creationg Json for id3 database list: " << ex.what() << "\n";
        return std::nullopt;
    }

    return std::move(data);

}

const std::vector<Id3Info> Id3Repository::id3fromJson(const std::string &file) const {

    std::vector<Id3Info> id3Data;

    if (fs::exists(file)) {
        FullId3Information fullInfo;
        try {
            std::ifstream streamInfoFile(file.c_str());
            nlohmann::json streamList = nlohmann::json::parse(streamInfoFile);

            for (const auto& streamInfo : streamList) {
            Id3Info info;
            info.uid = streamInfo.at("Uid");
            info.title_name = streamInfo.at("Title");
            info.album_name = streamInfo.at("Album");
            info.performer_name = streamInfo.at("Performer");
            info.track_no = streamInfo.at("TrackNo");
            info.all_tracks_no = streamInfo.at("AllTrackNo");
            info.fileExtension = streamInfo.at("Extension");
            info.albumCreation = streamInfo.at("AlbumCreation");
            info.url = streamInfo.at("Url");

            info.finishEntry();
            logger(LoggerFramework::Level::debug) << "reading <" << info.uid << "> <"<<info.title_name<<">\n";
            id3Data.emplace_back(info);
            }

        } catch (std::exception& ex) {
            logger(Level::warning) << "failed to read file: " << file << ": " << ex.what() << "\n";
            return {};
        }
    }
    return id3Data;

}

std::optional<nlohmann::json> Id3Repository::toJson(const std::vector<CoverElement> &coverDb) const {

    nlohmann::json data;

    try {
        for (const auto& elem : coverDb) {
            nlohmann::json jsonElem;
            jsonElem["Hash"] = elem.hash;
            jsonElem["HasCover"] = !elem.rawData.empty();
            if (!elem.rawData.empty()) {
                jsonElem["Cover"] = utility::base64_encode(elem.rawData.data(), elem.rawData.size());
            }
            auto& uidList = jsonElem["UidList"];
            for(const auto& uidElem : elem.uidListForCover)
                uidList.push_back(uidElem);
            logger(LoggerFramework::Level::debug) << "reading cover hash <" << elem.hash << ">\n";
            data.push_back(jsonElem);
        }
    } catch (std::exception& ex) {
        logger(Level::warning) << "error in creationg Json for coverElement list: " << ex.what() << "\n";
        return std::nullopt;
    }

    return std::move(data);
}

const std::vector<CoverElement> Id3Repository::coverFromJson(const std::string &filename) const {
    std::vector<CoverElement> coverList;

    if (fs::exists(filename)) {
        FullId3Information fullInfo;
        try {
            std::ifstream streamInfoFile(filename.c_str());
            nlohmann::json streamInfo = nlohmann::json::parse(streamInfoFile);
            for (const auto& elem : streamInfo) {
                CoverElement coverElement;
                coverElement.hash = elem["Hash"];
                auto vec = utility::base64_decode(elem["Cover"]);
                coverElement.rawData = std::move(vec);
                for (const auto& uidElem : elem["UidList"]) {
                    coverElement.uidListForCover.push_back(uidElem);
                }
                coverList.emplace_back(coverElement);
            }
        } catch (std::exception& ex) {
            logger(Level::warning) << "error parsing <"<< filename <<"> : " << ex.what() << "\n";
            return {};
        }
    }

    return coverList;
}

bool Id3Repository::writeJson(nlohmann::json &&data, const std::string &filename) const {
    std::ofstream of(filename.c_str());

    if (of.good()) {
        of << data;
        of.close();
        return true;
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

bool Id3Repository::readCache() {

    if (! m_enableCache) {
        logger(Level::info) << "cache not enabled, no read on cache\n";
        return false;
    }

    std::string id3CacheFile = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Cache)
            + "/id3_cache.json";
    std::string coverCacheFile = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Cache)
            + "/cover_cache.json";


    auto id3DatabaseList = id3fromJson(id3CacheFile);
    // convert json to id3Info vector
    m_simpleDatabase.insert(m_simpleDatabase.end(), std::begin(id3DatabaseList), std::end(id3DatabaseList));
    // read image cache
    auto coverDatabaseList = coverFromJson(coverCacheFile);
    // convert json to cover Database
    m_simpleCoverDatabase.insert(m_simpleCoverDatabase.end(), std::begin(coverDatabaseList), std::end(coverDatabaseList));

    return true;
}

bool Id3Repository::writeCacheInternal() {

    if (! m_enableCache) {
        logger(Level::info) << "cache not enabled, no write on cache\n";
        return false;
    }

    if (!m_cache_dirty)
        return false;

    logger(Level::debug) << "writing cache\n";

    std::string id3CacheFile = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Cache)
            + "/id3_cache.json";
    std::string coverCacheFile = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Cache)
            + "/cover_cache.json";

    // create json file
    auto jsonDatabase = toJson(m_simpleDatabase);
    // write simple database
    if (jsonDatabase)
        writeJson(std::move(*jsonDatabase), id3CacheFile);
    // write cover database

    auto jsonCover = toJson(m_simpleCoverDatabase);
    if (jsonCover)
        writeJson(std::move(*jsonCover), coverCacheFile);

    return true;
}

bool Id3Repository::remove(const std::string &uniqueID) {
    auto it = std::find_if(std::begin(m_simpleDatabase),
                           std::end(m_simpleDatabase),
                           [&uniqueID](const Id3Info& info) { return info.uid == uniqueID; });
    if (it != std::end(m_simpleDatabase)) {
        logger(Level::debug) << "removing audio file id <" << uniqueID
                             << "> Title: "<<it->title_name << " from database\n";
        m_simpleDatabase.erase(it);
        m_cache_dirty = true;
        return true;
    }
    logger(Level::warning)<< "removing file id <" << uniqueID << "> failed\n";
    return false;
}

void Id3Repository::clear() {
    m_simpleDatabase.clear();
    m_cache_dirty = true;
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

    logger(Level::info) << "start reading cache information\n";
    readCache();

    auto filelist = FileSystemAdditions::getAllFilesInDir(FileType::AudioMp3);
    auto jsonList = FileSystemAdditions::getAllFilesInDir(FileType::AudioJson);

    logger(Level::info) << "read mp3 file information\n";
    for (auto& file : filelist) {
        if (!isCached(file.name)) {
            logger(::Level::debug) << "list of id3 files - reading audio file <"<<file.name<<file.extension<<">\n";
            if (add(m_tagReader.readMp3AudioInfo(file.name)))
                m_cache_dirty = true;
        }
    }

    logger(Level::info) << "read json file information\n";
    for (auto& file : jsonList) {
        if (!isCached(file.name)) {
            logger(::Level::debug) << "list of json files - reading audio file <"<<file.name<<file.extension<<">\n";
            if (add(m_tagReader.readJsonAudioInfo(file.name)))
                m_cache_dirty = true;
        }
    }

    logger(Level::info) << "database read completed\n";

    writeCacheInternal();

    return true;
}

const CoverElement &Id3Repository::getCover(const std::string &coverUid) const {
    auto it = std::find_if(std::begin(m_simpleCoverDatabase),
                           std::end(m_simpleCoverDatabase),
                           [&coverUid](const CoverElement& elem){
        return elem.isConnectedToUid(coverUid);
    });
    if (it != std::end(m_simpleCoverDatabase)) {
        return *it;
    }
    return emptyElement;
}
