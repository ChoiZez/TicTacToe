#ifndef USERDATA_H
#define USERDATA_H

#include <string>

struct userData {
    std::string password;
    bool isLogged;
    bool isPlaying;
    size_t activeSession;
};

#endif
