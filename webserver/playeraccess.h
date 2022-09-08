#ifndef PLAYERACCESS_H
#define PLAYERACCESS_H

#include <memory>
#include <string>
#include "common/Extractor.h"
#include "playerinterface/Player.h"

class PlayerAccess
{
    std::shared_ptr<BasePlayer> m_player;

public:
    PlayerAccess(std::shared_ptr<BasePlayer>& player)
        : m_player(player) {}

    PlayerAccess() = delete;

    std::string access(const utility::Extractor::UrlInformation &urlInfo);

    std::string restAPIDefinition();

    boost::uuids::uuid getPlaylistID() const { return m_player->getPlaylistID(); }
    boost::uuids::uuid getSongID() const { return m_player->getSongID(); }
    std::string getTitle() const { return m_player->getTitle(); }
    std::string getAlbum() const { return m_player->getAlbum(); }
    std::string getPerformer() const { return m_player->getPerformer(); }
    int getSongPercentage() const { return m_player->getSongPercentage(); }
    bool getLoop() const { return m_player->getLoop(); }
    bool getShuffle() const { return m_player->getShuffle(); }
    bool isPause() const { return m_player->isPause(); }
    uint32_t getVolume() const { return m_player->getVolume(); }
    bool isSingle() const { return m_player->isSingle(); }
    void stop() const { m_player->stop(); }
    bool setPlaylist(const Common::AlbumPlaylistAndNames& name) { return m_player->setPlaylist(name); }
    bool startPlay(boost::uuids::uuid& titleID, uint32_t position) { return m_player->startPlay(titleID, position); }
    bool isPlaying() const { return m_player->isPlaying(); }

    bool hasPlayer() { return m_player != nullptr; }


};

#endif // PLAYERACCESS_H
