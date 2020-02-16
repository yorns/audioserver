#include "common/logger.h"

using namespace LoggerFramework;

void printIt()
{
    logger(Level::debug)    << "This is a  debug   message\n";
    logger(Level::info)     << "This is an info    message\n";
    logger(Level::warning)  << "This is a  warning message\n";
    logger(Level::error)    << "This is an error   message\n";

    logger(Level::info) << " -- info with parameters: " << 42 << " " << 3.14159 << " " << std::string_view("(string view data)") << " characters\n";

    logger(Level::error) << "\n";
}

int main()
{
    globalLevel = Level::debug;
    printIt();
    globalLevel = Level::info;
    printIt();
    globalLevel = Level::warning;
    printIt();
    globalLevel = Level::error;
    printIt();

    return EXIT_SUCCESS;
}
