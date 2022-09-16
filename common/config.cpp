#include "config.h"
#include "Constants.h"
#include <nlohmann/json.hpp>

void Common::Config::readConfig(std::string configFile) {

    std::ifstream ifs(configFile);
    nlohmann::json configData = nlohmann::json::parse(ifs);

    std::string debugLogLevel;
    auto config = shared_from_this();

    try {
        config->m_address = configData[ServerConstant::JsonField::Config::ipAddress];

        config->m_port = configData[ServerConstant::JsonField::Config::port];
        config->m_basePath = configData[ServerConstant::JsonField::Config::basePath];
        config->m_enableCache = configData[ServerConstant::JsonField::Config::enableCache];
        debugLogLevel = configData[ServerConstant::JsonField::Config::logLevel];
        if (configData.find(ServerConstant::JsonField::Config::amplify) != configData.end()) {
            config->m_amplify = configData[ServerConstant::JsonField::Config::amplify];
        }
        else {
            config->m_amplify = 1;
        }

        if (configData.find(ServerConstant::JsonField::Config::audioInterface) != configData.end()) {
            std::string audioInterface = configData[ServerConstant::JsonField::Config::audioInterface];
            if (audioInterface == ServerConstant::JsonField::Config::PlayerType::gstreamer)
                config->m_playerType = Common::Config::PlayerType::GstPlayer;
            if (audioInterface == ServerConstant::JsonField::Config::PlayerType::mpl)
                config->m_playerType = Common::Config::PlayerType::MpvPlayer;
        }

        config->m_logLevel = LoggerFramework::Level::debug;

        if (debugLogLevel == ServerConstant::JsonField::Config::LogLevel::info) {
            config->m_logLevel = LoggerFramework::Level::info;
        }
        else if (debugLogLevel == ServerConstant::JsonField::Config::LogLevel::warning) {
            config->m_logLevel = LoggerFramework::Level::warning;
        }
        else if (debugLogLevel == ServerConstant::JsonField::Config::LogLevel::error) {
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
