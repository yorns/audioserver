#include "common/logger.h"

using namespace LoggerFramework;

void printIt()
{
    logger(Level::debug)    << "This is a  debug   message\n";
    logger(Level::info)     << "This is an info    message\n";
    logger(Level::warning)  << "This is a  warning message\n";
    logger(Level::error)    << "This is an error   message\n";

    logger(Level::info) << " -- info with parameters: " << 42
                        << " " << 3.14159
                        << " " << std::string_view("(string view data)")
                        << " characters\n";

    logger(Level::error) << "\n";
}

void FileIt()
{
    fileLog(Level::debug)    << "This is a  debug   message\n";
    fileLog(Level::info)     << "This is an info    message\n";
    fileLog(Level::warning)  << "This is a  warning message\n";
    fileLog(Level::error)    << "This is an error   message\n";

    fileLog(Level::info) << " -- info with parameters: " << 42
                         << " " << 3.14159 << " "
                         << std::string_view("(string view data)")
                         << " characters\n";

    fileLog(Level::error) << "\n";
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

    loggerFile.open("test.log", std::ios::out);
    globalLevel = Level::debug;
    FileIt();
    globalLevel = Level::info;
    FileIt();
    globalLevel = Level::warning;
    FileIt();
    globalLevel = Level::error;
    FileIt();
    loggerFile.close();

    return EXIT_SUCCESS;
}
