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
            jentry[std::string(ServerConstant::Parameter::Database::imageFile)] = item.imageFile;
            jentry[std::string(ServerConstant::Parameter::Database::trackNo)] = item.track_no;
            json.push_back(jentry);
        }
    }
    return json.dump(2);
}

std::string DatabaseAccess::access(const utility::Extractor::UrlInformation &urlInfo) {

    logger(Level::debug) << "database access - parameter:"<<urlInfo->parameter<<" value:"<<urlInfo->value<<"\n";
    if (urlInfo->parameter == ServerConstant::Command::getAlbumList) {
        auto infoList = m_database.search(urlInfo->value, Database::SearchItem::album, Database::SearchAction::alike, Common::FileType::Audio);
        return convertToJson(infoList);
    }

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::overall )
        return convertToJson(m_database.search(urlInfo->value, Database::SearchItem::overall));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::interpret )
        return convertToJson(m_database.search(urlInfo->value, Database::SearchItem::interpret));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::titel )
        return convertToJson(m_database.search(urlInfo->value, Database::SearchItem::titel));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::album )
        return convertToJson(m_database.search(urlInfo->value, Database::SearchItem::album));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::uid ){
        auto uidData = m_database.search(urlInfo->value, Database::SearchItem::uid);
        if (uidData)
            logger(Level::debug) << "uid found <"<<uidData->size()<<"> elements\n";
        return convertToJson(uidData);
    }

    return "[]";
}
