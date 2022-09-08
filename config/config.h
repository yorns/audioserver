#ifndef AUDIOSERVER_CONFIG_H
#define AUDIOSERVER_CONFIG_H

#include <string>
#include <optional>
#include <memory>
#include "common/logger.h"

namespace Common {

struct Config : std::enable_shared_from_this<Config> {

    enum class PlayerType {
        unset,
        GstPlayer,
        MpvPlayer
    };

    std::string m_address;
    std::string m_port;
    std::string m_basePath;
    uint32_t m_amplify {1};
    PlayerType m_playerType {PlayerType::unset};

    bool m_enableCache {false};

    LoggerFramework::Level m_logLevel;

    constexpr bool isPlayerType(const PlayerType& playerType) { return (playerType == m_playerType); }
    void readConfig(std::string configFile);

    void print();
};

}

#endif
