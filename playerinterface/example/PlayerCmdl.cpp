#include <iostream>
#include "KeyHit.h"
#include <boost/asio.hpp>
#include <memory>
#include <functional>
#include <iomanip>
#include "playerinterface/Player.h"
#include "playerinterface/mpvplayer.h"
#include "common/repeattimer.h"


int main(int argc, char* argv[])
{
    globalLevel = Level::info;
    boost::asio::io_context context;
    KeyHit keyHit;
    std::shared_ptr<Player> player(new MpvPlayer(context,""));

    using namespace std::chrono_literals;
    RepeatTimer timer(context, 20ms);

    std::string playlistFilename {"/usr/local/var/audioserver/playlist/f7f10f6b-6527-4b6a-91b3-c2da1db14ebb.m3u"};
    if (argc == 2) {
        playlistFilename = std::string(argv[1]);
    }

    auto keyHandler =
            [&context, &player, &timer, playlistFilename](const char key) {
    // get player info into correct context
    context.post([&player, &key, &playlistFilename, &timer]() {
        switch (key) {
        case 'q': {
            player->stop();
            player->stopPlayer();
            timer.stop();
            break;
        }
        case 'p': {
            std::string url {playlistFilename};
            player->startPlay(std::move(url), "");
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
        case ' ': {
            player->pause();
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

    timer.setHandler([&player]() {
        if (player->isPlaying()) {
            auto file = player->getSong();
            auto pos  = player->getSongPercentage();
            std::cout.precision(2);
            std::cout.setf(std::ios::floatfield,std::ios::fixed);
            std::cout << "\e[?25l"<<std::setw(40) << file << " "<< std::setw(6) << pos/100.0 << " \t" << std::string((pos+1)/200, '#') << "     \r" <<std::flush;
       }
    });

    timer.start();

    player->setPlayerEndCB([](const std::string& name){
        boost::ignore_unused(name);
        std::cout << "\nEND";
    });

    player->setSongEndCB([&player](){
        if (player->isPlaying())
            std::cout << "\n";
    });


    context.run();

    std::cout << "\e[?25h\n";

}
