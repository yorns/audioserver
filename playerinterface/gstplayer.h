#ifndef GSTPLAYER_H
#define GSTPLAYER_H

#include "Player.h"
#include <gst/gst.h>
#include <memory>
#include <thread>
#include <boost/asio.hpp>
#include "common/filesystemadditions.h"
#include "common/repeattimer.h"

using namespace std::chrono_literals;

class GstPlayer  : public BasePlayer
{
    std::shared_ptr<GstElement> m_playbin;  /* Our one and only element */
    std::shared_ptr<GstBus> m_gstBus;

    RepeatTimer m_gstLoop;
    std::string m_songName;

    gboolean _handle_message (GstBus *, GstMessage *msg) {

      GError *err;
      gchar *debug_info;

      switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error (msg, &err, &debug_info);
          logger(LoggerFramework::Level::warning) << "Error received from element " << GST_OBJECT_NAME (msg->src) << ": " << err->message << "\n";
          logger(LoggerFramework::Level::warning) << "Debugging information: " << (debug_info ? debug_info : "none") << "\n";
          g_clear_error (&err);
          g_free (debug_info);
          break;
        case GST_MESSAGE_EOS: {
          logger(LoggerFramework::Level::debug) << "End-Of-Stream reached.\n";
          if (m_songEndCallback) m_songEndCallback(*m_currentItemIterator);
          if (calculateNextFileInList())
              doPlayFile(*m_currentItemIterator);
          break;
      }
        case GST_MESSAGE_STATE_CHANGED: {
          GstState old_state, new_state, pending_state;
          gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
          if (GST_MESSAGE_SRC (msg) == GST_OBJECT (m_playbin.get())) {
            if (new_state == GST_STATE_PLAYING) {
            }


          }
        } break;
      case GST_MESSAGE_TAG: {
          logger(LoggerFramework::Level::debug) << "Tag received\n";
          GstTagList *tags { nullptr };
          gst_message_parse_tag (msg, &tags);

          const gchar* titleTagName = "title";
          gchar* value; // memory management unclear!!
          if (gst_tag_list_get_string_index (tags, titleTagName, 0, &value)) {
              logger(LoggerFramework::Level::debug)<< "tag: "<<titleTagName<<" = " << value <<"\n";
              m_songName = value;
              m_songName += " - ";
          }

          const gchar* albumTagName = "album";
          if (gst_tag_list_get_string_index (tags, albumTagName, 0, &value)) {
              logger(LoggerFramework::Level::debug)<< "tag: "<<albumTagName<<" = " << value <<"\n";
              m_songName += value;
          }

          gst_tag_list_unref (tags);

          break;
      }
      }

      /* We want to keep receiving messages */
      return TRUE;
    }

    virtual void stopAndRestart() {
        m_currentItemIterator = m_playlist.begin();
        doPlayFile(*m_currentItemIterator);
    }

protected:

    bool doPlayFile(const std::string& uniqueId, const std::string& prefix = "file://") {
        std::string filename = prefix + Common::FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Audio) + '/' + uniqueId + ".mp3";

        logger(LoggerFramework::Level::info) << "\nplaying "<<uniqueId<<" > "<<filename<<">\n";
        auto ret = gst_element_set_state (m_playbin.get(), GST_STATE_READY);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            logger(LoggerFramework::Level::warning) "Unable to set the pipeline to the stop state.\n";
            return false;
        }

        g_object_set (m_playbin.get(), "uri", filename.c_str(), NULL);

        ret = gst_element_set_state (m_playbin.get(), GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            logger(LoggerFramework::Level::warning) "Unable to set the pipeline to the playing state.\n";
            return false;
        }
        m_isPlaying = true;
        return true;

    }

public:
    GstPlayer(boost::asio::io_context& context) :
    m_gstLoop(context, 50ms) {
        gint flags;

        logger(LoggerFramework::Level::info) << "Constructor GstPlayer\n";

        /* Initialize GStreamer */
        gst_init (nullptr, nullptr);

        /* Create the elements */
        m_playbin.reset(gst_element_factory_make ("playbin", "playbin"), [](GstElement* elem) {
                gst_element_set_state (elem, GST_STATE_NULL);
                gst_object_unref (elem);
                });

        if (!m_playbin) {
          std::cerr << "Not all elements could be created.\n";
          return;
        }

        m_gstBus.reset(gst_element_get_bus (m_playbin.get()), [](GstBus* bus) {
            gst_object_unref (bus);
        });

        if (!m_gstBus) {
          std::cerr << "Not all elements could be created.\n";
          return;
        }

        constexpr auto playFlagAudio = (1 << 1);

        /* Set flags to show Audio but ignore Video and Subtitles */
        g_object_get (m_playbin.get(), "flags", &flags, NULL);
        flags |= playFlagAudio;
        g_object_set (m_playbin.get(), "flags", flags, NULL);

        /* Add a bus watch, so we get notified when a message arrives */
        //gst_bus_add_watch (m_gstBus.get(), static_cast<GstBusFunc>(GstPlayer::handle_message), this);

        logger(LoggerFramework::Level::info) << "Constructor GstPlayer - DONE\n";

        m_gstLoop.setHandler([this](){
            while (auto msg = gst_bus_pop (m_gstBus.get())) {
              _handle_message(m_gstBus.get(), msg);
              gst_message_unref (msg);
            }
        });

        m_gstLoop.start();
    }


    bool startPlay(const std::vector<std::string>& list,
                           const std::string& playlistUniqueId,
                   const std::string& playlistName) final {

        if (list.empty())
            return false;

        m_playlist = list;
        m_playlist_orig = list;
        m_currentItemIterator = m_playlist.begin();

        return doPlayFile(*m_currentItemIterator);
    }


    bool stop() final {
        auto ret = gst_element_set_state (m_playbin.get(), GST_STATE_READY);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            logger(LoggerFramework::Level::warning) "Unable to set the pipeline to the stop state.\n";
            return false;
        }
        logger(LoggerFramework::Level::debug) "Stopping play\n";
        m_isPlaying = false;
        return true;
    }

    virtual bool stopPlayerConnection() final {
        gst_element_set_state (m_playbin.get(), GST_STATE_NULL);
        m_gstLoop.stop();
        return true;
    }

    bool seek_forward() final {
        gint64 pos, len;

        if (gst_element_query_position (m_playbin.get(), GST_FORMAT_TIME, &pos) &&
           gst_element_query_duration (m_playbin.get(), GST_FORMAT_TIME, &len)) {
            logger(LoggerFramework::Level::debug) << "seeking from time position " << pos << " + 20000000 \n";
            pos += 20000000000; // 20 seconds in nanoseconds
                if (pos > len) {
                return next_file();
                }
            if (!gst_element_seek (m_playbin.get(), 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                                   GST_SEEK_TYPE_SET, pos,
                                   GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
                logger(LoggerFramework::Level::warning) << "Seek failed!\n";
                return false;
            }
            else
                return true;
        }
        logger(LoggerFramework::Level::debug) << "seeking forward failed\n";
        return false;
    }

    bool seek_backward() final {
        gint64 pos;

        if (gst_element_query_position (m_playbin.get(), GST_FORMAT_TIME, &pos)) {
            if (pos > 20000000000)
                pos -= 20000000000; // 20 nanoseconds
            else
                pos = 0;
            if (!gst_element_seek (m_playbin.get(), 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                                   GST_SEEK_TYPE_SET, pos,
                                   GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
                logger(LoggerFramework::Level::warning) << "Seek failed!\n";
            }
            else
                return true;
        }
        logger(LoggerFramework::Level::debug) << "seeking forward failed\n";
        return false;
    }

    bool next_file()  final {
        if (!isPlaying()) return false;
        logger(LoggerFramework::Level::debug) << "play next audio file\n";
        if (calculateNextFileInList()) {
            if (m_songEndCallback) m_songEndCallback(*m_currentItemIterator);
            return doPlayFile(*m_currentItemIterator);
        }
        return false;
    }
    bool prev_file()  final {
        if (!isPlaying()) return false;
        logger(LoggerFramework::Level::debug) << "play previous audio file\n";
        if (calculatePreviousFileInList()) {
            if (m_songEndCallback) m_songEndCallback(*m_currentItemIterator);
            return doPlayFile(*m_currentItemIterator);
        }
        return false;
    }

    bool pause_toggle() final {

        GstStateChangeReturn ret;
        m_pause = !m_pause;
        if (m_pause)
            ret = gst_element_set_state (m_playbin.get(), GST_STATE_PAUSED);
        else
            ret = gst_element_set_state (m_playbin.get(), GST_STATE_PLAYING);

        if (ret == GST_STATE_CHANGE_FAILURE) {
            logger(LoggerFramework::Level::warning) "Unable to set the pipeline to the playing state.\n";
            return false;
        }
        return true;
    }

    bool jump_to_position(int percent)  final {

        gint64 pos, len;

        if (gst_element_query_duration (m_playbin.get(), GST_FORMAT_TIME, &len)) {
            pos = (len/100)*percent;
            if (!gst_element_seek (m_playbin.get(), 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                                   GST_SEEK_TYPE_SET, pos,
                                   GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
                logger(LoggerFramework::Level::warning) << "Seek failed!\n";
            }
            else {
                return true;
            }
        }
        return false;
    }

    bool jump_to_fileUID(const std::string& fileId)  final {
        auto it = std::find_if(std::begin(m_playlist),
                               std::end(m_playlist),
                               [&fileId](const std::string& uniqueId){ return uniqueId == fileId; });
        if (it != std::end(m_playlist)) {
            logger(LoggerFramework::Level::info) << "change to file id from <" << *it << "> to <"<< fileId << ">\n";
            if (m_isPlaying) {
                if (m_songEndCallback) m_songEndCallback(*m_currentItemIterator);
            }
            m_currentItemIterator = it;
            doPlayFile(*m_currentItemIterator);

        }
        else {
            logger(LoggerFramework::Level::warning) << "file <" << fileId << "> not in playlist\n";
            return false;
        }

    }

    const std::string getSongName() const final { return m_songName; }
    std::string getSongID() const final { return ""; }

    int getSongPercentage() const final {
        gint64 pos, len;
        if (gst_element_query_position (m_playbin.get(), GST_FORMAT_TIME, &pos)
          && gst_element_query_duration (m_playbin.get(), GST_FORMAT_TIME, &len)) {
            //logger(LoggerFramework::Level::debug) << "position is calculated to: "<< pos << "/" << len << " = " <<static_cast<int>(100.0*pos/len) << "%\n";
            return static_cast<int>(10000.0*pos/len);
        }
        //logger(LoggerFramework::Level::debug) << "position calculation failed\n";
        return 0;
    }


};

#endif // GSTPLAYER_H
