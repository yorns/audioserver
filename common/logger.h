#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <sstream>

namespace LoggerFramework {

enum class Level {
    debug,
    info,
    warning,
    error
};

extern Level globalLevel;

extern void setGlobalLevel(Level level);

template<class IO>
class Logger
{
private:
    Level m_actualLevel { Level::debug };
    IO& m_stream;

public:

    Logger(IO& stream) : m_stream(stream) {}

    Logger& operator()(Level level) {
        m_actualLevel = level;
        return *this;
    }

    template <typename T>
    Logger& operator<<(T value) {
        if (m_actualLevel >= globalLevel && m_stream.good())
            m_stream << value << std::flush;
        return *this;
    }

    IO& getStream() { return m_stream; }

};

}

extern LoggerFramework::Logger<std::ostream> logger_intern;
#define logger(l) logger_intern(l) << std::dec << __FILE__ << ":" << __LINE__ << " # "

extern std::fstream loggerFile;
extern LoggerFramework::Logger<std::fstream> file_logger_intern;
#define fileLog(l) file_logger_intern(l) << std::dec << __FILE__ << ":" << __LINE__ << " # "

#endif // LOGGER_H
