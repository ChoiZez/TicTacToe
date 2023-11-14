#ifndef TICTACTOE_H
#define TICTACTOE_H

#include <set>
#include <cstddef>

class TicTacToe {
private:
    size_t _cells;
    size_t _moveCnt;
    size_t _fieldSize;
    bool _turn;
    char *field;

    bool isWonDiag() {
        std::set<char> s;
        for (size_t diag = 0; diag < _cells; ++diag) {
            s.insert(field[diag * _cells + diag]);
        }
        if (!s.contains(0) && s.size() == 1) {
            return true;
        }
        s.clear();
        for (size_t diag = 0; diag < _cells; ++diag) {
            s.insert(field[diag * _cells + (_cells - 1 - diag)]);
        }
        return (!s.contains(0) && s.size() == 1);
    }

    bool isWonVert() {
        for (size_t col = 0; col < _cells; ++col) {
            std::set<char> s;
            for (size_t row = 0; row < _cells; ++row) {
                s.insert(field[_cells * row + col]);
            }
            if (!s.contains(0) && s.size() == 1) {
                return true;
            }
        }
        return false;
    }

    bool isWonHor() {
        for (size_t row = 0; row < _cells; ++row) {
            std::set<char> s;
            for (size_t col = 0; col < _cells; ++col) {
                s.insert(field[_cells * row + col]);
            }
            if (!s.contains(0) && s.size() == 1) {
                return true;
            }
        }
        return false;
    }

public:
    explicit TicTacToe(size_t cells = 3) : _cells(cells), _moveCnt(0), _fieldSize(_cells * _cells), _turn(false) {
        field = new char[_fieldSize]{0};
    }

    ~TicTacToe() {
        delete[] field;
    }

    void setCell(size_t cellID) {
        field[cellID] = _turn ? 'X' : 'O';
        ++_moveCnt;
        _turn ^= 1;
    }

    bool isWon() {
        return isWonDiag() || isWonVert() || isWonHor();
    }

    [[nodiscard]] bool isDraw() const {
        return _moveCnt == _fieldSize;
    }

    [[nodiscard]] bool getTurn() const {
        return _turn;
    }

    void clear() {
        delete[] field;
        field = new char[_fieldSize]{0};
        _moveCnt = 0;
    }
};


#endif