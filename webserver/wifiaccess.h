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

    std::string access(const utility::Extractor::UrlInformation &urlInfo) {

        if (!urlInfo || urlInfo->m_parameterList.size() != 2) {
            logger(Level::warning) << "invalid url given for database access\n";
            return R"({"result": "illegal url given" })";
        }

        logger(Level::info) << "wifi access - parameter: <"<<urlInfo->m_parameterList[0].name<<"> value: <"<<urlInfo->m_parameterList[0].value<<">\n";
        logger(Level::info) << "wifi access - parameter: <"<<urlInfo->m_parameterList[1].name<<"> value: <"<<urlInfo->m_parameterList[1].value<<">\n";

        // /wifi?ssid=<SSID>&psk=<PSK>

        if ( urlInfo->hasParameter( ServerConstant::Parameter::Wifi::ssid ) &&
             urlInfo->hasParameter( ServerConstant::Parameter::Wifi::psk )) {

            logger(Level::info) << "set ssid request\n";
            auto ssid = urlInfo->getValueOfParameter( ServerConstant::Parameter::Wifi::ssid );
            auto psk = urlInfo->getValueOfParameter(  ServerConstant::Parameter::Wifi::psk );

            if (! ssid.empty() && !psk.empty() ) {

                m_wifiManager.setSsid( ssid, psk );

                return R"({"result": "ok"})";
            }
            else {
                return R"({"result": "ssid or password not set correctly"})";
            }

        }

        return R"({"result": "unknown command"})";

    }

    std::string restAPIDefinition() { return ""; }
};


#endif // WIFIACCESS_H
