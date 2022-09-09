#ifndef WIFIACCESS_H
#define WIFIACCESS_H

#include <string>

#include "common/Extractor.h"
#include "common/Constants.h"
#include "wifi/wifi.h"

class WifiAccess
{
    WifiManager& m_wifiManager;

public:
    WifiAccess(WifiManager& wifiManager)
        : m_wifiManager(wifiManager) {}

    std::string access(const utility::Extractor::UrlInformation &urlInfo);

    std::string restAPIDefinition() { return ""; }
};


#endif // WIFIACCESS_H
