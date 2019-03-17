//
// Created by Vincent on 07/03/2019.
//

#ifndef TOPPLE_SMP_H
#define TOPPLE_SMP_H

#include "move.h"
#include "search.h"

struct smp_move_t {
    move_t move;
    int move_number;
    int reduced_depth;
    int depth;

    smp_move_t(move_t move, int move_number, int reduced_depth, int depth)
        : move(move), move_number(move_number), reduced_depth(reduced_depth), depth(depth) {}
};

class parallel_move_searcher_t {
    bool parallel_enabled;
    semaphore_t &semaphore;
    search_t &search;

    const int ply, alpha, beta;
    const std::atomic_bool &aborted;
public:
    parallel_move_searcher_t(bool parallel_enabled, semaphore_t &semaphore, search_t &search, int ply, int alpha, int beta, const std::atomic_bool &aborted);

    // Blocking, could return a move that failed high at any time, for any searched move
    void queue_zws(smp_move_t move);
    std::vector<smp_move_t> get_failed_high();
    bool has_failed_high();
private:
    void fail_high(smp_move_t move);

    std::mutex failed_vec_mutex;
    std::vector<smp_move_t> failed_vec;

    std::vector<std::future<void>> futures;
};


#endif //TOPPLE_SMP_H
