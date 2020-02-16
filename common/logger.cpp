#include "logger.h"

LoggerFramework::Level LoggerFramework::globalLevel { LoggerFramework::Level::debug };
LoggerFramework::Logger logger_intern;
