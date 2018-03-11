//
// Created by Vincent on 30/09/2017.
//

#ifndef TOPPLE_EVAL_H
#define TOPPLE_EVAL_H

#include "types.h"
#include "board.h"

/**
 * Initialise evaluation tables. Must be called before {@link eval}
 */
void eval_init();

/**
 * Return an evaluation of {@code board}, relative to the side to move.
 *
 * {@link eval_init> must have been called first
 *
 * @param board
 * @return evaluation relative to the side to move
 */
int eval(const board_t &board);

#endif //TOPPLE_EVAL_H
