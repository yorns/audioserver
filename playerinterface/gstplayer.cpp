#include "gstplayer.h"


gboolean GstPlayer::_handle_message(GstBus *, GstMessage *msg) {

    GError *err;
    gchar *debug_info;
    InternalState state { InternalState::null };

    //logger(LoggerFramework::Level::info) << "message received <"<<(int)GST_MESSAGE_TYPE(msg)<<"> (" << gst_message_type_get_name(GST_MESSAGE_TYPE(msg)) <<")\n";

    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_INFO:
        gst_message_parse_error (msg, &err, &debug_info);
        logger(LoggerFramework::Level::warning) << "Error received from element " << GST_OBJECT_NAME (msg->src) << ": " << err->message << "\n";
        logger(LoggerFramework::Level::warning) << "Debugging information: " << (debug_info ? debug_info : "none") << "\n";
        g_clear_error (&err);
        g_free (debug_info);
        break;

    case GST_MESSAGE_EOS: {

        // pulling execution into correct context (just in case)
        auto eos_handler = [this]() {
        logger(LoggerFramework::Level::debug) << "End-Of-Stream reached.\n";
        if (m_songEndCallback) m_songEndCallback(m_currentItemIterator->m_uniqueId);
        if (calculateNextFileInList()) {
            doPlayFile(*m_currentItemIterator);
        }
        else {
            m_currentItemIterator = m_playlist.begin();
            m_album = "";
            m_title = "";
            m_performer = "";
            m_isPlaying = false;
        }
        };
        m_context.post([eos_handler]() {eos_handler();});
        break;
    }

    case GST_MESSAGE_STATE_CHANGED: {
        GstState old_state, new_state; //, pending_state;
        gst_message_parse_state_changed (msg, &old_state, &new_state, nullptr);

        auto printState = [](GstState state) {
            if (state == GST_STATE_PLAYING) {
                return "playing";
            }
            if (state == GST_STATE_VOID_PENDING) {
                return "void pending";
            }
            if (state == GST_STATE_PAUSED) {
                return "paused";
            }
            if (state == GST_STATE_READY) {
                return "ready";
            }
            if (state == GST_STATE_NULL) {
                return "null";
            }
            return "unknown";
        };

        if (GST_MESSAGE_SRC (msg) == GST_OBJECT (m_playbin.get())) {
            logger(LoggerFramework::Level::info) << "statechange found with playbin set: old <"
                                                 << printState(old_state) <<"> - new <" << printState(new_state)
                                                 << ">\n"; // - pending <" << printState(pending_state) << ">\n";
            if (new_state == GST_STATE_PLAYING) {
                    setVolume(m_volume);
                    m_isPlaying = true;
                    state = InternalState::playing;
            }
            if (new_state == GST_STATE_VOID_PENDING) {
                state = InternalState::pending;
            }
            if (new_state == GST_STATE_PAUSED) {
                m_isPlaying = true;
                state = InternalState::pause;
            }
            if (new_state == GST_STATE_READY) {
                m_isPlaying = false;
                state = InternalState::ready;
            }
            if (new_state == GST_STATE_NULL) {
                m_isPlaying = false;
                state = InternalState::null;
            }
            if (m_stateChange) {
                logger(LoggerFramework::Level::info) << "running statechange callback\n";
                m_stateChange(state);
            }

        }
        else {
//            logger(LoggerFramework::Level::info) << "------------------ No clue, what this should do or not - playbin.get() failed\n";
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
            m_title = value;
            m_songName = m_title;
            m_songName += " - ";
        }

        const gchar* albumTagName = "album";
        if (gst_tag_list_get_string_index (tags, albumTagName, 0, &value)) {
            logger(LoggerFramework::Level::debug)<< "tag: "<<albumTagName<<" = " << value <<"\n";
            m_album = value;
            m_songName += m_album;
        }

        gst_tag_list_unref (tags);

        break;
    }
    case GST_MESSAGE_ASYNC_DONE: {
        logger(LoggerFramework::Level::info) << "Async done received\n";
        break;
    }
    default:
        break;
    }

    /* We want to keep receiving messages */
    return TRUE;
}

void GstPlayer::stopAndRestart() {
    m_currentItemIterator = m_playlist.begin();
    doPlayFile(*m_currentItemIterator);
}

bool GstPlayer::doPlayFile(const Common::PlaylistItem &playlistItem) {

    logger(LoggerFramework::Level::info) << "playing "<<playlistItem.m_uniqueId<<" ("<<playlistItem.m_url<<")\n";

    // clean album information
    m_album = "";
    m_title = "";
    m_performer = "";
    resetPause();

    auto ret = gst_element_set_state (m_playbin.get(), GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        logger(LoggerFramework::Level::warning) "Unable to set the pipeline to the stop state.\n";
        return false;
    }

    logger(LoggerFramework::Level::debug) << "playing <" << playlistItem.m_url << ">\n";

    g_object_set (m_playbin.get(), "uri", playlistItem.m_url.c_str(), NULL);

    ret = gst_element_set_state (m_playbin.get(), GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        logger(LoggerFramework::Level::warning) "Unable to set the pipeline to the playing state.\n";
        return false;
    }
    return true;

}

GstPlayer::GstPlayer(boost::asio::io_context &context) :
    m_context(context), m_volume_tmp(0), m_gstLoop(context, 50ms) {
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
        logger(LoggerFramework::Level::error) << "playbin could not be created\n";
        return;
    }

    m_gstBus.reset(gst_element_get_bus (m_playbin.get()), [](GstBus* bus) {
        gst_object_unref (bus);
    });

    if (!m_gstBus) {
        logger(LoggerFramework::Level::error) << "gstreamer buss could not be created\n";
        return;
    }

    constexpr auto playFlagAudio = (1 << 1);

    /* Set flags to show Audio but ignore Video and Subtitles */
    g_object_get (m_playbin.get(), "flags", &flags, NULL);
    flags |= playFlagAudio;
    g_object_set (m_playbin.get(), "flags", flags, NULL);

    setVolume(15); // set default volume (test 15)

    logger(LoggerFramework::Level::info) << "Constructor GstPlayer - DONE\n";

    m_gstLoop.setHandler([this](){
        while (auto msg = gst_bus_pop (m_gstBus.get())) {
            _handle_message(m_gstBus.get(), msg);
            gst_message_unref (msg);
        }
    });

    m_gstLoop.start();
}

bool GstPlayer::setVolume(uint32_t volume) {

    logger(LoggerFramework::Level::info) << "set volume to <"<<volume<<">\n";

    double volume_double = volume/100.0;
    volume_double *= 1; // 2.5;
    g_object_set ( m_playbin.get(), "volume", volume_double, NULL );

    m_volume = volume;

    return true;
}

bool GstPlayer::startPlay(const boost::uuids::uuid &songUID, uint32_t position) {

    if (needsOnlyUnpause(m_playlistUniqueId)) {
        logger(LoggerFramework::Level::debug) << "unpause playlist <" << m_playlistName <<">\n";
        auto ret = gst_element_set_state (m_playbin.get(), GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            logger(LoggerFramework::Level::warning) "Unable to set the pipeline to the playing state.\n";
            return false;
        }
        return true;
    }

    if (songUID.is_nil()) {
        if (m_playlist.empty()) {
            logger(LoggerFramework::Level::info) << "empty playlist cannot be started\n";
            return false;
        }

        logger(LoggerFramework::Level::info) << "start playing with first playlist item\n";
        m_currentItemIterator = m_playlist.begin();
    }
    else {

        if (songUID != m_currentItemIterator->m_uniqueId) {
            m_currentItemIterator = std::find_if(std::begin(m_playlist), std::end(m_playlist), [&songUID](auto& elem){ return elem.m_uniqueId == songUID; });
            if (m_currentItemIterator == std::end(m_playlist)) {
                logger(LoggerFramework::Level::warning) << "cannot set anyone title from playlist <" << m_playlistName
                                                        << ">\n (searching for <"<< songUID <<"> in <"<<m_playlist.size()<<"> elemenents\n";
                return false;
            }
        }
    }
    logger(LoggerFramework::Level::info) << "start pos (" << position <<") of Playing new playlist <"<<m_playlistUniqueId<<"> ("<<m_playlistName<<")\n";

    if (position == 0) {
        auto retValue = doPlayFile(*m_currentItemIterator);
        return retValue;
    }
    else {
        // what should be done on statechange
        m_stateChange = [this, position](InternalState state){
            if (state == InternalState::playing) {
                g_object_set ( m_playbin.get(), "volume", 0.0, NULL ); // gst arm cannot jump to position in ready or pause mode
                jump_to_position(position/100.0); m_stateChange = nullptr;
            }
        };
        if (doPlayFile(*m_currentItemIterator)) {            
            return true;
        }
        m_stateChange = nullptr;
        return false;

    }
}

bool GstPlayer::stop() {
    auto ret = gst_element_set_state (m_playbin.get(), GST_STATE_READY);

    if (ret == GST_STATE_CHANGE_FAILURE) {
        logger(LoggerFramework::Level::warning) "Unable to set the pipeline to the stop state.\n";
        return false;
    }

    logger(LoggerFramework::Level::debug) "Stopping play\n";
//    m_isPlaying = false;
    resetPause();

    return true;
}

bool GstPlayer::stopPlayerConnection() {
    gst_element_set_state (m_playbin.get(), GST_STATE_NULL);
    m_gstLoop.stop();
    return true;
}

bool GstPlayer::seek_forward() {
    gint64 pos, len;

    if (gst_element_query_position (m_playbin.get(), GST_FORMAT_TIME, &pos) &&
            gst_element_query_duration (m_playbin.get(), GST_FORMAT_TIME, &len)) {
        logger(LoggerFramework::Level::debug) << "seeking from time position " << pos << " + 20000000ns \n";
        pos += 20000000000; // 20 seconds in nanoseconds
        if (pos > len) {
            return next_file();
        }
        if (!gst_element_seek (m_playbin.get(), 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                               GST_SEEK_TYPE_SET, pos,
                               GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
            logger(LoggerFramework::Level::warning) << "Seek forwad failed\n";
            return false;
        }
        else
            return true;
    }
    logger(LoggerFramework::Level::debug) << "seeking forward failed\n";
    return false;
}

bool GstPlayer::seek_backward() {
    gint64 pos;

    if (gst_element_query_position (m_playbin.get(), GST_FORMAT_TIME, &pos)) {
        if (pos > 20000000000)
            pos -= 20000000000; // 20 nanoseconds
        else
            pos = 0;
        if (!gst_element_seek (m_playbin.get(), 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                               GST_SEEK_TYPE_SET, pos,
                               GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
            logger(LoggerFramework::Level::warning) << "seek backward failed\n";
        }
        else
            return true;
    }
    logger(LoggerFramework::Level::debug) << "seeking backward failed\n";
    return false;
}

bool GstPlayer::next_file() {
    if (!isPlaying()) return false;
    logger(LoggerFramework::Level::debug) << "play next audio file\n";
    if (calculateNextFileInList()) {
        if (m_songEndCallback) m_songEndCallback(m_currentItemIterator->m_uniqueId);
        return doPlayFile(*m_currentItemIterator);
    }
    return false;
}

bool GstPlayer::prev_file() {
    if (!isPlaying()) return false;
    logger(LoggerFramework::Level::debug) << "play previous audio file\n";
    auto lastUniqueId = m_currentItemIterator->m_uniqueId;
    if (calculatePreviousFileInList()) {
        if (m_songEndCallback) m_songEndCallback(lastUniqueId);
        return doPlayFile(*m_currentItemIterator);
    }
    return false;
}

bool GstPlayer::pause_toggle() {

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

bool GstPlayer::jump_to_position(int percent) {

    gint64 pos, len;

    static int busy_counter;
    busy_counter = 0;

    // on arm this does not work as espected.
    // state must be playing else duration cannot be received

    while(busy_counter < 100 && !gst_element_query_duration (m_playbin.get(), GST_FORMAT_TIME, &len)) {
        logger(LoggerFramework::Level::warning) << "gstreamer failure - making busy wait\n";
        busy_counter++;
        usleep(10000);
    }

    if (gst_element_query_duration (m_playbin.get(), GST_FORMAT_TIME, &len)) {
        pos = (len/100)*percent;
        if (!gst_element_seek (m_playbin.get(), 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                               GST_SEEK_TYPE_SET, pos,
                               GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
            logger(LoggerFramework::Level::warning) << "Seek failed!\n";
        }
        else {
            logger(LoggerFramework::Level::info) << "seek to "<<percent<<"% / " << pos << "sec posision done\n";
            return true;
        }
    }
    else {
        logger(LoggerFramework::Level::info) << "gstreamer is not able to identify duration interface\n";
    }
    return false;
}

bool GstPlayer::jump_to_fileUID(const boost::uuids::uuid &fileId) {

    auto it = std::find_if(std::begin(m_playlist),
                           std::end(m_playlist),
                           [&fileId](const auto& playlistItem){ return playlistItem.m_uniqueId == fileId; });
    if (it != std::end(m_playlist)) {
        logger(LoggerFramework::Level::info) << "change to file id from <" << it->m_uniqueId << "> to <"<< fileId << ">\n";
        if (m_isPlaying) {
            if (m_songEndCallback) m_songEndCallback(m_currentItemIterator->m_uniqueId);
        }
        m_currentItemIterator = it;
        doPlayFile(*m_currentItemIterator);
    }
    else {
        logger(LoggerFramework::Level::warning) << "file <" << fileId << "> not in current playlist\n";
        return false;
    }
    return true;
}

boost::uuids::uuid GstPlayer::getSongID() const {
    if(m_currentItemIterator != std::end(m_playlist))
        return m_currentItemIterator->m_uniqueId;
    try {
    return boost::lexical_cast<boost::uuids::uuid>(std::string(ServerConstant::unknownCoverFileUid));
    } catch (std::exception& ex) {
        logger(LoggerFramework::Level::warning) << ex.what() << "\n";
        return boost::uuids::uuid();
    }

}

int GstPlayer::getSongPercentage() const {
    gint64 pos, len;
    if (gst_element_query_position (m_playbin.get(), GST_FORMAT_TIME, &pos)
            && gst_element_query_duration (m_playbin.get(), GST_FORMAT_TIME, &len)) {
//        logger(LoggerFramework::Level::info) << "position is calculated to: "<< pos << "/" << len << " = " <<static_cast<int>(100.0*pos/len) << "%\n";
        return static_cast<int>(10000.0*pos/len);
    }
    logger(LoggerFramework::Level::info) << "position calculation failed\n";
    return 0;
}
