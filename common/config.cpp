#include "config.h"
#include <nlohmann/json.hpp>

void Common::Config::readConfig(std::string configFile) {

    std::ifstream ifs(configFile);
    nlohmann::json configData = nlohmann::json::parse(ifs);

    std::string debugLogLevel;
    auto config = shared_from_this();

    try {
        config->m_address = configData["IpAddress"];
        config->m_port = configData["Port"];
        config->m_basePath = configData["BasePath"];
        config->m_enableCache = configData["EnableCache"];
        debugLogLevel = configData["LogLevel"];
        if (configData.find("Amplify") != configData.end()) {
            config->m_amplify = configData["Amplify"];
        }
        else {
            config->m_amplify = 1;
        }

        if (configData.find("AudioInterface") != configData.end()) {
            std::string audioInterface = configData["AudioInterface"];
            if (audioInterface == "gst")
                config->m_playerType = Common::Config::PlayerType::GstPlayer;
            if (audioInterface == "mpl")
                config->m_playerType = Common::Config::PlayerType::MpvPlayer;
        }

        config->m_logLevel = LoggerFramework::Level::debug;

        if (debugLogLevel == "info") {
            config->m_logLevel = LoggerFramework::Level::info;
        }
        else if (debugLogLevel == "warning") {
            config->m_logLevel = LoggerFramework::Level::warning;
        }
        else if (debugLogLevel == "error") {
            config->m_logLevel = LoggerFramework::Level::error;
    }

    } catch (std::exception& ex) {
        std::cerr << "cannot read config <"<<ex.what()<<"\n";
   }

    return;
}

void Common::Config::print() {
    logger(LoggerFramework::Level::info) << "Configuration\n";
    logger(LoggerFramework::Level::info) << "Address: " << m_address << ":" << m_port << "\n";
    logger(LoggerFramework::Level::info) << "Base Path: "<< m_basePath << "\n";
    logger(LoggerFramework::Level::info) << "Amplifying with: " << m_amplify << "\n";
    logger(LoggerFramework::Level::info) << "Selected Internal Player: " << (m_playerType==PlayerType::unset?"<unset>":(m_playerType==PlayerType::GstPlayer?"Gstreamer Output":"MPL Output")) << "\n";
    logger(LoggerFramework::Level::info) << "With Cache: " << (m_enableCache?"yes":"no") << "\n";
}
