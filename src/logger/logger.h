#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <string>
#include <map>
#include <iomanip>

class Logger {
public:
    explicit Logger(const std::string &);

    ~Logger();

    enum logType {
        INFO,
        DEBUG,
        WARNING,
        ERROR
    };

    void log(logType, const std::string &);

private:
    std::ofstream logFile;
    std::map<logType, std::string> logMapper{
            {logType::INFO,    "INFO"},
            {logType::DEBUG,   "DEBUG"},
            {logType::WARNING, "WARNING"},
            {logType::ERROR,   "ERROR"},
    };
};

#endif