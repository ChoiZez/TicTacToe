#ifndef GAMESESSION_H
#define GAMESESSION_H

#include <vector>

#include "tictactoe.h"

class gameSession : public TicTacToe{
private:
    std::vector<int> users;
public:
    void restart(){
        clear();
        users.clear();
    }

    void addUser(int i) {
        users.push_back(i);
    }

    [[nodiscard]] const std::vector<int> &getUsers() const {
        return users;
    }
};


#endif
