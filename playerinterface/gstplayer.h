#ifndef GSTPLAYER_H
#define GSTPLAYER_H

#include "Player.h"
#include <gst/gst.h>
#include <memory>
#include <thread>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include "common/filesystemadditions.h"
#include "common/repeattimer.h"

using namespace std::chrono_literals;

class GstPlayer  : public BasePlayer
{
    std::shared_ptr<GstElement> m_playbin;  /* Our one and only element */
    std::shared_ptr<GstBus> m_gstBus;

    RepeatTimer m_gstLoop;
    std::string m_songName;
    std::string m_title; // empty as long as no data is set from gstreamer;
    std::string m_album;
    std::string m_performer;

    gboolean _handle_message (GstBus *, GstMessage *msg);

    virtual void stopAndRestart();

protected:

    bool doPlayFile(const Common::PlaylistItem& playlistItem);

public:
    GstPlayer(boost::asio::io_context& context);

    bool setVolume(uint32_t volume) final;

    bool startPlay(const boost::uuids::uuid& songUID, uint32_t position) final;

    bool stop() final;

    virtual bool stopPlayerConnection() final;

    bool seek_forward() final;

    bool seek_backward() final;

    bool next_file()  final;
    bool prev_file()  final;

    bool pause_toggle() final;

    bool jump_to_position(int percent)  final;

    bool jump_to_fileUID(const boost::uuids::uuid& fileId)  final;

    const std::string getTitle() const final { return m_title; }
    const std::string getAlbum() const final { return m_album; }
    const std::string getPerformer() const final { return m_performer; }

    const std::string getSongName() const final { return m_songName; }
    boost::uuids::uuid getSongID() const final;

    int getSongPercentage() const final;


};

#endif // GSTPLAYER_H
