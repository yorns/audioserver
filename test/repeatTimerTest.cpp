#include <boost/asio.hpp>
#include "common/repeattimer.h"

using namespace std::chrono_literals;

int main()
{
    boost::asio::io_context context;
    boost::asio::signal_set signals(context, SIGINT);

    //RepeatTimer timer1(context,std::chrono::milliseconds(500));
    RepeatTimer timer1(context, 500ms);

    signals.async_wait([&timer1](const boost::system::error_code&, int ) { timer1.stop(); std::cout << "END\n"; });

    timer1.setHandler([](){ static int i{0}; std::cout << "I was here ( x "<<i++<<" )\n"; });
    timer1.start();

    context.run();

    return EXIT_SUCCESS;
}
