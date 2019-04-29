#ifndef SERVER_SIMPLEDATABASE_H
#define SERVER_SIMPLEDATABASE_H

#include <vector>
#include <string>
#include <boost/beast.hpp>
#include "Id3Info.h"
#include "common/Extractor.h"
#include <boost/filesystem.hpp>

namespace filesys =  boost::filesystem;
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

class SimpleDatabase {

    std::vector<Id3Info> simpleDatabase;

public:
    SimpleDatabase() {}

    enum class DatabaseSearchType {
        unknown,
        titel,
        album,
        interpret,
        overall
    };

    std::vector<Id3Info> findInDatabase(const std::string &what, DatabaseSearchType type) {

        std::vector<Id3Info> findData;

        std::for_each(std::begin(simpleDatabase), std::end(simpleDatabase),
                      [&what, &findData, type](const Id3Info &info) {
                          if (((type == DatabaseSearchType::titel || type == DatabaseSearchType::overall) &&
                               (info.titel_name.find(what) != std::string::npos)) ||
                              ((type == DatabaseSearchType::album || type == DatabaseSearchType::overall) &&
                               (info.album_name.find(what) != std::string::npos)) ||
                              ((type == DatabaseSearchType::interpret || type == DatabaseSearchType::overall) &&
                               (info.performer_name.find(what) != std::string::npos)))
                              findData.push_back(info);
                      });

        return findData;

    }

    void emplace_back(Id3Info &&info) {
        simpleDatabase.emplace_back(info);
    }

    boost::optional<std::tuple<std::string, DatabaseSearchType>> findInDatabaseToJson(const std::string &path) {

        boost::optional<std::tuple<std::string, DatabaseSearchType>> jsonList;

        /* is this a REST find request? */
        auto urlInfo = utility::Extractor::getUrlInformation(path);

        if (urlInfo && urlInfo->command == "find") {

            std::cerr << "find request with <" << urlInfo->parameter << "> of value <" << urlInfo->value << ">\n";
            boost::beast::error_code ec;
            http::string_body::value_type body;

            DatabaseSearchType type{DatabaseSearchType::unknown};

            if (urlInfo->parameter == "titel") {
                type = DatabaseSearchType::titel;
            }
            if (urlInfo->parameter == "album") {
                type = DatabaseSearchType::album;
            }
            if (urlInfo->parameter == "interpret") {
                type = DatabaseSearchType::interpret;
            }
            if (urlInfo->parameter == "overall") {
                type = DatabaseSearchType::overall;
            }

            if (type != DatabaseSearchType::unknown) {
                auto list = findInDatabase(urlInfo->value, type);

                // convert to json
                nlohmann::json json;
                nlohmann::json jsonArray;

                std::for_each(std::begin(list), std::end(list), [&jsonArray](const Id3Info &info) {
                    nlohmann::json jsonListEntry;
                    jsonListEntry["titel"] = info.titel_name;
                    jsonListEntry["album"] = info.album_name;
                    jsonListEntry["performer"] = info.performer_name;
                    jsonListEntry["track"] = info.track_no;
                    jsonListEntry["uuid"] = info.filename;
                    jsonArray.push_back(jsonListEntry);
                });

                json["list"] = jsonArray;
                jsonList = std::make_tuple(json.dump(2), type);
            } else {
                jsonList = std::make_tuple(std::string(), type);
            }
        }
        return jsonList;
    }

    std::vector<std::string> getAllFilesInDir(const std::string &dirPath) {

        // Create a vector of string
        std::vector<std::string> listOfFiles;
        try {
            // Check if given path exists and points to a directory
            if (filesys::exists(dirPath) && filesys::is_directory(dirPath)) {
                // Create a Directory Iterator object and points to the starting of directory
                filesys::directory_iterator iter(dirPath);

                // Create a Directory Iterator object pointing to end.
                filesys::directory_iterator end;

                // Iterate till end
                while (iter != end) {
                    // Check if current entry is a directory and if exists in skip list
                    if (!filesys::is_directory(iter->path())) {
                        // Add the name in vector
                        listOfFiles.push_back(iter->path().string());
                    }

                    boost::system::error_code ec;
                    // Increment the iterator to point to next entry in recursive iteration
                    iter.increment(ec);
                    if (ec) {
                        std::cerr << "Error While Accessing : " << iter->path().string() << " :: " << ec.message()
                                  << '\n';
                    }
                }
            }
        }
        catch (std::system_error &e) {
            std::cerr << "Exception :: " << e.what();
        }
        return listOfFiles;
    }

    void loadDatabase(const std::string &mp3Directory) {

        std::vector<std::string> filelist = getAllFilesInDir(mp3Directory);
        Id3Reader id3Reader;

        for (auto it{filelist.begin()}; it != filelist.end(); ++it) {
            std::cerr << " read <" << *it << ">\n";
            simpleDatabase.emplace_back(id3Reader.getInfo(*it));
        }

    }

};


#endif //SERVER_SIMPLEDATABASE_H
