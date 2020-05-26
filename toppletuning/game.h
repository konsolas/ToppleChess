//
// Created by Vincent on 23/05/2020.
//

#ifndef TOPPLE_GAME_H
#define TOPPLE_GAME_H

#include "../search.h"

enum GameResult {
    UNKNOWN = -1, WIN = 2, DRAW = 1, LOSS = 0
};

// Represents a self play game
class game_t {
    const processed_params_t &white;
    const processed_params_t &black;
    const search_limits_t &limits;

public:
    game_t(const processed_params_t &white, const processed_params_t &black, const search_limits_t &limits) :
            white(white), black(black), limits(limits) {}

    // Reported relative to white
    GameResult play(board_t pos);

private:
    static GameResult check_result(board_t &pos);
};


#endif //TOPPLE_GAME_H
