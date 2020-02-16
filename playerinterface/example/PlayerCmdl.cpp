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

int main(int argc, char* argv[])
{
    globalLevel = Level::info;

    if (argc != 2) {
        std::cerr << "usage "<<argv[0] <<" <playlist file>\n";
        return EXIT_FAILURE;
    }

    boost::asio::io_context context;
    KeyHit keyHit;
    std::unique_ptr<BasePlayer> player(new MpvPlayer(context));
    RepeatTimer timer(context, 100ms);
    boost::asio::signal_set signals(context, SIGINT);

    auto playlistFilename = std::string(argv[1]);

    auto keyHandler =
            [&context, &player, &timer, &signals, playlistFilename, &keyHit](const char key) {
    // get player info into correct context
    context.post([&player, &timer, &signals, &playlistFilename, &keyHit, key]() {
        switch (key) {
        case 'q': {
            player->stop();
            player->stopPlayerConnection();
            timer.stop();
            signals.cancel();
            keyHit.stop();
            break;
        }
        case 'p': {
            std::string url {playlistFilename};
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
    });
    };

    keyHit.setKeyReceiver(keyHandler);
    keyHit.start();

    timer.setHandler([&player]() {
        if (player->isPlaying()) {
            auto file = player->getSong();
            auto pos  = player->getSongPercentage();
            std::cout.precision(2);
            std::cout.setf(std::ios::floatfield, std::ios::fixed);
            std::cout << "\e[?25l"<<std::setw(40) << file << " "<< std::setw(6) << pos/100.0 << " \t" << std::string((pos+1)/200, '#') << "       \r" <<std::flush;
       }
    });

    timer.start();

    player->setPlayerEndCallBack([](){
        std::cout << "\nstopped\n";
    });

    player->setSongEndCB([&player](const std::string& ){
        if (player->isPlaying())
            std::cout << "\n";
    });

    signals.async_wait([&player, &timer, &keyHit](const boost::system::error_code& ec, int ) {
        if (!ec) {
            player->stop();
            player->stopPlayerConnection();
            timer.stop();
            keyHit.stop();
        }
    });

    context.run();

    std::cout << "\e[?25h\n";

}
