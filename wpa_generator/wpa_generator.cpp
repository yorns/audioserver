#include <string>
#include <vector>
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

struct NetworkEntry {
    std::string ssid;
    std::string psk;
};

struct WpaData {
    std::string initial;
    std::vector<NetworkEntry> networkList;
    std::string NetEntryRest;
};

constexpr auto jsonFile { "/etc/wpaConfig.json" };
constexpr auto wpaConfigFile { "/etc/wpa_supplicant.conf" };

int main(int argc, char* argv[])
{

    if (argc != 1) {
        std::cerr << "usage " << argv[0] << " [create|delete]\n";
        return EXIT_FAILURE;
    }

    // read wpaFile
    std::ifstream ifs(jsonFile);

    if (!ifs.good()) {
        std::cerr << "cannot open file <" << jsonFile << ">\n";
        return EXIT_FAILURE;
    }

    // convert json
    WpaData wpaConfig;
    try {
        nlohmann::json wpaJson = nlohmann::json::parse(ifs);

        auto network = wpaJson.at("Networks");
        for (auto& networkEntry : network) {
            NetworkEntry entry;
            entry.ssid = networkEntry.at("Ssid");
            entry.psk = networkEntry.at("Psk");
            wpaConfig.networkList.emplace_back(std::move(entry));
        }

    } catch (std::exception& ex) {
        std::cerr << "cannot read file: " << jsonFile << " because: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }

    ifs.close();

    // write wpa_supplicant.conf
    std::stringstream wpa_supplicant_conf;

    wpa_supplicant_conf << "ctrl_interface=/var/run/wpa_supplicant\n"
                        << "ctrl_interface_group=0\n"
                        << "update_config=1\n"
                        << "\n";

    for(auto& entry : wpaConfig.networkList) {
        wpa_supplicant_conf << "network={\n"
                            << "    ssid=\"" << entry.ssid << "\"\n"
                            << "    psk=\"" << entry.psk << "\"\n"
                            << "    proto=RSN\n"
                            << "    key_mgmt=WPA-PSK\n"
                            << "    pairwise=CCMP\n"
                            << "    auth_alg=OPEN\n"
                            << "}\n";
    }

    std::ofstream ofs(wpaConfigFile);

    if (!ofs.good()) {
        std::cerr << "cannot open file <" << wpaConfigFile << ">\n";
        return EXIT_FAILURE;
    }

    ofs << wpa_supplicant_conf.str();
    ofs.close();

    return EXIT_SUCCESS;
}
