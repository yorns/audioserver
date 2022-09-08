#include "id3repository.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "common/Constants.h"
#include "common/filesystemadditions.h"
#include "common/albumlist.h"
#include <iterator>
#include <vector>
#include <stack>

using namespace Database;
using namespace LoggerFramework;
using namespace Common;

namespace fs = boost::filesystem;

bool Database::writeJson(nlohmann::json &&data, const std::string &filename) {
    std::ofstream of(filename.c_str());

    if (of.good()) {
        of << data.dump(2);
        of.close();
        return true;
    }

    return false;
}


const std::vector<CoverElement> Database::coverFromJson(const std::string &filename) {
    std::vector<CoverElement> coverList;

    if (fs::exists(filename)) {
        FullId3Information fullInfo;
        try {
            std::ifstream streamInfoFile(filename.c_str());
            nlohmann::json streamInfo = nlohmann::json::parse(streamInfoFile);
            for (const auto& elem : streamInfo) {
                CoverElement coverElement;
                coverElement.hash = elem["Hash"];
                //auto vec = utility::base64_decode(elem["Cover"]);
                //coverElement.rawData = std::move(vec);
                for (const auto& uidElem : elem["UidList"]) {
                    coverElement.uidListForCover.push_back(boost::lexical_cast<boost::uuids::uuid>(std::string(uidElem)));
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

std::optional<nlohmann::json> Database::coverToJson(const std::vector<CoverElement> &coverDb) {

    nlohmann::json data;

    try {
        for (const auto& elem : coverDb) {
            nlohmann::json jsonElem;
            jsonElem["Hash"] = elem.hash;
            jsonElem["HasCover"] = !elem.rawData.empty();
            //if (!elem.rawData.empty()) {
            //    jsonElem["Cover"] = utility::base64_encode(elem.rawData.data(), static_cast<uint32_t>(elem.rawData.size()));
            //}
            auto& uidList = jsonElem["UidList"];
            for(const auto& uidElem : elem.uidListForCover)
                uidList.push_back(boost::uuids::to_string(uidElem));
            logger(LoggerFramework::Level::debug) << "reading cover hash <" << elem.hash << ">\n";
            data.push_back(jsonElem);
        }
    } catch (std::exception& ex) {
        logger(Level::warning) << "error in creationg Json for coverElement list: " << ex.what() << "\n";
        return std::nullopt;
    }

    return std::optional<nlohmann::json>(data);
}


bool Id3Repository::addCover(boost::uuids::uuid&& uid, std::vector<char>&& data) {

    bool isUpdated = m_simpleCoverDatabase.addCover(std::move(data), uid);

//    logger(Level::debug) << "add cover to database\n";
//    auto coverIt = std::find_if(std::begin(m_simpleCoverDatabase), std::end(m_simpleCoverDatabase),
//                                [&hash](const CoverElement& elem){ return elem.hash == hash; });

//    bool isUpdated { false };

//    if (coverIt == std::end(m_simpleCoverDatabase)) {
//        logger(Level::debug) << " +++++ unknown cover adding (0x"<< std::hex << hash << std::dec << ")\n";
//        CoverElement coverElem;
//        coverElem.insertNewUid(std::move(uid));
//        coverElem.hash = hash;
//        coverElem.rawData = std::move(data);
//        m_simpleCoverDatabase.emplace_back(coverElem);
//        isUpdated = true;
//    }
//    else {
//        logger(Level::debug) << "    -----  cover known (0x"<< std::hex << hash << std::dec << ")\n";
//        isUpdated = coverIt->insertNewUid(std::move(uid));
//    }

    if (isUpdated)
        m_cache_dirty = true;

    return isUpdated;
}

std::optional<boost::uuids::uuid> Id3Repository::add(const Common::FileNameType& filename) {

    if (auto entry = add(m_tagReader.readJsonAudioInfo(filename))) {
        m_cache_dirty = true;
        logger (Level::info) << "added info " << boost::lexical_cast<std::string>(*entry) << " to repository\n";
        return entry;
    }
    if (auto entry = add(m_tagReader.readMp3AudioInfo(filename))) {
        m_cache_dirty = true;
        logger (Level::info) << "added song " << boost::lexical_cast<std::string>(*entry)
                             << "(filename: " << filename.name << filename.extension << ") to repository\n";
        return entry;
    }
    return std::nullopt;
}

// from: http://www.zedwood.com/article/cpp-is-valid-utf8-string-function
bool Id3Repository::utf8_check_is_valid(const std::string& string) const
{
    int c,i,ix,n,j;
    for (i=0, ix=(int)string.length(); i < ix; i++)
    {
        c = (unsigned char) string[i];
        //if (c==0x09 || c==0x0a || c==0x0d || (0x20 <= c && c <= 0x7e) ) n = 0; // is_printable_ascii
        if (0x00 <= c && c <= 0x7f) n=0; // 0bbbbbbb
        else if ((c & 0xE0) == 0xC0) n=1; // 110bbbbb
        else if ( c==0xed && i<(ix-1) && ((unsigned char)string[i+1] & 0xa0)==0xa0) return false; //U+d800 to U+dfff
        else if ((c & 0xF0) == 0xE0) n=2; // 1110bbbb
        else if ((c & 0xF8) == 0xF0) n=3; // 11110bbb
        //else if (($c & 0xFC) == 0xF8) n=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
        //else if (($c & 0xFE) == 0xFC) n=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
        else return false;
        for (j=0; j<n && i<ix; j++) { // n bytes matching 10bbbbbb follow ?
            if ((++i == ix) || (( (unsigned char)string[i] & 0xC0) != 0x80))
                return false;
        }
    }
    return true;
}

std::optional<nlohmann::json> Id3Repository::id3ToJson(const std::vector<Id3Info>::const_iterator& begin, const std::vector<Id3Info>::const_iterator& end) const
{
    nlohmann::json data;

    if (begin == end)
        return std::nullopt;

    try {
        for (auto it = begin; it != end; ++it) {
            const auto& id3Elem = *it;
            if (!utf8_check_is_valid(id3Elem.informationSource)) {
              logger(Level::error) << "wrong encoding: " << id3Elem.informationSource << "\n";
              continue;
            }
            if (!utf8_check_is_valid(id3Elem.urlAudioFile)) {
              logger(Level::error) << "wrong encoding: " << id3Elem.urlAudioFile << "\n";
              continue;
            }

            nlohmann::json jsonElem;
            jsonElem["Uid"] = boost::uuids::to_string(id3Elem.uid);
            jsonElem["InfoSrc"] = id3Elem.informationSource;
            jsonElem["Title"] = id3Elem.title_name;
            jsonElem["Album"] = id3Elem.album_name;
            jsonElem["Performer"] = id3Elem.performer_name;
            jsonElem["TrackNo"] = id3Elem.track_no;
            jsonElem["AllTrackNo"] = id3Elem.all_tracks_no;
            jsonElem["Extension"] = id3Elem.coverFileExt;
            jsonElem["AlbumCreation"] = id3Elem.albumCreation;
            jsonElem["Disk"] = id3Elem.cd_no;
            jsonElem["Url"] = id3Elem.urlAudioFile;
            jsonElem["CoverUrl"] = id3Elem.urlCoverFile;
            data.push_back(jsonElem);
        }

    } catch (std::exception& ex) {
        logger(Level::warning) << "error in creationg Json for id3 database list: " << ex.what() << "\n";
        return std::nullopt;
    }

    return std::optional<nlohmann::json>(data);


}


std::optional<nlohmann::json> Id3Repository::id3ToJson(const std::vector<Id3Info> &id3Db) const {

    nlohmann::json data;

    try {
        for (const auto& id3Elem : id3Db) {

            if (!utf8_check_is_valid(id3Elem.informationSource)) {
              logger(Level::error) << "wrong encoding: " << id3Elem.informationSource << "\n";
              continue;
            }
            if (!utf8_check_is_valid(id3Elem.urlAudioFile)) {
              logger(Level::error) << "wrong encoding: " << id3Elem.urlAudioFile << "\n";
              continue;
            }

            nlohmann::json jsonElem;
            jsonElem["Uid"] = boost::uuids::to_string(id3Elem.uid);
            jsonElem["InfoSrc"] = id3Elem.informationSource;
            jsonElem["Title"] = id3Elem.title_name;
            jsonElem["Album"] = id3Elem.album_name;
            jsonElem["Performer"] = id3Elem.performer_name;
            jsonElem["TrackNo"] = id3Elem.track_no;
            jsonElem["AllTrackNo"] = id3Elem.all_tracks_no;
            jsonElem["Extension"] = id3Elem.coverFileExt;
            jsonElem["AlbumCreation"] = id3Elem.albumCreation;
            jsonElem["Disk"] = id3Elem.cd_no;
            jsonElem["Url"] = id3Elem.urlAudioFile;
            jsonElem["lesCoverUrl"] = id3Elem.urlCoverFile;
            data.push_back(jsonElem);
        }

    } catch (std::exception& ex) {
        logger(Level::warning) << "error in creationg Json for id3 database list: " << ex.what() << "\n";
        return std::nullopt;
    }

    return std::optional<nlohmann::json>(data);

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
            info.uid = boost::lexical_cast<boost::uuids::uuid>(std::string(streamInfo.at("Uid")));
            info.informationSource = streamInfo.at("InfoSrc");
            info.title_name = streamInfo.at("Title");
            info.album_name = streamInfo.at("Album");
            info.performer_name = streamInfo.at("Performer");
            info.track_no = streamInfo.at("TrackNo");
            info.all_tracks_no = streamInfo.at("AllTrackNo");
            info.coverFileExt = streamInfo.at("Extension");
            info.albumCreation = streamInfo.at("AlbumCreation");
            info.cd_no = streamInfo.at("Disk");
            info.urlAudioFile = streamInfo.at("Url");
            info.urlCoverFile = streamInfo.at("CoverUrl");

            info.finishEntry();
            logger(LoggerFramework::Level::debug) << "reading <" << boost::uuids::to_string(info.uid) << "> <"<<info.title_name<<">\n";
            id3Data.emplace_back(info);
            }

        } catch (std::exception& ex) {
            logger(Level::warning) << "failed to read file: " << file << ": " << ex.what() << "\n";
            return {};
        }
    }
    return id3Data;

}




std::optional<boost::uuids::uuid> Id3Repository::add(std::optional<FullId3Information>&& audioItem) {

    if (audioItem) {
        auto uniqueID = audioItem->info.uid;
        auto uniqueID_cp = uniqueID;
        try {
        logger(Level::info) << "adding audio file <" << audioItem->info.urlAudioFile << audioItem->info.audioFileExt //boost::lexical_cast<std::string>(uniqueID)
                             << "> ("<<audioItem->info.title_name <<"/" << audioItem->info.album_name << ") to database\n";
        } catch (std::exception& ex) {
            logger(LoggerFramework::Level::warning) << ex.what() << "\n";
        }
        if (!audioItem->pictureAvailable || audioItem->info.urlCoverFile.empty()) {
            audioItem->info.urlCoverFile = "img/unknown.png";
            audioItem->info.coverFileExt = ".png";
        }

        m_simpleDatabase.emplace_back(std::move(audioItem->info));
        if (audioItem->pictureAvailable) {
            if (addCover(std::move(uniqueID), std::move(audioItem->data)))
                logger(Level::debug) << "added audioItem <" << uniqueID << ">\n";
        }
        return uniqueID_cp;
    }

    return std::nullopt;
}

bool Id3Repository::readCache() {

    if (! m_enableCache) {
        logger(Level::info) << "cache not enabled, no read on cache\n";
        return false;
    }

    std::string id3CacheFileBase = "id3_cache";
    std::string coverCacheFile = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Cache)
            + "/cover_cache.json";


    // read audio cach information
    auto cacheFileList = Common::FileSystemAdditions::getAllFilesInDir(FileType::Cache);
    for (auto file : cacheFileList) {
        if ( file.extension == ".json" &&
             file.name.substr(0, id3CacheFileBase.length()) == id3CacheFileBase) {
            auto id3CacheFileName = file.dir + "/" + file.name + file.extension;
            logger(LoggerFramework::Level::debug) << "reading cache file "<< id3CacheFileName <<"\n";
            auto id3DatabaseList = id3fromJson(id3CacheFileName);
            // convert json to id3Info vector
            m_simpleDatabase.insert(m_simpleDatabase.end(), std::begin(id3DatabaseList), std::end(id3DatabaseList));
        }
    }

    m_simpleCoverDatabase.readCache(coverCacheFile);

    return true;
}

bool Id3Repository::writeCacheInternal() {

    if (! m_enableCache) {
        logger(Level::info) << "cache not enabled, no write on cache\n";
        return false;
    }

    if (!m_cache_dirty) {
        logger(Level::info) << "no changes in cache found\n";
        return false;
    }

    logger(Level::debug) << "writing cache\n";

    std::string id3CacheFileBase = "id3_cache";
    std::string coverCacheFile = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Cache)
            + "/cover_cache.json";

    // create json files
    auto genCacheName = [](uint32_t id) {
        return Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Cache)
                    + "/id3_cache_" + std::to_string(id) + ".json";
    };

#define MAXJSONFILES 1000
#define MAXENTRIESPERFILE 200
    unsigned int i{0};
    bool endFound {false};
    while (!endFound) {
        auto start = std::next(std::begin(m_simpleDatabase), i*MAXENTRIESPERFILE);
        auto stop = std::next(std::begin(m_simpleDatabase), (i+1)*MAXENTRIESPERFILE);

        if ((i+1)* MAXENTRIESPERFILE >= m_simpleDatabase.size()) {
            stop = std::end(m_simpleDatabase);
            endFound = true;
        }

        auto jsonDatabase = id3ToJson(start, stop);

        std::cerr << ".";//jsonDatabase->dump(2);

        // write simple database
        if (jsonDatabase) {
            writeJson(std::move(*jsonDatabase), genCacheName(i));
            std::cerr << "#("<<i<<")";//jsonDatabase->dump(2);
        }
        else {
            endFound = true;
        }
        i++;
    }
    std::cerr << "\n";

    m_simpleCoverDatabase.writeCache(coverCacheFile);

    return true;
}

bool Id3Repository::isCached(const std::string &url) const {
    logger(LoggerFramework::Level::debug) << "cache test for url <"<<url<<">\n";
    return std::find_if(std::cbegin(m_simpleDatabase), std::cend(m_simpleDatabase),
                        [&url](const Id3Info& elem) { return elem.informationSource == url; }) != std::cend(m_simpleDatabase);
}

bool Id3Repository::remove(const boost::uuids::uuid& uniqueID) {
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

std::vector<std::tuple<Id3Info, std::vector<boost::uuids::uuid>>> Id3Repository::findDuplicates() {

    if (m_simpleDatabase.size() < 1)
        return {};

    std::vector<std::tuple<Id3Info, std::vector<boost::uuids::uuid>>> dublicateList;

    for(auto it{ std::cbegin(m_simpleDatabase) }; it != std::cend(m_simpleDatabase); ++it) {
        /* find dublicats within following entries */
        for(auto dub { it + 1 }; dub != std::cend(m_simpleDatabase); ++dub) {
            if (*it == *dub) {
                auto dublicatList = std::find_if(std::begin(dublicateList),
                                                 std::end(dublicateList), [&it](const auto& elem)
                { return std::get<Id3Info>(elem) == *it; });
                if ( dublicatList != std::end(dublicateList)) {
                    std::tuple<Id3Info, std::vector<boost::uuids::uuid>> entry {*dublicatList};
                    auto list { std::get<std::vector<boost::uuids::uuid>>(entry) };
                    list.push_back(dub->uid);
                }
                else {
                    Id3Info info {*it};
                    auto entryIt = dublicateList.insert(std::cbegin(dublicateList),
                                                        std::make_tuple<Id3Info, std::vector<boost::uuids::uuid>> (std::move(info), {}));
                    std::tuple<Id3Info, std::vector<boost::uuids::uuid>>& entry {*entryIt};
                    auto list { std::get<std::vector<boost::uuids::uuid>>(entry) };
                    list.push_back(dub->uid);
                }
            }
        }
    }
    return dublicateList;
}

std::vector<Id3Info> Id3Repository::search(const boost::uuids::uuid &what, SearchItem , SearchAction action) {

    std::vector<Id3Info> findData;

    if (action == SearchAction::uniqueId) {
        try {
            std::for_each(std::begin(m_simpleDatabase), std::end(m_simpleDatabase),
                          [&what, &findData](const Id3Info &info) {
                if (info.uid == what) {
                    //logger(Level::debug) << "found uniqueId search: " << info.toString() <<"\n";
                    findData.push_back(info);
                }
            });
        } catch (std::exception& ex) {
            logger(Level::warning) << "cannot convert Uid <"<<what<<">: "<<ex.what()<<"\n";
        }
    }

    return findData;
}

std::vector<Id3Info> Id3Repository::upn(std::vector<std::string>& whatList, SearchItem item) {

    std::stack<std::string> stack;
    std::vector<Id3Info> retList;

    if (whatList.size() < 2) {
        return retList;
    }

    for (auto& it : whatList) {
        if (it == "&" || it=="|") {
            if (stack.size() < 2) {
                logger (Level::warning) << "UPN failed\n";
                break;
            }
            auto value1 = stack.top();
            stack.pop();
            auto value2 = stack.top();
            stack.pop();

            std::for_each(std::begin(m_simpleDatabase), std::end(m_simpleDatabase),[&value1, &value2, &item, &it, &retList](const Id3Info& info){
                if (it == "&") {

                    if (((item == SearchItem::title || item == SearchItem::overall) &&
                         (info.title_name.find(value1) != std::string::npos &&
                          info.title_name.find(value2) != std::string::npos)) ||
                            ((item == SearchItem::album || item == SearchItem::overall) &&
                             (info.album_name.find(value1) != std::string::npos &&
                              info.album_name.find(value2) != std::string::npos)) ||
                            ((item == SearchItem::performer || item == SearchItem::overall) &&
                             (info.performer_name.find(value1) != std::string::npos &&
                              info.performer_name.find(value2) != std::string::npos)
                             )) {
                        retList.push_back(info);
                    }

                }
                if (it == "|") {
                    if (((item == SearchItem::title || item == SearchItem::overall) &&
                         (info.title_name.find(value1) != std::string::npos ||
                          info.title_name.find(value2) != std::string::npos)) ||
                            ((item == SearchItem::album || item == SearchItem::overall) &&
                             (info.album_name.find(value1) != std::string::npos ||
                              info.album_name.find(value2) != std::string::npos)) ||
                            ((item == SearchItem::performer || item == SearchItem::overall) &&
                             (info.performer_name.find(value1) != std::string::npos ||
                              info.performer_name.find(value2) != std::string::npos)
                             )) {
                        retList.push_back(info);
                    }
                }
            });
        } else {
            stack.push(it);
        }

    }

    return retList;
}

std::vector<Id3Info> Id3Repository::search(const std::string &what, SearchItem item, SearchAction action) {

    std::vector<Id3Info> findData;

    if (action == SearchAction::uniqueId || item == SearchItem::uid) {
        try {
            auto whatUuid = boost::lexical_cast<boost::uuids::uuid>(what);
            std::for_each(std::begin(m_simpleDatabase), std::end(m_simpleDatabase),
                          [&whatUuid, &findData](const Id3Info &info) {
                if (info.uid == whatUuid) {
                    logger(Level::info) << "found uniqueId search: " << info.toString() <<"\n";
                    findData.push_back(info);
                }
            });
        } catch (std::exception& ex) {
            logger(Level::warning) << "cannot convert Uid <"<<what<<">: "<<ex.what()<<"\n";
        }
    }
    else {
        auto whatList = Common::extractWhatList(what);

        if(item == SearchItem::overall &&
                ( what.find_first_of(" & ") != std::string::npos ||
                  what.find_first_of(" | ") != std::string::npos )
                ) {
            findData = upn(whatList, item);
        }
        else {

            std::for_each(std::begin(m_simpleDatabase), std::end(m_simpleDatabase),
                          [&whatList, &what, &findData, item, action](const Id3Info &info) {
                if (action == SearchAction::alike) {
                    if (((item == SearchItem::title || item == SearchItem::overall) &&
                         (info.isAlikeTitle(whatList))) ||
                            ((item == SearchItem::album || item == SearchItem::overall || item == SearchItem::album_and_interpret) &&
                             (info.isAlikeAlbum(whatList))) ||
                            ((item == SearchItem::album || item == SearchItem::overall || item == SearchItem::album_and_interpret) &&
                             (info.isAlikeTag(whatList))) ||
                            ((item == SearchItem::performer || item == SearchItem::overall || item == SearchItem::album_and_interpret) &&
                             (info.isAlikePerformer(whatList)))) {
                        findData.push_back(info);
                    }
                }
                else {
                    if (((item == SearchItem::title || item == SearchItem::overall) &&
                         (info.title_name == what)) ||
                            ((item == SearchItem::album || item == SearchItem::overall) &&
                             (info.album_name == what)) ||
                            ((item == SearchItem::performer || item == SearchItem::overall) &&
                             (info.performer_name == what))) {
                        findData.push_back(info);
                    }
                }
            });
        }
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
                entry.m_tagList = it->tags;
                entry.m_coverUrl = it->urlCoverFile;
                entry.m_playlist.push_back(std::make_tuple(it->uid, it->cd_no*1000 + it->track_no));
                albumList.emplace_back(entry);
            }
            else {
                if (it->performer_name != albumIt->m_performer) {
                    albumIt->m_performer = "multiple performer";
                }
                if (albumIt->m_coverUrl.empty() && !it->urlCoverFile.empty()) {
                    albumIt->m_coverUrl = it->urlCoverFile;
                }

                albumIt->m_playlist.push_back(std::make_tuple(it->uid, it->cd_no*1000 + it->track_no));
                albumIt->m_tagList = it->tags;
            }
        }
    }
    logger(Level::info) << "extracted <"<<albumList.size()<<"> album playlists\n";
    for (auto& i : albumList) {
        std::sort(std::begin(i.m_playlist), std::end(i.m_playlist), [](const auto& t1, const auto& t2)
        { return std::get<uint32_t>(t1) < std::get<uint32_t>(t2); });

    }

    return albumList;
}

std::optional<Id3Info> Id3Repository::getId3InfoByUid(const boost::uuids::uuid& uniqueId) const {
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

    logger(Level::info) << "reading uncached files - read mp3 file information\n";
    for (auto& file : filelist) {
        std::string fullname = "file://" + FileSystemAdditions::getFullName(file);
        if (!isCached(fullname)) {
            logger(::Level::debug) << "id3 file <"<<fullname<<"> not cached - reading\n";
            if (add(m_tagReader.readMp3AudioInfo(file)))
                m_cache_dirty = true;
        }
        else {
            logger(Level::debug) << "file <"<<file.name << "> is cached\n";
        }
    }

    logger(Level::info) << "reading uncached files - read json file information\n";
    for (auto& file : jsonList) {
        std::string fullname = "file://" + FileSystemAdditions::getFullName(file);
        if (!isCached(fullname)) {
            logger(::Level::debug) << "list of json files - reading audio file <" << fullname << ">\n";
            if (add(m_tagReader.readJsonAudioInfo(file)))
                m_cache_dirty = true;
        }
        else {
            logger(Level::debug) << "file <"<<file.name << "> is cached\n";
        }
    }

    logger(Level::info) << "database read completed\n";

    writeCacheInternal();

    return true;
}

bool Id3Repository::writeCache() {
    return writeCacheInternal();
}

void Id3Repository::addTags(const SongTagReader& songTagReader)
{
    for (auto& elem : m_simpleDatabase) {
        auto tagList = songTagReader.findSongTagList( elem.getNormalizedAlbum(),
                                                        elem.getNormalizedTitle(),
                                                        elem.getNormalizedPerformer() );
        if (tagList.size() > 0) {
            std::string tagString;
            for (const auto& str : tagList) {
                tagString += "<" + TagConverter::getTagName(str) + ">";
            }
            logger(Level::debug) << "adding tags ("<<tagString<<") to "<<elem.toString()<<"\n";
            elem.setTags(std::move(tagList));
        }
    }


}

const CoverElement &Id3Repository::getCover(const boost::uuids::uuid& coverUid) const {

    if (auto elem = m_simpleCoverDatabase.getCover(coverUid)) {
        return elem->get();
    }

    return emptyElement;
}

bool CoverElement::isConnectedToUid(const boost::uuids::uuid &uid) const {
    return std::find_if(std::cbegin(uidListForCover), std::cend(uidListForCover),
                        [&uid](const boost::uuids::uuid& elem) { return uid == elem; } ) != std::cend(uidListForCover);
}

bool CoverDatabase::addCover(std::vector<char> &&rawData, const boost::uuids::uuid &_uid) {
    auto uid {_uid};
    std::size_t hash = Common::genHash(rawData);

    auto it = std::find_if(std::begin(m_simpleCoverDatabase), std::end(m_simpleCoverDatabase),
                           [&hash](const CoverElement& elem) { return hash == elem.hash; } );

    // is there a cover with this hash?
    if (it != std::cend(m_simpleCoverDatabase)) {
        // add the current audio uid
        it->insertNewUid(std::move(uid));
        return true;
    }

    // if cover not found (by hash), create new cover element
    CoverElement elem;
    elem.hash = hash;
    elem.rawData = move(std::move(rawData));
    elem.insertNewUid(std::move(uid));

    m_simpleCoverDatabase.emplace_back(std::move(elem));

    return true;
}

bool CoverDatabase::readCache(const std::string &coverCacheFile)
{
    // read image cache information
    auto coverDatabaseList = coverFromJson(coverCacheFile);
    for (auto& elem : coverDatabaseList) {
        boost::filesystem::path filename = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Cache)
                + "/" + std::to_string(elem.hash) + ".cache";
        if (boost::filesystem::exists(filename)) {
            logger(Level::debug) << "reading file: " << filename <<"\n";
            std::ifstream ofs(filename.c_str(), std::ios::binary);
            ofs.unsetf(std::ios::skipws);
            if (ofs.good()) {

                std::streampos fileSize;

                ofs.seekg(0, std::ios::end);
                fileSize = ofs.tellg();
                ofs.seekg(0, std::ios::beg);

                // reserve capacity
                elem.rawData.reserve(static_cast<std::vector<uint8_t>::size_type>(fileSize));
                elem.rawData.insert(elem.rawData.begin(),
                                    std::istream_iterator<uint8_t>(ofs),
                                    std::istream_iterator<uint8_t>());

                m_simpleCoverDatabase.emplace_back(std::move(elem));

            }
            else {
                logger(Level::warning) << "cannot read file <"<<filename<<">\n";
                return false;
            }
        }
        else {
            logger(Level::warning) << "file <"<<filename<<"> does not exist\n";
        }

    }

    return true;
}

bool CoverDatabase::writeCache(const std::string &coverCacheFile) {
    // write cover database

    logger(Level::info) << "writing <" << m_simpleCoverDatabase.size() << "> cover data entries\n";
    uint64_t entryCounter {0};
    std::for_each(std::begin(m_simpleCoverDatabase), std::end(m_simpleCoverDatabase),
                  [&entryCounter](const auto& elem) { entryCounter += elem.uidListForCover.size(); } );
    logger(Level::info) << "medial number of items with same cover: " << entryCounter/m_simpleCoverDatabase.size() << "\n";
    auto jsonCover = coverToJson(m_simpleCoverDatabase);
    if (jsonCover)
        writeJson(std::move(*jsonCover), coverCacheFile);

    logger(Level::info) << "writing cover cache files\n";

    for (const auto& elem : m_simpleCoverDatabase) {
        boost::filesystem::path filename = Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Cache) +
                "/" + std::to_string(elem.hash) + ".cache";
        // write image if file does not exists
        if (!boost::filesystem::exists(filename)) {
            logger(Level::debug) << "creating file: " << filename <<"\n";
            std::ofstream ofs(filename.c_str(), std::ios::binary);
            if (ofs.good()) {
                for(const auto& val : elem.rawData)
                    ofs << val;
            }
            else {
                logger(Level::warning) << "cannot create file <"<<filename<<">\n";
                return false;
            }
        }

    }

    return true;
}
