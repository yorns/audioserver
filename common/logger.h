#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <sstream>

namespace LoggerFramework {

enum Level {
    debug   = 0,
    info    = 1,
    warning = 2,
    error   = 3
};

extern Level globalLevel;

class Logger
{
private:
    Level actualLevel { Level::debug };

public:

    Logger& operator()(Level level) {
        actualLevel = level;
        return *this;
    }

    template <typename T>
    Logger& operator<<(T value) {
        if (actualLevel >= globalLevel)
            std::cout << value;
        return *this;
    }

};

}

extern LoggerFramework::Logger logger_intern;

#define logger(l) logger_intern(l) << __FILE__ << ":" << __LINE__ << " # "

#endif // LOGGER_H
