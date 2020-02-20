#include <boost/asio.hpp>
#include "common/repeattimer.h"

using namespace std::chrono_literals;

int main()
{
    boost::asio::io_context context;
    boost::asio::signal_set signals(context, SIGINT);

    //RepeatTimer timer(context, std::chrono::milliseconds(500));
    RepeatTimer timer(context, 500ms);

    timer.setHandler([](){ static int i{0}; std::cout << "I was here ( x "<<i++<<" )\n"; });
    timer.start();

    signals.async_wait([&timer](const boost::system::error_code&, int ) { timer.stop(); std::cout << "END\n"; });

    context.run();

    return EXIT_SUCCESS;
}
