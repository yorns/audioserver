#include "playlistcontainer.h"

#include <boost/filesystem.hpp>
#include <functional>
#include <boost/uuid/uuid_io.hpp>
#include <stack>
#include <variant>
#include <exception>

#include "common/filesystemadditions.h"
#include "common/logger.h"
#include "common/NameGenerator.h"
#include "id3repository.h"

using namespace Database;
using namespace Common;
using namespace LoggerFramework;

void PlaylistContainer::addPlaylist(Playlist &&playlist) {
    m_playlists.emplace_back(playlist);
}

bool PlaylistContainer::addItemToPlaylistName(const std::string &playlistName, boost::uuids::uuid &&audioUniqueId) {
    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists), [&playlistName](const Playlist& item) {
        return item.getName() == playlistName;
    });

    if (it != std::end(m_playlists)) {
        it->addToList(std::move(audioUniqueId));
        return true;
    }

    return false;
}

bool PlaylistContainer::addItemToPlaylistUID(const boost::uuids::uuid &playlistUniqueID, boost::uuids::uuid &&audioUniqueId) {
    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists), [&playlistUniqueID](const Playlist& item) {
        return item.getUniqueID() == playlistUniqueID;
    });

    if (it != std::end(m_playlists)) {
        it->addToList(std::move(audioUniqueId));
        return true;
    }

    return false;
}

bool PlaylistContainer::removePlaylistUID(const boost::uuids::uuid &playlistUniqueId) {
    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists),
                           [&playlistUniqueId](const Playlist& pl)
    { return (pl.getUniqueID() == playlistUniqueId); });

    if (it != std::end(m_playlists) ) {
        m_playlists.erase(it);
        try {
            auto rmResult = FileSystemAdditions::removeFile(FileType::PlaylistM3u, boost::lexical_cast<std::string>(playlistUniqueId));
            return rmResult;
        } catch (std::exception& ex) {
            logger(LoggerFramework::Level::warning) << ex.what() << "\n";
            return false;
        }
    }
    return false;
}

bool PlaylistContainer::removePlaylistName(const std::string &playlistName) {
    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists),
                           [&playlistName](const Playlist& pl)
    { return (pl.getName() == playlistName); });

    if (it != std::end(m_playlists) ) {
        auto playlistUniqueId { it->getUniqueID() };
        m_playlists.erase(it);
        try {
            auto rmFile = FileSystemAdditions::removeFile(FileType::PlaylistM3u, boost::lexical_cast<std::string>(playlistUniqueId));
            return rmFile;
        } catch (std::exception& ex) {
            logger(LoggerFramework::Level::warning) << ex.what() << "\n";
            return false;
        }

    }
    return false;
}

bool PlaylistContainer::writeChangedPlaylists() {

    bool success{true};

    logger(Level::debug) << "writing playlists if necessary\n";

    for (auto &elem : m_playlists) {
        if (!elem.write()) {
            logger(Level::warning) << "playlist file <"<<boost::uuids::to_string(elem.getUniqueID())<<".m3u> could not be saved";
        }
    }

    system("sync");

    return success;
}

std::optional<std::string> PlaylistContainer::convertName(const boost::uuids::uuid &uid) {
    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists), [&uid] (const Playlist& playlist) {
        return playlist.getUniqueID() == uid;
    });
    if (it != std::end(m_playlists))
        return it->getName();
    else
        return std::nullopt;

}

std::optional<boost::uuids::uuid> PlaylistContainer::convertName(const std::string &name) {

    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists), [&name] (const Playlist& playlist) {
        return playlist.getName() == name;
    });
    if (it != std::end(m_playlists))
        return it->getUniqueID();

    return std::nullopt;
}

bool PlaylistContainer::readPlaylistsM3U() {

    // read m3u actually not supported
    return false;
    //    auto fileList = FileSystemAdditions::getAllFilesInDir(FileType::PlaylistM3u);

    //    for (auto file : fileList) {
    //        logger(Level::debug) << "reading m3u playlist <"<<file.name<<">\n";
    //        std::string fileName = FileSystemAdditions::getFullQualifiedDirectory(FileType::PlaylistM3u) + '/';
    //        fileName += file.name + file.extension;
    //        if (file.extension == ".m3u") {
    //            Playlist playlist(file.name, ReadType::isM3u, Persistent::isPermanent, Changed::isUnchanged);
    //            if (playlist.readM3u()) {
    //                m_playlists.emplace_back(playlist);
    //            }
    //            else
    //                logger(Level::warning) << "reading file <"<<file.name+file.extension <<"> failed\n";
    //        }
    //    }

    //    return true;
}

bool PlaylistContainer::readPlaylistsJson(Database::FindAlgo&& findUuidList,
                                          Database::InsertCover&& coverInsert) {
    auto streamList = FileSystemAdditions::getAllFilesInDir(FileType::PlaylistJson);

    for (auto file : streamList) {
        logger(Level::debug) << "reading json playlist <"<<file.name<<">\n";
        std::string fileName = FileSystemAdditions::getFullQualifiedDirectory(FileType::PlaylistJson) + '/';
        fileName += file.name + file.extension;
        if (file.extension == ".json") {
            Playlist playlist(std::move(fileName), ReadType::isJson, Persistent::isPermanent, Changed::isConst);
            if (playlist.readJson(std::move(findUuidList), std::move(coverInsert))) {
                m_playlists.emplace_back(playlist);
            }
            else
                logger(Level::warning) << "reading file <"<<file.name+file.extension <<"> failed\n";
        }
    }

    return true;
}

bool PlaylistContainer::insertAlbumPlaylists(const std::vector<AlbumListEntry> &albumList) {

    for(auto& elem : albumList) {
        try {
            auto uniqueID = boost::lexical_cast<boost::uuids::uuid>(NameGenerator::create("", "").unique_id);
            Playlist playlist("", ReadType::isM3u, Persistent::isTemporal);
            playlist.setName(elem.m_name);
            playlist.setPerformer(elem.m_performer);
            playlist.setCover(elem.m_coverUrl);
            playlist.setUniqueID(std::move(uniqueID));
            for(auto& uid : elem.m_playlist) {
                auto addUid = std::get<boost::uuids::uuid>(uid);
                playlist.addToList(std::move(addUid));
            }
            m_playlists.emplace_back(playlist);
        } catch (std::exception& ex) {
            logger(LoggerFramework::Level::warning) << ex.what() << "\n";
        }
    }

    return true;
}

void PlaylistContainer::insertTagsFromItems(const Database::Id3Repository& repository) {
    for(auto& elem : m_playlists) {
        for (const auto& item : elem.getUniqueAudioIdsPlaylist()) {
            if ( auto id3Info = repository.getId3InfoByUid(item)) {
                elem.setTagList(id3Info->tags);
            }
        }
    }
}

void PlaylistContainer::addTags(const std::vector<Tag>& tagList, const boost::uuids::uuid& playlistID) {
    for(auto& elem : m_playlists) {
        if (elem.getUniqueID() == playlistID) {
            elem.setTagList(tagList);
        }
    }
}

void PlaylistContainer::addTags(const std::vector<Tag>& tagList) {
    for(auto& elem : m_playlists) {
        elem.setTagList(tagList);
    }
}

std::optional<std::string> PlaylistContainer::createvirtual_m3u(const boost::uuids::uuid &playlistUuid)  {

    if (getPlaylistByUID(playlistUuid)) {
        auto list = getPlaylistByUID(playlistUuid)->get();
        //    nlohmann::json virtualPlaylistJson;
        nlohmann::json audioList;
        // std::stringstream m3uOutput;
        if (list.getPlaylist().size() > 0) {
            for (const auto& elem : list.getPlaylist()) {
                std::stringstream id;
                id << ServerConstant::audioPath << "/" << elem << ".mp3";
                audioList.push_back(id.str());
            }
            //logger(Level::debug) << "created list: " << audioList.dump(2) << "\n";
            return audioList.dump(2);

        }
        else {
            logger(Level::info) << "requested virtual playlist not found: " << playlistUuid << "\n";
        }
    }
    return std::nullopt;
}

std::optional<std::reference_wrapper<Playlist>> PlaylistContainer::getPlaylistByName(const std::string& playlistName) {
    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists),
                           [playlistName](const Playlist& elem) { return elem.getName() == playlistName; });

    if (it != std::end(m_playlists)) {
        logger(Level::debug) << "playlist found with <"<<it->getUniqueAudioIdsPlaylist().size()<<"> elements\n";
        return *it;
    }
    return std::nullopt;

}

std::optional<std::reference_wrapper<Playlist>> PlaylistContainer::getPlaylistByUID(const boost::uuids::uuid &uid) {
    auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists),
                           [uid](const Playlist& elem) {
        return elem.getUniqueID() == uid; });

    if (it != std::end(m_playlists)) {
        logger(Level::debug) << "playlist found with <"<<it->getUniqueAudioIdsPlaylist().size()<<"> elements\n";
        return *it;
    }
    return std::nullopt;
}

std::optional<const Playlist> PlaylistContainer::getCurrentPlaylist() const {

    if (m_currentPlaylist) {
        auto it = std::find_if(std::begin(m_playlists), std::end(m_playlists),
                               [this](const Playlist& elem) { return elem.getUniqueID() == m_currentPlaylist.value(); });

        if (it != std::end(m_playlists)) {
            return *it;
        }
    }
    return std::nullopt;
}

std::optional<const boost::uuids::uuid> PlaylistContainer::getCurrentPlaylistUniqueID() const {
    return m_currentPlaylist;
}

bool PlaylistContainer::setCurrentPlaylist(boost::uuids::uuid &&currentPlaylistUniqueId) {
    if (std::find_if(std::begin(m_playlists), std::end(m_playlists),
                     [&currentPlaylistUniqueId](const Playlist& playlist) {
                     return (playlist.getUniqueID() == currentPlaylistUniqueId);})
            != std::end(m_playlists)) {
        m_currentPlaylist = currentPlaylistUniqueId;
        return true;
    }

    logger(Level::warning) << "playlist <" << currentPlaylistUniqueId << "> not found\n";
    return false;
}

void PlaylistContainer::unsetCurrentPlaylist() { m_currentPlaylist.reset(); }

bool PlaylistContainer::isUniqueName(const std::string &name) {
    if (std::find_if(std::begin(m_playlists), std::end(m_playlists),
                     [&name](const Playlist& playlist) { return playlist.getName() == name; })
            != std::end(m_playlists)) {
        return false;
    }
    return true;
}

std::vector<std::pair<std::string, boost::uuids::uuid> > PlaylistContainer::getAllPlaylists() {
    std::vector<std::pair<std::string, boost::uuids::uuid>> list;
    for(auto& elem : m_playlists) {
        list.push_back(std::make_pair(elem.getName(), elem.getUniqueID()));
    }
    return list;
}

std::vector<Playlist> PlaylistContainer::searchPlaylists(const boost::uuids::uuid &whatUid, SearchAction action) {

    std::vector<Playlist> playlist;
    auto playlist_it = std::find_if(std::begin(m_playlists), std::end(m_playlists),
                                    [&whatUid](const Playlist& playlist){ return playlist.getUniqueID() == whatUid; });
    if (playlist_it != std::end(m_playlists))
        playlist.push_back(*playlist_it);

    return playlist;
}

std::vector<Playlist> PlaylistContainer::upn_playlist(std::vector<std::string>& whatList) {

    typedef std::variant<std::string, bool> upn_value;

    std::vector<Playlist> retList;

    if (whatList.size() < 2) {
        return retList;
    }

    std::for_each(std::begin(m_playlists), std::end(m_playlists),
                  [&whatList, &retList](const Playlist& info){

        std::stack<upn_value> stack;

        if (whatList.size() > 1) {

            for (auto& it : whatList) {

                if (it == "&" || it == "|") {

                    if (stack.size() < 2) {
                        //logger (Level::warning) << "evaluation failed: not enough data\n";
                        break;
                    }

                    upn_value value1 = stack.top();
                    stack.pop();

                    upn_value value2 = stack.top();
                    stack.pop();


                    bool evaluate1{false};
                    bool evaluate2{false};


                    if (const bool* tmp = std::get_if<bool>(&value1)) {
                        evaluate1 = *tmp;
                    }
                    else {
                        evaluate1 = (info.getNameLower().find(std::get<std::string>(value1)) != std::string::npos ||
                                info.getPerformerLower().find(std::get<std::string>(value1)) != std::string::npos);

                    }

                    if (const bool* tmp = std::get_if<bool>(&value2)) {
                        evaluate2 = *tmp;
                    }
                    else {
                        evaluate2 = (info.getNameLower().find(std::get<std::string>(value2)) != std::string::npos ||
                                info.getPerformerLower().find(std::get<std::string>(value2)) != std::string::npos);
                    }

                    if (it == "&")
                        stack.push(evaluate1 && evaluate2);

                    if (it == "|")
                        stack.push(evaluate1 || evaluate2);

                    // logger (Level::info) << "push: <"<< (std::get<bool>(stack.top())?"true":"false") << ">\n";

                }
                else {
                    // logger (Level::info) << "push <"<<it<<">\n";
                    stack.push(it);
                }

            }

            if (stack.size() != 1) {
                // logger(Level::warning) << "find mechanism failed (stack is " << stack.size() << "\n";
            } else {
                if (const bool* tmp = std::get_if<bool>(&stack.top())) {
                    if (*tmp == true)
                        retList.push_back(info);
                } else {
                    logger(Level::warning) << "find mechanism failed - no bool evaluation\n";
                }
            }

        }
    });

    return retList;
}


std::vector<Playlist> PlaylistContainer::searchPlaylists(const std::string &what, SearchAction action) {

    std::vector<Playlist> playlist;
    boost::uuids::uuid whatUid;
    bool uidValid { false };
    try {
        whatUid = boost::lexical_cast<boost::uuids::uuid>(what);
        uidValid = true;
    } catch (std::exception& ) {
        uidValid = false;
    }

    switch(action) {
    case SearchAction::exact: {
        logger(Level::info) << "exact seaching for string <" << what << ">\n";
        for(auto playlistItem{std::cbegin(m_playlists)}; playlistItem != std::cend(m_playlists); ++playlistItem) {
            if ( playlistItem->getName() == what ||
                 playlistItem->getPerformer() == what ||
                 (uidValid && playlistItem->getUniqueID() == whatUid)) {
                playlist.push_back(*playlistItem);
            }
        }
        break;
    }
    case SearchAction::alike: {

        auto whatList = Common::extractWhatList(what);

        std::string seperator;
        std::stringstream tmp;
        for(auto whatElem : whatList) {
            tmp << seperator << whatElem;
            seperator = ", ";
        }
        logger(Level::info) << "alike playlist seaching for string <" << what << "> (" << tmp.str() << ")\n";

        bool is_upn {false};

        std::for_each (std::begin(whatList), std::end(whatList), [&is_upn](const std::string& index){
            if (index == "&" || index == "|")
                is_upn = true;
        });

        if(is_upn) {
            playlist = upn_playlist(whatList);
        }
        else {


            for(auto playlistItem{std::cbegin(m_playlists)}; playlistItem != std::cend(m_playlists); ++playlistItem) {
                bool found {true};
                for(auto whatElem : whatList) {
                    if (playlistItem->getNameLower().find(whatElem) == std::string::npos &&
                            playlistItem->getPerformerLower().find(whatElem) == std::string::npos) {
                        // logger(Level::info) << "not found <"<<whatElem <<"> in name or performer\n";
                        found = false;
                        break;
                    }
                }
                if (playlistItem->isTagAlike(whatList)) {
                    logger(Level::info) << "found tag <" << tmp.str() << "> in <" <<playlistItem->getName() <<">\n";
                    found = true;
                }

                if (found)
                    playlist.push_back(*playlistItem);
            }
        }
        break;
    }
    case SearchAction::uniqueId : {
        logger(Level::info) << "uid playlist seaching for string <" << what << ">\n";

        if (uidValid) {
            auto playlist_it = std::find_if(std::begin(m_playlists), std::end(m_playlists),
                                            [&whatUid](const Playlist& playlist){ return playlist.getUniqueID() == whatUid; });
            if (playlist_it != std::end(m_playlists))
                playlist.push_back(*playlist_it);
        }
        break;
    }
    }
    logger(Level::debug) << "searchPlaylist size <"<<playlist.size()<<">\n";
    return playlist;
}
