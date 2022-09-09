#ifndef PLAYERACCESS_H
#define PLAYERACCESS_H

#include <memory>
#include <string>

#include <boost/asio/io_context.hpp>

#include "common/Extractor.h"
#include "common/config.h"

#include "playerinterface/Player.h"
#include "playerinterface/gstplayer.h"
#include "playerinterface/mpvplayer.h"

class PlayerAccess
{
    std::shared_ptr<BasePlayer> m_player {nullptr};

public:
    PlayerAccess(std::shared_ptr<Common::Config> config, boost::asio::io_context& ioc);

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

    void setSongEndCB(SongEndCallback&& endfunc);


    void resetPlayer() const { m_player->resetPlayer(); }
    bool hasPlayer() { return m_player != nullptr; }


};

#endif // PLAYERACCESS_H
