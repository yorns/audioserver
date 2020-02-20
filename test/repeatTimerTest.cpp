#include <boost/asio.hpp>
#include "common/logger.h"
#include "common/repeattimer.h"

using namespace std::chrono_literals;
using namespace LoggerFramework;

int main()
{
    boost::asio::io_context context;
    boost::asio::signal_set signals(context, SIGINT);

    //RepeatTimer timer(context, std::chrono::milliseconds(500));
    RepeatTimer timer(context, 500ms);

    timer.setHandler([]()
    {   static int i{0};
        logger(Level::info) << "I was here ( x "<<i++<<" )\n";
    });
    timer.start();

    signals.async_wait([&timer](const boost::system::error_code&, int )
    {   timer.stop();
        logger(Level::info) << "END\n";
    });

    context.run();

    return EXIT_SUCCESS;
}
