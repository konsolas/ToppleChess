//
// Created by Vincent on 30/09/2017.
//

#ifndef TOPPLE_SEARCH_H
#define TOPPLE_SEARCH_H

#include <future>
#include "board.h"

class search_t {
public:
    explicit search_t(board_t board, tt::hash_t *tt);

    move_t think(int n_threads, int max_depth);

    template<bool PV> int searchAB(int alpha, int beta, int ply, int depth, bool can_null);
    template<bool PV> int searchQS(int alpha, int beta, int ply);

    void save_pv();
private:
    volatile bool is_active = false;
    board_t board;
    tt::hash_t *tt;

    // PV
    int pv_table_len[MAX_PLY] = {0};
    move_t pv_table[MAX_PLY][MAX_PLY] = {{0}};
    int last_pv_len = 0;
    move_t last_pv[MAX_PLY] = {0};

    // Stats
    U64 nodes = 0;
    U64 hashhit = 0;
    U64 hashcut = 0;
    U64 fhf = 0;
    U64 fh = 0;
    U64 nulltries = 0;
    U64 nullcuts = 0;
};


#endif //TOPPLE_SEARCH_H
