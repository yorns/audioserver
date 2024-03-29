#include "databaseaccess.h"
#include "common/logger.h"
#include <nlohmann/json.hpp>
#include "database/SimpleDatabase.h"

using namespace LoggerFramework;

std::string DatabaseAccess::convertToJson(const std::optional<std::vector<Id3Info>> list) {

    nlohmann::json json;
    if (list) {
        if (list->empty()){
            return R"([])";
        }
        for(auto item : list.value()) {
            nlohmann::json jentry;
            std::string urlAudioFile {item.urlAudioFile};
            if (item.urlAudioFile.length() > ServerConstant::fileprefix.length() &&
                    item.urlAudioFile.substr(0,ServerConstant::fileprefix.length()) == ServerConstant::fileprefix) {
                std::string extension = std::string(ServerConstant::mp3Extension); // default use mp3
                if (auto dotPos = item.urlAudioFile.find_last_of('.')) {
                    extension = item.urlAudioFile.substr(dotPos);
                }
                urlAudioFile = std::string(ServerConstant::audioPath) + "/" + boost::uuids::to_string(item.uid) + extension;
            }
            jentry[ServerConstant::JsonField::uid] = boost::uuids::to_string(item.uid);
            jentry[ServerConstant::JsonField::performer] = item.performer_name;
            jentry[ServerConstant::JsonField::album] = item.album_name;
            jentry[ServerConstant::JsonField::title] = item.title_name;
            jentry[ServerConstant::JsonField::imageUrl] = item.urlCoverFile;
            jentry[ServerConstant::JsonField::trackNo] = item.track_no;
            jentry[ServerConstant::JsonField::audioUrl] = urlAudioFile;
            json.push_back(jentry);
        }
    }
    else {
        return R"([])";
    }
    //logger(Level::info) << json.dump(2)<<"\n";
    return json.dump(2);
}

std::string DatabaseAccess::convertToJson(const std::vector<Database::Playlist>& list) {

    nlohmann::json json;

    try {
        if (list.empty()) {
            return R"([])";
        }
        for(auto item : list) {
            nlohmann::json jentry;
            jentry[ServerConstant::JsonField::uid] = boost::uuids::to_string(item.getUniqueID());
            jentry[ServerConstant::JsonField::album] = item.getName();
            jentry[ServerConstant::JsonField::performer] = item.getPerformer();
            jentry[ServerConstant::JsonField::title] = "";
            std::string relativCoverPath = item.getCover();
            jentry[ServerConstant::JsonField::cover] = relativCoverPath;
            jentry[ServerConstant::JsonField::trackNo] = 0;
            jentry[ServerConstant::JsonField::url] = "";

            json.push_back(jentry);
        }

    } catch (const nlohmann::json::exception& ex) {
        logger(Level::error) << "conversion to json failed: " << ex.what();
        return R"([])";
    }

    // logger(Level::info) << "json data:\n" << json.dump(2)<<"\n";
    return json.dump(2);
}

std::optional<std::string> DatabaseAccess::extractUuidFromTarget(std::string_view target)
{
    auto pos1 = target.find_last_of("/");
    if (pos1 != std::string_view::npos) {
        auto pos2 = target.find_last_of(".");
        {
            return std::string(target.substr(pos1+1, pos2-pos1-1));
        }
    }

    return std::nullopt;
}

std::string DatabaseAccess::access(const utility::Extractor::UrlInformation &urlInfo) {

    if (!urlInfo || !urlInfo->m_parsed || urlInfo->m_parameterList.size() != 1) {
        logger(Level::warning) << "invalid url given for database access\n";
        return R"({"result": "illegal url given" })";
    }

    auto& parameter = urlInfo->m_parameterList.at(0).name;
    auto& value = urlInfo->m_parameterList.at(0).value;
    auto command = urlInfo->getCommand();

    logger(Level::info) << "database access cmd <"<<command<<"> parameter:<"<<parameter<<"> value:<"<<value<<">\n";

    if (parameter == ServerConstant::Command::getAlbumList) {
        auto list = m_database->searchPlaylistItems(value, Database::SearchAction::alike);

        logger(Level::info) << "found a list of <"<<list.size()<< "> elements\n";

        std::sort(std::begin(list), std::end(list), [](Database::Playlist& item1, Database::Playlist& item2) { return item1.getUniqueID() > item2.getUniqueID(); });

        return convertToJson(list);
    }

    if ( parameter == ServerConstant::Parameter::Database::overall )
        return convertToJson(m_database->searchAudioItems(value, Database::SearchItem::overall, Database::SearchAction::exact));

    if ( parameter == ServerConstant::Parameter::Database::performer )
        return convertToJson(m_database->searchAudioItems(value, Database::SearchItem::performer, Database::SearchAction::exact));

    if ( parameter == ServerConstant::Parameter::Database::title )
        return convertToJson(m_database->searchAudioItems(value, Database::SearchItem::title, Database::SearchAction::exact));

    if ( parameter == ServerConstant::Parameter::Database::album )
        return convertToJson(m_database->searchAudioItems(value, Database::SearchItem::album, Database::SearchAction::exact));

    if ( parameter == ServerConstant::Parameter::Database::uid ){
        auto uidData = m_database->searchAudioItems(value, Database::SearchItem::uid , Database::SearchAction::uniqueId);
        logger(Level::debug) << "audio item uid found <"<<uidData.size()<<"> elements\n";
        if (uidData.size()>0) {
            auto retJson = convertToJson(uidData);
            logger(Level::info) << retJson;
            return retJson;
        }
    }

    if (parameter == ServerConstant::Parameter::Database::playlist ) {
        auto plData = m_database->searchPlaylistItems(value);
        logger(Level::debug) << "playlist uid found <"<<plData.size()<<"> elements\n";
        if (plData.size() > 0)
            return convertToJson(plData);
    }

    return "[]";
}

std::optional<std::vector<char> > DatabaseAccess::getVirtualFile(const std::string_view &target) const {
    logger(LoggerFramework::Level::debug) << "searching for file: "<<target<<"\n";
    auto pos1 = target.find_last_of("/");
    if (pos1 != std::string_view::npos) {
        auto pos2 = target.find_last_of(".");
        if (pos2 != std::string_view::npos) {
            std::string uidStr = std::string(target.substr(pos1+1, pos2-pos1-1));
            logger(LoggerFramework::Level::debug) << "searching for cover UID <"<<uidStr<<">\n";
            boost::uuids::uuid uid;
            try {
                uid = boost::lexical_cast<boost::uuids::uuid>(uidStr);
            } catch(std::exception& ex) {
                logger(Level::warning) << "could not interpret <"<<uidStr<<">: "<< ex.what()<<"\n";
                return std::nullopt;
            }

            return m_database->getCover(std::move(uid));
        }
    }
    return std::nullopt;
}

std::optional<std::vector<char> > DatabaseAccess::virtualImageHandler(const std::string_view &_target)  {
    // split target
    auto target = utility::toJson(std::string(_target));
    logger(Level::debug) << "test for virtual image request for <"<<target<<">\n";

    if (testUrlPath(target, ServerConstant::imagePath)) {
        logger(LoggerFramework::Level::debug) << "searching for cover at <"<<target<<">\n";
        if (auto uidStr = extractUuidFromTarget(target)) {
            logger(LoggerFramework::Level::debug) << "searching for cover UID <"<<*uidStr<<">\n";
            boost::uuids::uuid uid;
            try {
                uid = boost::lexical_cast<boost::uuids::uuid>(*uidStr);
            } catch(std::exception& ex) {
                logger(Level::warning) << "could not interpret <"<<*uidStr<<">: "<< ex.what()<<"\n";
                return std::nullopt;
            }

            return getDatabase()->getCover(std::move(uid));
        }
        else {
            boost::uuids::uuid unknownCover;
            try {
                unknownCover = boost::lexical_cast<boost::uuids::uuid>(ServerConstant::unknownCoverFileUid);
            } catch(std::exception& ex) {
                logger(Level::error) << " ERROR: cannot convert unknown cover uuid string: " << ex.what() << "\n";
            }
            return getDatabase()->getCover(unknownCover);
        }
    }
    return std::nullopt;
}

std::optional<std::string> DatabaseAccess::virtualAudioHandler(const std::string_view &_target)  {

    auto target = utility::toJson(std::string(_target));
    logger(Level::debug) << "test virtual audio request for <"<<target<<">\n";

    if (testUrlPath(target, ServerConstant::audioPath)) {
        if (auto uidStr = extractUuidFromTarget(target)) {
            logger(LoggerFramework::Level::debug) << "searching for audio UID <"<<*uidStr<<">\n";
            boost::uuids::uuid uid;
            try {
                uid = boost::lexical_cast<boost::uuids::uuid>(*uidStr);
            } catch(std::exception& ex) {
                logger(Level::warning) << "could not interpret <"<<*uidStr<<">: "<< ex.what()<<"\n";
                return std::nullopt;
            }

            return getDatabase()->getFileFromUUID(uid);
        }

    }
    return std::nullopt;
}

std::optional<std::string> DatabaseAccess::virtualPlaylistHandler(const std::string_view &_target) {

    auto target = utility::toJson(std::string(_target));
    logger(Level::debug) << "virtual playlist request for <"<<target<<">\n";

    if (testUrlPath(target, ServerConstant::playlistPath)) {
        if (auto uidStr = extractUuidFromTarget(target)) {
            logger(LoggerFramework::Level::debug) << "searching for playlist UID <"<<*uidStr<<">\n";
            boost::uuids::uuid uid;
            try {
                uid = boost::lexical_cast<boost::uuids::uuid>(*uidStr);
            } catch(std::exception& ex) {
                logger(Level::warning) << "could not interpret <"<<*uidStr<<">: "<< ex.what()<<"\n";
                return std::nullopt;
            }
            return getDatabase()->getM3UPlaylistFromUUID(uid);
        }
    }
    return std::nullopt;
}
