#include "databaseaccess.h"
#include "common/logger.h"
#include <nlohmann/json.hpp>
#include "database/SimpleDatabase.h"

using namespace LoggerFramework;

std::string DatabaseAccess::convertToJson(const std::optional<std::vector<Id3Info>> list) {

    nlohmann::json json;
    if (list) {
        for(auto item : list.value()) {
            nlohmann::json jentry;
            jentry[std::string(ServerConstant::Parameter::Database::uid)] = item.uid;
            jentry[std::string(ServerConstant::Parameter::Database::interpret)] = item.performer_name;
            jentry[std::string(ServerConstant::Parameter::Database::album)] = item.album_name;
            jentry[std::string(ServerConstant::Parameter::Database::titel)] = item.title_name;
            std::string relativCoverPath =
                    Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::CoversRelative) +
                    "/" + item.uid + item.fileExtension;
            jentry[std::string(ServerConstant::Parameter::Database::imageFile)] = relativCoverPath;
            jentry[std::string(ServerConstant::Parameter::Database::trackNo)] = item.track_no;
            json.push_back(jentry);
        }
    }
    return json.dump(2);
}

std::string DatabaseAccess::convertToJson(const std::vector<Database::Playlist>& list) {

    nlohmann::json json;

    try {
        for(auto item : list) {
            nlohmann::json jentry;
            jentry[std::string(ServerConstant::Parameter::Database::uid)] = item.getUniqueID();
            jentry[std::string(ServerConstant::Parameter::Database::album)] = item.getName();
            jentry[std::string(ServerConstant::Parameter::Database::interpret)] = item.getPerformer();
            jentry[std::string(ServerConstant::Parameter::Database::titel)] = "";
            std::string relativCoverPath = item.getCover();
            jentry[std::string(ServerConstant::Parameter::Database::imageFile)] = relativCoverPath;
            jentry[std::string(ServerConstant::Parameter::Database::trackNo)] = 0;
            json.push_back(jentry);
        }

    } catch (const nlohmann::json::exception& ex) {
        logger(Level::error) << "conversion to json failed: " << ex.what();
    }

    return json.dump();
}

std::string DatabaseAccess::access(const utility::Extractor::UrlInformation &urlInfo) {

    if (!urlInfo) {
        logger(Level::warning) << "invalid url given for database access\n";
        return R"({"result": "illegal url given" })";
    }

    logger(Level::info) << "database access - parameter:<"<<urlInfo->parameter<<"> value:<"<<urlInfo->value<<">\n";
    if (urlInfo->parameter == ServerConstant::Command::getAlbumList) {
        // get all albums and sort/reduce
        auto infoList = m_database.searchPlaylistItems(urlInfo->value, Database::SearchAction::alike);

        return convertToJson(infoList);
    }

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::overall )
        return convertToJson(m_database.searchAudioItems(urlInfo->value, Database::SearchItem::overall, Database::SearchAction::exact));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::interpret )
        return convertToJson(m_database.searchAudioItems(urlInfo->value, Database::SearchItem::interpret, Database::SearchAction::exact));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::titel )
        return convertToJson(m_database.searchAudioItems(urlInfo->value, Database::SearchItem::titel, Database::SearchAction::exact));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::album )
        return convertToJson(m_database.searchAudioItems(urlInfo->value, Database::SearchItem::album, Database::SearchAction::exact));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::uid ){
        auto uidData = m_database.searchAudioItems(urlInfo->value, Database::SearchItem::uid , Database::SearchAction::uniqueId);
        logger(Level::debug) << "audio item uid found <"<<uidData.size()<<"> elements\n";
        if (uidData.size()>0)
            return convertToJson(uidData);

        auto plData = m_database.searchPlaylistItems(urlInfo->value);
        logger(Level::debug) << "playlist uid found <"<<uidData.size()<<"> elements\n";
        if (plData.size() > 0)
            return convertToJson(plData);
    }

    return "[]";
}
