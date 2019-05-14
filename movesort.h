//
// Created by Vincent on 30/06/2018.
//

#ifndef TOPPLE_MOVESORT_H
#define TOPPLE_MOVESORT_H

#include "movegen.h"

enum GenMode {
    NORMAL, QUIESCENCE
};

enum GenStage {
    GEN_NONE,
    GEN_HASH,
    GEN_GOOD_CAPT,
    GEN_KILLER_1,
    GEN_KILLER_2,
    GEN_KILLER_3,
    GEN_BAD_CAPT,
    GEN_QUIETS,
};

// History heuristic
class history_heur_t {
public:
    void good_history(move_t good_move, int depth) {
        int bonus = depth * depth;
        tableHist(good_move, bonus);
    }

    void bad_history(move_t bad_move, int depth) {
        int penalty = -depth * depth;
        tableHist(bad_move, penalty);
    }

    int get(move_t move) const {
        return historyTable[move.info.team][move.info.from][move.info.to];
    }
private:
    // Indexed by [TEAM][FROM][TO]
    int historyTable[2][64][64] = {};

    // Intenal update routine
    void tableHist(move_t move, int delta) {
        historyTable[move.info.team][move.info.from][move.info.to] += delta;
        if (historyTable[move.info.team][move.info.from][move.info.to] > 1000000000) {
            for (auto &h : historyTable) {
                for (auto &i : h) {
                    for (int &j : i) {
                        j /= 16;
                    }
                }
            }
        }
    }
};

// Killer heuristic
class killer_heur_t {
public:
    void update(move_t killer, int ply) {
        if (!killer.info.is_capture && killer != killers[ply][0]) {
            killers[ply][1] = killers[ply][0];
            killers[ply][0] = killer;
        }
    }

    move_t primary(int ply) const {
        return killers[ply][0];
    }

    move_t secondary(int ply) const {
        return killers[ply][1];
    }

private:
    move_t killers[MAX_PLY][2] = {{}};
};

struct heuristic_set_t {
    killer_heur_t killers;
    history_heur_t history;
};

class movesort_t {
public:
    movesort_t(GenMode mode, const heuristic_set_t &heuristics, const board_t &board, move_t hash_move, move_t refutation, int ply);

    move_t next(GenStage &stage, int &score);
    move_t *generated_quiets(int &count);
private:
    GenMode mode;
    const heuristic_set_t &heur;
    const board_t &board;
    move_t hash_move;
    move_t refutation;

    move_t killer_1, killer_2, killer_3;

    void buf_swap_main(int a, int b);
    void buf_swap_capt(int a, int b);

    movegen_t gen;

    int main_idx = 0;
    int main_buf_size = 0;
    move_t main_buf[128];
    int main_scores[128];

    int capt_idx = 0;
    int capt_buf_size = 0;
    move_t capt_buf[64];
    int capt_scores[64];
};


#endif //TOPPLE_MOVESORT_H
