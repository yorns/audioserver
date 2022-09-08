#ifndef DATABASEACCESS_H
#define DATABASEACCESS_H

#include <optional>
#include <vector>
#include <string_view>
#include <boost/uuid/uuid_io.hpp>
#include "database/SimpleDatabase.h"

class DatabaseAccess
{
    //Database::SimpleDatabase& m_database;
    std::shared_ptr<Database::SimpleDatabase> m_database;

    std::string convertToJson(const std::optional<std::vector<Id3Info>> list);
    std::string convertToJson(const std::vector<Database::Playlist>& list);

    std::optional<std::string> extractUuidFromTarget(std::string_view target)
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

    bool testUrlPath(std::string_view url, const std::string& path) {
        if (url.substr(0,2+path.length()) == "/"+path+"/")
            return true;
        else
            return false;
    }


public:
    DatabaseAccess() = delete;
    DatabaseAccess(std::shared_ptr<Database::SimpleDatabase> simpleDatabase): m_database(simpleDatabase) {}

    std::string access(const utility::Extractor::UrlInformation &urlInfo);
    std::optional<std::vector<char>> getVirtualFile(const std::string_view& target) const;

    std::string restAPIDefinition();

    std::shared_ptr<Database::SimpleDatabase> getDatabase() { return m_database; }


    std::optional<std::vector<char>> virtualImageHandler(const std::string_view& _target)  {
        // split target
        auto target = utility::urlConvert(std::string(_target));
        logger(Level::debug) << "test for virtual image request for <"<<target<<">\n";

        if (testUrlPath(target, "img")) {
            logger(LoggerFramework::Level::debug) << "searching for cover at <"<<target<<">\n";
            if (auto uidStr = extractUuidFromTarget(target)) {
                logger(LoggerFramework::Level::info) << "searching for cover UID <"<<*uidStr<<">\n";
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

    std::optional<std::string> virtualAudioHandler(const std::string_view& _target)  {

        auto target = utility::urlConvert(std::string(_target));
        logger(Level::debug) << "test virtual audio request for <"<<target<<">\n";

        if (testUrlPath(target, "audio")) {
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

    std::optional<std::string> virtualPlaylistHandler(const std::string_view& _target) {

        auto target = utility::urlConvert(std::string(_target));
        logger(Level::debug) << "virtual playlist request for <"<<target<<">\n";

        if (testUrlPath(target, "pl")) {
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



};

#endif // DATABASEACCESS_H
