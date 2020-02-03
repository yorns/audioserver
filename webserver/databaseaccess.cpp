#include "databaseaccess.h"


std::string DatabaseAccess::convertToJson(const std::vector<Id3Info> list) {
    nlohmann::json json;
    for(auto item : list) {
        nlohmann::json jentry;
        jentry[ServerConstant::Parameter::Database::uid.to_string()] = item.uid;
        jentry[ServerConstant::Parameter::Database::interpret.to_string()] = item.performer_name;
        jentry[ServerConstant::Parameter::Database::album.to_string()] = item.album_name;
        jentry[ServerConstant::Parameter::Database::titel.to_string()] = item.titel_name;
        jentry[ServerConstant::Parameter::Database::imageFile.to_string()] = item.imageFile;
        jentry[ServerConstant::Parameter::Database::trackNo.to_string()] = item.track_no;
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

    return "[]";
}
