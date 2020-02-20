#include <iostream>
#include "KeyHit.h"
#include <boost/asio.hpp>
#include <memory>
#include <functional>
#include <iomanip>
#include "playerinterface/Player.h"
#include "playerinterface/mpvplayer.h"
#include "common/repeattimer.h"
#include "common/logger.h"

using namespace LoggerFramework;
using namespace std::chrono_literals;

class AudioPlayer
{
private:
    boost::asio::io_context context;
    KeyHit keyHit;
    std::unique_ptr<BasePlayer> player { new MpvPlayer(context) };
    RepeatTimer m_timer {context, 100ms};
    boost::asio::signal_set signals {context, SIGINT};

    std::string m_playlistFilename;

    void keyHandler(const char key) {
        switch (key) {
        case 'q': {
            player->stop();
            player->stopPlayerConnection();
            m_timer.stop();
            signals.cancel();
            keyHit.stop();
            logger(Level::info) << "all components stopped\n";
            break;
        }
        case 'p': {
            std::string url {m_playlistFilename};
            player->startPlay(std::move(url));
            break;
        }
        case '+': {
            player->next_file();
            break;
        }
        case '-': {
            player->prev_file();
            break;
        }
        case 'k': {
            player->seek_forward();
            break;
        }
        case 'j': {
            player->seek_backward();
            break;
        }
        case ' ': {
            player->pause_toggle();
            break;
        }
        case 's': {
            player->stop();
            break;
        }
        }

    }

    void init() {

        keyHit.setKeyReceiver([this](const char key){
            context.post([this, key]() { keyHandler(key); });
        });

        std::cout.precision(2);
        std::cout.setf(std::ios::floatfield, std::ios::fixed);
        std::cout << "\e[?25l";

        m_timer.setHandler([this]() {
            if (player->isPlaying()) {
                auto file = player->getSong();
                auto pos  = player->getSongPercentage();
                std::cout << std::setw(40) << file << " "<< std::setw(6) << pos/100.0 << " \t" << std::string((pos+1)/200, '#') << "           \r" <<std::flush;
           }
        });

        player->setPlayerEndCallBack([](){
            std::cout << "\nstopped\n";
        });

        player->setSongEndCB([this](const std::string& ){
            if (player->isPlaying())
                std::cout << "\n";
        });

        signals.async_wait([this](const boost::system::error_code& ec, int ) {
            if (!ec) {
                player->stop();
                player->stopPlayerConnection();
                m_timer.stop();
                keyHit.stop();
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
        keyHit.start();
        m_timer.start();
        context.run();
    }

};

int main(int argc, char* argv[])
{
    globalLevel = Level::warning;

    if (argc != 2) {
        std::cerr << "usage "<<argv[0] <<" <playlist file>\n";
        return EXIT_FAILURE;
    }

    auto audioPlayer = std::make_unique<AudioPlayer>(argv[1]);
    audioPlayer->run();

    return EXIT_SUCCESS;
}
