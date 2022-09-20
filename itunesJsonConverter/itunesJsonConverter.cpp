#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include "common/NameGenerator.h"
#include "common/Constants.h"
#include "boost/lexical_cast.hpp"

struct PodcastEntry {
    std::string titleName;
    std::string titleNo;
    std::string streamUrl;
};

struct PodcastData {
    std::string albumName;
    std::string performerName;
    std::string coverUrl;

    std::vector<PodcastEntry> titleList;

};

int main(int argc, char* argv[])
{

    if (argc != 2) {
        std::cerr << "usage: "<<argv[0]<< " inputFile\n";
        return EXIT_FAILURE;
    }
    std::ifstream ifs(argv[1]);
    nlohmann::json itunes = nlohmann::json::parse(ifs);

    PodcastData list;

    try {

        list.albumName = itunes.at("channel").at("title");
        list.performerName = itunes.at("channel").at("author");
        list.coverUrl = itunes.at("channel").at("image")[0].at("url");

        for(auto i : itunes.at("channel").at("item")) {
            PodcastEntry entry;
            std::cout << " ------\n";
            entry.titleName = i.at("title")[1] ;
            entry.streamUrl = i.at("enclosure").at("@url");
            entry.titleNo =  i.at("episode");
            list.titleList.emplace_back(std::move(entry));
        }

    } catch (nlohmann::json::exception& ex) {
        std::cerr << "parsing error: " << ex.what();
    }

    if (list.titleList.empty()) {
        std::cerr << "error, no entries given - giving up\n";
        return EXIT_FAILURE;
    }

    // now, when everything is gathered, generate files

    for(const auto& entry : list.titleList) {
        // TODO: make this generic with audioserver

        nlohmann::json json;
        std::string uid = Common::NameGenerator::create("","").unique_id;
        json[ServerConstant::JsonField::uid] = uid;
        json[ServerConstant::JsonField::title] = entry.titleName;
        json[ServerConstant::JsonField::album] = list.albumName;
        json[ServerConstant::JsonField::performer] = list.performerName;
        json[ServerConstant::JsonField::extension] = ".png";
        json[ServerConstant::JsonField::albumCreation] = true;
        json[ServerConstant::JsonField::trackNo] = boost::lexical_cast<int>(entry.titleNo);
        json[ServerConstant::JsonField::allTrackNo] = list.titleList.size(); // BEWARE: this is best guess
        json[ServerConstant::JsonField::url] = entry.streamUrl;
        json[ServerConstant::JsonField::imageUrl] = list.coverUrl;

        std::string filename (list.performerName+"_"+entry.titleNo+".json");
        std::ofstream ofs(filename);
        ofs << json.dump(2);
        ofs.close();
    }

    sync();

    return EXIT_SUCCESS;
}
