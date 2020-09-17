#include <string>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <fstream>
#include <common/logger.h>

using namespace LoggerFramework;

struct NetworkEntry {
    std::string Ssid;
    std::string Psk;
    NetworkEntry(const std::string& ssid, const std::string& psk)
        : Ssid(ssid), Psk(psk) {}
    NetworkEntry() {}
};

class WifiManager {

    std::vector<NetworkEntry> networkList;

    void removeEntry(const std::string& ssid) {
        auto it = std::find_if(std::begin(networkList), std::end(networkList), [&ssid](const NetworkEntry& entry){
            return ssid == entry.Ssid;
        });
        if (it != std::end(networkList)) {
            networkList.erase(it);
        }
    }

public:
    constexpr static auto jsonFile { "/etc/wpaConfig.json" };

    WifiManager() { readConfigFile(); }

    void receiveWifiConfig(const std::string&) {
        // Todo gather all wifi APs (not strictly necessarry, but for more evaluation
    }

    void readConfigFile() {
        // read wpaFile
        std::ifstream ifs(jsonFile);

        if (!ifs.good()) {
            logger(Level::info) << "cannot open file <" << jsonFile << ">\n";
            return;
        }

        // convert json
        try {
            nlohmann::json wpaJson = nlohmann::json::parse(ifs);

            auto network = wpaJson.at("Networks");
            for (auto& networkEntry : network) {
                NetworkEntry entry;
                entry.Ssid = networkEntry.at("Ssid");
                entry.Psk = networkEntry.at("Psk");
                networkList.emplace_back(std::move(entry));
            }

        } catch (std::exception& ex) {
            std::cerr << "cannot read file: " << jsonFile << " because: " << ex.what() << "\n";
            return;
        }

        ifs.close();

    }

    void writeConfigFile() {

        std::ofstream ofs(jsonFile);

        if (!ofs.good()) {
            logger(Level::info) << "cannot open file <" << jsonFile << ">\n";
            return;
        }

        // convert json
        NetworkEntry wpaConfig;
        auto network = nlohmann::json();
        auto jnetworkList = nlohmann::json::array();
        try {
            for (auto& item : networkList) {
                auto networkEntry = nlohmann::json();
                networkEntry["Ssid"] = item.Ssid;
                networkEntry["Psk"] = item.Psk;
                jnetworkList.push_back(networkEntry);
            }
            network["Networks"] = jnetworkList;
        } catch (std::exception& ex) {
            std::cerr << "cannot write file: " << jsonFile << " because: " << ex.what() << "\n";
            return;
        }

        ofs << network;
        ofs.close();
        system("sync");

    }

    void generateWpaConfig() {
        // actually done in wpa_generator
        system("wpa_generator");
    }

    void setSsid(std::string_view ssid, std::string_view psk) {
        NetworkEntry entry {std::string(ssid), std::string(psk)};
        removeEntry(std::string(ssid));
        networkList.emplace_back(std::move(entry));
        writeConfigFile();
    }



};
