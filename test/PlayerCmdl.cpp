#include <iostream>
#include "KeyHit.h"
#include <boost/asio.hpp>
#include <memory>
#include <functional>
#include <iomanip>
#include "playerinterface/Player.h"
#include "playerinterface/mpvplayer.h"
#include "playerinterface/gstplayer.h"
#include "common/repeattimer.h"
#include "common/logger.h"
#include "database/playlist.h"
#include "common/Constants.h"

using namespace LoggerFramework;
using namespace std::chrono_literals;

class AudioPlayer
{
private:
    boost::asio::io_context m_context;
    KeyHit m_keyHit;
    std::unique_ptr<BasePlayer> m_player { new GstPlayer(m_context) };
    RepeatTimer m_timer {m_context, 100ms};
    boost::asio::signal_set m_signals {m_context, SIGINT};

    std::string m_playlistFilename;

    void keyHandler(const char key) {
        switch (key) {
        case 'q': {
            m_player->stop();
            m_player->stopPlayerConnection();
            m_timer.stop();
            m_signals.cancel();
            m_keyHit.stop();
            logger(Level::info) << "all components stopped\n";
            break;
        }
        case 'p': {
            Database::Playlist playlist(m_playlistFilename, Database::ReadType::isM3u, Database::Persistent::isTemporal);
            playlist.readM3u();
            //m_player->startPlay(playlist.getUniqueIdPlaylist(), playlist.getUniqueID(), "");
            break;
        }
        case '+': {
            m_player->next_file();
            break;
        }
        case '-': {
            m_player->prev_file();
            break;
        }
        case 'k': {
            m_player->seek_forward();
            break;
        }
        case 'j': {
            m_player->seek_backward();
            break;
        }
        case ' ': {
            m_player->pause_toggle();
            break;
        }
        case 'c': {
            m_player->toogleLoop();
            break;
        }
        case 'x': {
            m_player->toggleShuffle();
            break;
        }
        case 's': {
            m_player->stop();
            break;
        }
        }

    }

    void init() {

        m_keyHit.setKeyReceiver([this](const char key){
            m_context.post([this, key]() { keyHandler(key); });
        });

        std::cout.precision(2);
        std::cout.setf(std::ios::floatfield, std::ios::fixed);
        std::cout << "\e[?25l";

        m_timer.setHandler([this]() {
            if (m_player->isPlaying()) {
                auto file = m_player->getSongName();
                auto pos  = m_player->getSongPercentage();
                std::cout << std::setw(40) << file << " "<< std::setw(6)
                          << pos/100.0 << " \t" << std::string((pos+1)/200, '#')
                          << "           \r" <<std::flush;
           }
        });

        m_player->setPlaylistEndCB([](){
            std::cout << "\nPlaylist stopped\n";
        });

        m_player->setSongEndCB([this](const std::string& ){
            if (m_player->isPlaying())
                std::cout << "\n";
        });

        m_signals.async_wait([this](const boost::system::error_code& ec, int ) {
            if (!ec) {
                m_player->stop();
                m_player->stopPlayerConnection();
                m_timer.stop();
                m_keyHit.stop();
            } else {
                logger(Level::info) << "signal handler stopped\n";
            }
        });
    }

public:

    AudioPlayer(const std::string& playlistFile) : m_playlistFilename(playlistFile) { init(); }
    ~AudioPlayer() { std::cout << "\e[?25h\n"; }

    AudioPlayer() = delete;
    AudioPlayer(const AudioPlayer& ) = delete;
    AudioPlayer(AudioPlayer&& ) = delete;

    AudioPlayer& operator=(const AudioPlayer& ) = delete;
    AudioPlayer& operator=(AudioPlayer&& ) = delete;

    void run() {
        m_keyHit.start();
        m_timer.start();
        m_context.run();
    }

};

/* please create an infrastructure like this
mkdir -p /tmp/playertest/mp3/
mkdir -p /tmp/playertest/playlist/
 */

int main(int argc, char* argv[])
{
    globalLevel = Level::warning;
    ServerConstant::base_path = ServerConstant::sv{"/tmp/playertest"};

    if (argc != 2) {
        std::cerr << "usage "<<argv[0] <<" <playlist file>\n";
        return EXIT_FAILURE;
    }

    auto audioPlayer = std::make_unique<AudioPlayer>(argv[1]);
    audioPlayer->run();

    return EXIT_SUCCESS;
}
