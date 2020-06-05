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
    GEN_GOOD_NOISY,
    GEN_QUIETS,
    GEN_BAD_NOISY,
};

// History heuristic
class history_heur_t {
public:
    void update(move_t move, int bonus) {
        table[move.info.team][move.info.from][move.info.to] +=
                bonus - table[move.info.team][move.info.from][move.info.to] * abs(bonus) / 16384;
    }

    int get(move_t move) const {
        return table[move.info.team][move.info.from][move.info.to];
    }
private:
    // Indexed by [TEAM][FROM][TO]
    int16_t table[2][64][64] = {};
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
    history_heur_t history;
    killer_heur_t killers;
};

class movesort_t {
public:
    movesort_t(GenMode mode, const heuristic_set_t &heuristics, const board_t &board, move_t hash_move, move_t refutation, int ply);

    move_t next(GenStage &stage, int &score, bool skip_quiets);
    move_t *generated_quiets(size_t &count);
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
    int bad_capt_idx = 0;
    int bad_capt_buf_size = 0;
    move_t capt_buf[64];
};


#endif //TOPPLE_MOVESORT_H
