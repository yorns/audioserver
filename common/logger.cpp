#include "logger.h"


LoggerFramework::Level LoggerFramework::globalLevel { LoggerFramework::Level::debug };

void LoggerFramework::setGlobalLevel(LoggerFramework::Level level)
    { LoggerFramework::globalLevel = level; }

LoggerFramework::Logger<std::ostream> logger_intern(std::cout);

std::fstream loggerFile;
LoggerFramework::Logger<std::fstream> file_logger_intern(loggerFile);
