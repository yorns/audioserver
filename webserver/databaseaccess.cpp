#include "databaseaccess.h"
#include "common/logger.h"
#include <nlohmann/json.hpp>

using namespace LoggerFramework;

std::string DatabaseAccess::convertToJson(const std::vector<Id3Info> list) {
    nlohmann::json json;
    for(auto item : list) {
        nlohmann::json jentry;
        jentry[std::string(ServerConstant::Parameter::Database::uid)] = item.uid;
        jentry[std::string(ServerConstant::Parameter::Database::interpret)] = item.performer_name;
        jentry[std::string(ServerConstant::Parameter::Database::album)] = item.album_name;
        jentry[std::string(ServerConstant::Parameter::Database::titel)] = item.titel_name;
        jentry[std::string(ServerConstant::Parameter::Database::imageFile)] = item.imageFile;
        jentry[std::string(ServerConstant::Parameter::Database::trackNo)] = item.track_no;
        json.push_back(jentry);
    }
    return json.dump(2);
}

std::string DatabaseAccess::access(const utility::Extractor::UrlInformation &urlInfo) {

    logger(Level::debug) << "database access - parameter:"<<urlInfo->parameter<<" value:"<<urlInfo->value<<"\n";
    if (urlInfo->parameter == ServerConstant::Command::getAlbumList) {
        auto infoList = m_database.findAlbum(urlInfo->value);
        return convertToJson(infoList);
    }

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::overall )
        return convertToJson(m_database.findInDatabase(urlInfo->value, SimpleDatabase::DatabaseSearchType::overall));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::interpret )
        return convertToJson(m_database.findInDatabase(urlInfo->value, SimpleDatabase::DatabaseSearchType::interpret));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::titel )
        return convertToJson(m_database.findInDatabase(urlInfo->value, SimpleDatabase::DatabaseSearchType::titel));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::album )
        return convertToJson(m_database.findInDatabase(urlInfo->value, SimpleDatabase::DatabaseSearchType::album));

    if ( urlInfo->parameter == ServerConstant::Parameter::Database::uid ){
        logger(Level::debug) << "uid found <"<<m_database.findInDatabase(urlInfo->value, SimpleDatabase::DatabaseSearchType::uid).size()<<"> elements\n";
        return convertToJson(m_database.findInDatabase(urlInfo->value, SimpleDatabase::DatabaseSearchType::uid));
    }

    return "[]";
}
