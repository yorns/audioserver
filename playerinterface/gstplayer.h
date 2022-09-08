#ifndef GSTPLAYER_H
#define GSTPLAYER_H

#include "config/system_config.h"

#ifdef HAVE_GST

#include "Player.h"
#include <gst/gst.h>
#include <memory>
#include <thread>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include "common/filesystemadditions.h"
#include "common/repeattimer.h"

using namespace std::chrono_literals;

//! gst interface class
/*! gstreamer is a nightmare. Therefore this class is meant to give a meaningful API.
 * for setting volume or position, start a file, stop it pause it, or whatever is needed
 * to have a meaningful handling. E.g. setting a position MUST be one single call whatever
 * it takes. Gstreamer gives everything to the caller for no need as the caller do not want
 * to care how the position is reached and in what state
 */
class GstPlayer  : public BasePlayer
{
    boost::asio::io_context& m_context;
    enum class InternalState { playing, pause, ready, null, pending };
    std::shared_ptr<GstElement> m_playbin;  /* Our one and only element */
    std::shared_ptr<GstBus> m_gstBus;

    uint32_t m_volume_tmp;

    RepeatTimer m_gstLoop;
    std::string m_songName;
    std::string m_title; // empty as long as no data is set from gstreamer;
    std::string m_album;
    std::string m_performer;

    std::function<void(InternalState)> m_stateChange;

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

#else

// code for a dummy GST-player

#include "Player.h"
#include <boost/asio.hpp>

class GstPlayer  : public BasePlayer
{

    virtual void stopAndRestart() {}

public:
    GstPlayer(boost::asio::io_context& ) {}

    bool setVolume(uint32_t) { return false; }

    bool startPlay(const boost::uuids::uuid& songUID, uint32_t position) { return false; }

    bool stop() { return false; }

    virtual bool stopPlayerConnection() { return false; }

    bool seek_forward() { return false; }

    bool seek_backward() { return false; }

    bool next_file()  { return false; }
    bool prev_file()  { return false; }

    bool pause_toggle() { return false; }

    bool jump_to_position(int percent)  { return false; }

    bool jump_to_fileUID(const boost::uuids::uuid& fileId)  { return false; }

    const std::string getTitle() const final { return ""; }
    const std::string getAlbum() const final { return ""; }
    const std::string getPerformer() const final { return ""; }

    const std::string getSongName() const final { return ""; }
    boost::uuids::uuid getSongID() const final { return boost::uuids::uuid(); }

    int getSongPercentage() const final { return 0; }


};

#endif
#endif // GSTPLAYER_H
