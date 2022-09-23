#include "wifi.h"

void WifiManager::removeEntry(const std::string &ssid) {
    auto it = std::find_if(std::begin(networkList), std::end(networkList), [&ssid](const NetworkEntry& entry){
        return ssid == entry.Ssid;
    });
    if (it != std::end(networkList)) {
        networkList.erase(it);
    }
}

void WifiManager::receiveWifiConfig(const std::string &) {
    // Todo gather all wifi APs (not strictly necessarry, but for more evaluation
}

void WifiManager::readConfigFile() {
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

void WifiManager::writeConfigFile() {

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
            networkEntry[ServerConstant::JsonField::wifi::ssid] = item.Ssid;
            networkEntry[ServerConstant::JsonField::wifi::psk] = item.Psk;
            jnetworkList.push_back(networkEntry);
        }
        network[ServerConstant::JsonField::wifi::networks] = jnetworkList;
    } catch (std::exception& ex) {
        std::cerr << "cannot write file: " << jsonFile << " because: " << ex.what() << "\n";
        return;
    }

    ofs << network;
    ofs.close();
    generateWpaConfig();

}

void WifiManager::generateWpaConfig() {
    // actually done in wpa_generator
    system("/usr/bin/wpa_generate.sh");
}

void WifiManager::setSsid(std::string_view ssid, std::string_view psk) {
    NetworkEntry entry {std::string(ssid), std::string(psk)};
    removeEntry(std::string(ssid));
    networkList.emplace_back(std::move(entry));
    writeConfigFile();
}
