//
// Created by Vincent Tang on 2019-01-05.
//

#ifndef TOPPLE_PGN_H
#define TOPPLE_PGN_H

#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include "../board.h"

// TODO: Unfinished

struct game_t {
    board_t board;
    double result;
    std::vector<std::string> comments;
};

class pgn_t {
    std::stringstream pgn_file;
public:
    pgn_t(std::string &pgn_file);
    std::vector<game_t> read_games();

private:
    void skip_pgn_header(std::stringstream &stream);
    move_t read_san(board_t &board, std::string san_move);
};


#endif //TOPPLE_PGN_H
