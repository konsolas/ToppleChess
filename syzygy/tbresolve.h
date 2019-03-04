//
// Created by Vincent on 02/03/2019.
//

#ifndef TOPPLE_TBRESOLVE_H
#define TOPPLE_TBRESOLVE_H

#include <vector>
#include <atomic>
#include "../board.h"
#include "../eval.h"

bool resolve_pv(board_t &pos, evaluator_t &evaluator, std::vector<move_t> &pv,
        int &score, size_t max_ply, const std::atomic_bool &aborted);

#endif //TOPPLE_TBRESOLVE_H
