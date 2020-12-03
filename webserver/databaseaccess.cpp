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
            jentry[std::string(ServerConstant::Parameter::Database::uid)] = boost::uuids::to_string(item.uid);
            jentry[std::string(ServerConstant::Parameter::Database::performer)] = item.performer_name;
            jentry[std::string(ServerConstant::Parameter::Database::album)] = item.album_name;
            jentry[std::string(ServerConstant::Parameter::Database::title)] = item.title_name;
            jentry[std::string(ServerConstant::Parameter::Database::imageFile)] = item.urlCoverFile;
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
            jentry[std::string(ServerConstant::Parameter::Database::uid)] = boost::uuids::to_string(item.getUniqueID());
            jentry[std::string(ServerConstant::Parameter::Database::album)] = item.getName();
            jentry[std::string(ServerConstant::Parameter::Database::performer)] = item.getPerformer();
            jentry[std::string(ServerConstant::Parameter::Database::title)] = "";
            std::string relativCoverPath = item.getCover();
            jentry[std::string(ServerConstant::Parameter::Database::imageFile)] = relativCoverPath;
            jentry[std::string(ServerConstant::Parameter::Database::trackNo)] = 0;
            json.push_back(jentry);
        }

    } catch (const nlohmann::json::exception& ex) {
        logger(Level::error) << "conversion to json failed: " << ex.what();
    }

    return json.dump(2);
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
        auto list = m_database.searchPlaylistItems(value, Database::SearchAction::alike);

        std::sort(std::begin(list), std::end(list), [](Database::Playlist& item1, Database::Playlist& item2) { return item1.getUniqueID() > item2.getUniqueID(); });

        return convertToJson(list);
    }

    if ( parameter == ServerConstant::Parameter::Database::overall )
        return convertToJson(m_database.searchAudioItems(value, Database::SearchItem::overall, Database::SearchAction::exact));

    if ( parameter == ServerConstant::Parameter::Database::performer )
        return convertToJson(m_database.searchAudioItems(value, Database::SearchItem::performer, Database::SearchAction::exact));

    if ( parameter == ServerConstant::Parameter::Database::title )
        return convertToJson(m_database.searchAudioItems(value, Database::SearchItem::title, Database::SearchAction::exact));

    if ( parameter == ServerConstant::Parameter::Database::album )
        return convertToJson(m_database.searchAudioItems(value, Database::SearchItem::album, Database::SearchAction::exact));

    if ( parameter == ServerConstant::Parameter::Database::uid ){
        auto uidData = m_database.searchAudioItems(value, Database::SearchItem::uid , Database::SearchAction::uniqueId);
        logger(Level::debug) << "audio item uid found <"<<uidData.size()<<"> elements\n";
        if (uidData.size()>0)
            return convertToJson(uidData);
    }

    if (parameter == ServerConstant::Parameter::Database::playlist ) {
        auto plData = m_database.searchPlaylistItems(value);
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

            return m_database.getCover(std::move(uid));
        }
    }
    return std::nullopt;
}
