//
// Created by Vincent on 30/09/2017.
//

#ifndef TOPPLE_SEARCH_H
#define TOPPLE_SEARCH_H

#include <future>
#include <algorithm>
#include <cstring>
#include "board.h"

// History heuristic
class history_heur_t {
public:
    history_heur_t() {
        memset(historyTable, 0, sizeof(historyTable));
    }

    void update(move_t goodmove_t, move_t *previous, int len, int depth) {
        int bonus = depth * depth;

        table(goodmove_t, bonus);

        for(int i = 0; i < len; i++) {
            move_t prev = previous[i];
            if (!prev.is_capture) {
                table(prev, -bonus);
            }
        }

        if(get(goodmove_t) > 20000) {
            throttle();
        }
    }

    int get(move_t move) {
        return historyTable[move.piece][move.to];
    }
private:
    int historyTable[16][64] = {0}; // Indexed by [PIECETYPE][TOSQUARE]

    // Intenal update routine
    void table(move_t move, int delta) {
        historyTable[move.piece][move.to] += delta;
    }

    // Throttle the history heuristic to be bounded by +- 20000
    void throttle() {
        // Divide everything in the history table by 2.
        for(int i = 0; i < 16; i++) {
            for(int j = 0; j < 64; j++) {
                if (historyTable[i][j]) {
                    historyTable[i][j] /= 2;
                }
            }
        }
    }
};

// Killer heuristic
class killer_heur_t {
public:
    killer_heur_t() {
        memset(killers, 0, sizeof(killers));
    }
    void update(move_t killer, int ply) {
        if(!killer.is_capture) {
            killers[ply][1] = killers[ply][0];
            killers[ply][0] = killer;
        }
    }
    move_t primary(int ply) {
        return killers[ply][0];
    }
    move_t secondary(int ply) {
        return killers[ply][1];
    }
private:
    move_t killers[MAX_PLY][2] = {0};
};

class search_t {
    friend class movegen_t;
public:
    explicit search_t(board_t board, tt::hash_t *tt);

    move_t think(int n_threads, int max_depth, const std::atomic_bool &aborted);
    template<bool H> int searchAB(board_t &board, int alpha, int beta, int ply, int depth, bool can_null, const std::atomic_bool &aborted);
    int searchQS(board_t &board, int alpha, int beta, int ply, const std::atomic_bool &aborted);

    void save_pv();
private:
    board_t board;
    tt::hash_t *tt;

    // PV
    int pv_table_len[MAX_PLY] = {0};
    move_t pv_table[MAX_PLY][MAX_PLY] = {{EMPTY_MOVE}};
    int last_pv_len = 0;
    move_t last_pv[MAX_PLY] = {EMPTY_MOVE};

    // Heuristics
    killer_heur_t killer_heur;
    history_heur_t history_heur;

    // Stats
    U64 nodes = 0;
    U64 ntnodes = 0;
    U64 fhf = 0;
    U64 fh = 0;
    U64 hit = 0;
    U64 hash_try = 0;
    U64 cut = 0;
};

#endif //TOPPLE_SEARCH_H
