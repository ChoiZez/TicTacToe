#include "logger.h"

Logger::Logger(const std::string &filePath) try {
    logFile.open(filePath);
    if (!logFile.is_open()) {
        throw std::invalid_argument("Can't open log file: " + filePath);
    }
} catch (const std::exception &e) {
    std::cerr << e.what();
}

Logger::~Logger() {
    logFile.close();
}

void Logger::log(const Logger::logType logType, const std::string &message) {
    const auto currentTime = std::chrono::system_clock::now();
    const auto t_c = std::chrono::system_clock::to_time_t(currentTime);
    const auto gmt_time = gmtime(&t_c);
    logFile << std::put_time(gmt_time, "%Y-%m-%d %H:%M:%S") << " - [" << logMapper[logType] << "] " << message
            << std::endl;
}