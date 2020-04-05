#include <gst/gst.h>
#include <unistd.h>

#include <thread>
#include <iostream>

/* Structure to contain all our information, so we can pass it around */
class GstInterface {
public:
  std::shared_ptr<GstElement> playbin;  /* Our one and only element */
  std::shared_ptr<GstBus> gstBus;
  std::shared_ptr<GMainLoop> main_loop;  /* GLib's Main Loop */

  std::thread th;

  static gboolean handle_message (GstBus *bus, GstMessage *msg, gpointer data) {
      return static_cast<GstInterface*>(data)->_handle_message(bus, msg);
  }
  gboolean _handle_message (GstBus *, GstMessage *msg) {

    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE (msg)) {
      case GST_MESSAGE_ERROR:
        gst_message_parse_error (msg, &err, &debug_info);
        g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
        g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
        g_clear_error (&err);
        g_free (debug_info);
        g_main_loop_quit (main_loop.get());
        break;
      case GST_MESSAGE_EOS:
        g_print ("End-Of-Stream reached.\n");
        g_main_loop_quit (main_loop.get());
        break;
      case GST_MESSAGE_STATE_CHANGED: {
        GstState old_state, new_state, pending_state;
        gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
        if (GST_MESSAGE_SRC (msg) == GST_OBJECT (playbin.get())) {
          if (new_state == GST_STATE_PLAYING) {
            /* Once we are in the playing state, analyze the streams */
//            analyze_streams (data);
          }
        }
      } break;
    }

    /* We want to keep receiving messages */
    return TRUE;
  }

  GstInterface() {
      gint flags;

      /* Initialize GStreamer */
      gst_init (nullptr, nullptr);

      /* Create the elements */
      playbin.reset(gst_element_factory_make ("playbin", "playbin"), [](GstElement* elem) {
              gst_element_set_state (elem, GST_STATE_NULL);
              gst_object_unref (elem);
              });

      if (!playbin) {
        std::cerr << "Not all elements could be created.\n";
        return;
      }

      gstBus.reset(gst_element_get_bus (playbin.get()), [](GstBus* bus) {
          gst_object_unref (bus);
      });


      constexpr auto playFlagAudio = (1 << 1);

      /* Set flags to show Audio but ignore Video and Subtitles */
      g_object_get (playbin.get(), "flags", &flags, NULL);
      flags |= playFlagAudio;
      g_object_set (playbin.get(), "flags", flags, NULL);

      /* Add a bus watch, so we get notified when a message arrives */
      gst_bus_add_watch (gstBus.get(), static_cast<GstBusFunc>(GstInterface::handle_message), this);

  }

  void run() {
      th = std::thread( [this]() {

          /* Create a GLib Main Loop and set it to run */
          main_loop.reset( g_main_loop_new (nullptr, FALSE), [](GMainLoop* main_loop) {
              g_main_loop_unref (main_loop);
          });
          g_main_loop_run (main_loop.get());

          std::cerr << "\n\n ***** MAIN LOOP ENDED ******\n\n";

      });

  }

  void play() {

      /* Set the URI to play */
      g_object_set (playbin.get(), "uri", "https://wdr-diemaus-live.icecastssl.wdr.de/wdr/diemaus/live/mp3/128/stream.mp3", NULL);

      auto ret = gst_element_set_state (playbin.get(), GST_STATE_PLAYING);
      if (ret == GST_STATE_CHANGE_FAILURE) {
          g_printerr ("Unable to set the pipeline to the playing state.\n");
          return;
      }
  }

  void stop() {
      auto ret = gst_element_set_state (playbin.get(), GST_STATE_READY);
      if (ret == GST_STATE_CHANGE_FAILURE) {
          g_printerr ("Unable to set the pipeline to the playing state.\n");
          return;
      }
  }

  ~GstInterface() {
      th.join();
  }

};

int main(int argc, char *argv[]) {
  GstInterface data;

    data.run();
    data.play();

  return 0;
}

///* Extract some metadata from the streams and print it on the screen */
//static void analyze_streams (GstInterface *data) {
//  gint i;
//  GstTagList *tags;
//  gchar *str;
//  guint rate;

//  /* Read some properties */
//  g_object_get (data->playbin, "n-video", &data->n_video, NULL);
//  g_object_get (data->playbin, "n-audio", &data->n_audio, NULL);
//  g_object_get (data->playbin, "n-text", &data->n_text, NULL);

//  g_print ("%d video stream(s), %d audio stream(s), %d text stream(s)\n",
//    data->n_video, data->n_audio, data->n_text);

//  g_print ("\n");
//  for (i = 0; i < data->n_video; i++) {
//    tags = NULL;
//    /* Retrieve the stream's video tags */
//    g_signal_emit_by_name (data->playbin, "get-video-tags", i, &tags);
//    if (tags) {
//      g_print ("video stream %d:\n", i);
//      gst_tag_list_get_string (tags, GST_TAG_VIDEO_CODEC, &str);
//      g_print ("  codec: %s\n", str ? str : "unknown");
//      g_free (str);
//      gst_tag_list_free (tags);
//    }
//  }

//  g_print ("\n");
//  for (i = 0; i < data->n_audio; i++) {
//    tags = NULL;
//    /* Retrieve the stream's audio tags */
//    g_signal_emit_by_name (data->playbin, "get-audio-tags", i, &tags);
//    if (tags) {
//      g_print ("audio stream %d:\n", i);
//      if (gst_tag_list_get_string (tags, GST_TAG_AUDIO_CODEC, &str)) {
//        g_print ("  codec: %s\n", str);
//        g_free (str);
//      }
//      if (gst_tag_list_get_string (tags, GST_TAG_LANGUAGE_CODE, &str)) {
//        g_print ("  language: %s\n", str);
//        g_free (str);
//      }
//      if (gst_tag_list_get_uint (tags, GST_TAG_BITRATE, &rate)) {
//        g_print ("  bitrate: %d\n", rate);
//      }
//      gst_tag_list_free (tags);
//    }
//  }

//  g_print ("\n");
//  for (i = 0; i < data->n_text; i++) {
//    tags = NULL;
//    /* Retrieve the stream's subtitle tags */
//    g_signal_emit_by_name (data->playbin, "get-text-tags", i, &tags);
//    if (tags) {
//      g_print ("subtitle stream %d:\n", i);
//      if (gst_tag_list_get_string (tags, GST_TAG_LANGUAGE_CODE, &str)) {
//        g_print ("  language: %s\n", str);
//        g_free (str);
//      }
//      gst_tag_list_free (tags);
//    }
//  }

//  g_object_get (data->playbin, "current-video", &data->current_video, NULL);
//  g_object_get (data->playbin, "current-audio", &data->current_audio, NULL);
//  g_object_get (data->playbin, "current-text", &data->current_text, NULL);

//  g_print ("\n");
//  g_print ("Currently playing video stream %d, audio stream %d and text stream %d\n",
//    data->current_video, data->current_audio, data->current_text);
//  g_print ("Type any number and hit ENTER to select a different audio stream\n");
//}

/* Process messages from GStreamer */
