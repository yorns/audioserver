#include <string>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <fstream>
#include <common/logger.h>
#include <common/Constants.h>

using namespace LoggerFramework;


class WifiManager {

    struct NetworkEntry {
        std::string Ssid;
        std::string Psk;
        NetworkEntry(const std::string& ssid, const std::string& psk)
            : Ssid(ssid), Psk(psk) {}
        NetworkEntry() {}
    };

    std::vector<NetworkEntry> networkList;

    void removeEntry(const std::string& ssid);

public:
    constexpr static auto jsonFile { "/etc/wpaConfig.json" };

    WifiManager() { readConfigFile(); }

    void receiveWifiConfig(const std::string&);

    void readConfigFile();

    void writeConfigFile();

    void generateWpaConfig();

    void setSsid(std::string_view ssid, std::string_view psk);



};
