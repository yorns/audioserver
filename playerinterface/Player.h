#ifndef SIMPLEMOVIEUI_PLAYER_H
#define SIMPLEMOVIEUI_PLAYER_H

#include <functional>
#include <string>
#include <fstream>
#include <vector>
#include "common/logger.h"

struct StopInfoEntry {
    std::string fileName;
    std::string stopTime;
    bool        valid;
};

class Player {

protected:
    std::string m_configDbFileName;
    std::vector<StopInfoEntry> m_stopInfoList;

    std::string extractName(const std::string& fullName);
    std::function<void(const std::string& )> m_endfunc;

public:

    Player(const std::string& configDB)
    : m_configDbFileName(configDB) {
    }

    virtual ~Player() = default;

    virtual bool startPlay(const std::string &url, const std::string& playerInfo, bool fromLastStop = false) = 0;
    virtual bool stop() = 0;
    virtual bool seek_forward() = 0;
    virtual bool seek_backward() = 0;
    virtual bool next_file() = 0;
    virtual bool prev_file() = 0;
    virtual bool pause() = 0;

    virtual bool isPlaying() = 0;

    virtual std::string getSongFile() = 0;
    virtual std::string getSongName() = 0;

    void setPlayerEndCB(const std::function<void(const std::string& )>& endfunc);

};


#endif //SIMPLEMOVIEUI_PLAYER_H
