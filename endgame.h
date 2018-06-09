//
// Created by Vincent on 09/06/2018.
//

#ifndef TOPPLE_ENDGAME_H
#define TOPPLE_ENDGAME_H

#include <optional>

#include "board.h"

struct eg_eval_t {
    eg_eval_t(bool valid, int eval) : valid(valid), eval(eval) {};

    bool valid;
    int eval;
};

/**
 * Initialise evaluation tables. Must be called before {@link eval}
 */
void eg_init();

/**
 * Return an evaluation of an endgame in {@code board}, relative to the side to move.
 *
 * {@link eg_init> must have been called first
 *
 * @param board
 * @return evaluation relative to WHITE
 */
eg_eval_t eval_eg(const board_t &board);

#endif //TOPPLE_ENDGAME_H
