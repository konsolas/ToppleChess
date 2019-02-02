//
// Created by Vincent on 30/09/2017.
//

#ifndef TOPPLE_SEARCH_H
#define TOPPLE_SEARCH_H

#include <future>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <utility>
#include <vector>
#include <cmath>

#include "board.h"
#include "move.h"
#include "eval.h"

namespace search_heur {
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
            if (!killer.info.is_capture) {
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
}

struct search_limits_t {
    // Game situation
    explicit search_limits_t(int now, int time, int inc, int moves_to_go) {
        game_situation = true;

        // Set other limits
        depth_limit = MAX_PLY;
        node_limit = UINT64_MAX;

        // Handle time control
        {
            // Try and use a consistent amount of time per move
            suggested_time_limit = (time /
                          (moves_to_go > 0 ? moves_to_go + 1 : std::max(50 - now, 30)));

            // Handle increment: Look to gain some time if there isn't much left
            suggested_time_limit += inc / 2;

            // Allow search to use up to 3x as much time as suggested.
            hard_time_limit = suggested_time_limit * 3;

            // Clamp both time limits
            suggested_time_limit = std::clamp(suggested_time_limit, 0, time - 50);
            hard_time_limit = std::clamp(hard_time_limit, 0, time - 50);
        }
    }

    // Custom search
    explicit search_limits_t(int move_time, int depth, U64 max_nodes, std::vector<move_t> root_moves) :
            hard_time_limit(move_time), depth_limit(depth), node_limit(max_nodes), root_moves(std::move(root_moves)) {
        game_situation = false;
        suggested_time_limit = hard_time_limit;
    }

    bool game_situation;
    int suggested_time_limit;

    int hard_time_limit;
    int depth_limit;
    U64 node_limit;
    std::vector<move_t> root_moves = std::vector<move_t>();
};

struct search_context_t {
    board_t board;

    // Heuristics
    search_heur::killer_heur_t h_killer;
    search_heur::history_heur_t h_history;

    // SMP
    int tid = 0;
};

class search_t {
    friend class movesort_t;
public:
    explicit search_t(board_t board, evaluator_t &evaluator, tt::hash_t *tt, unsigned int threads, search_limits_t limits);

    move_t think(const std::atomic_bool &aborted);
private:
    int search_aspiration(int prev_score, int depth, const std::atomic_bool &aborted, int &n_legal);
    int search_root(search_context_t &context, int alpha, int beta, int depth, const std::atomic_bool &aborted, int &n_legal);

    template<bool PV, bool H>
    int search_ab(search_context_t &context, int alpha, int beta, int ply, int depth, bool can_null, move_t excluded,
                  const std::atomic_bool &aborted);
    template<bool PV, bool H>
    int search_qs(search_context_t &context, int alpha, int beta, int ply, const std::atomic_bool &aborted);

    void save_pv();
    bool has_pv();
    void update_pv(int ply, move_t move);

    bool keep_searching(int depth);
    bool is_aborted(const std::atomic_bool &aborted);
    bool is_root_move(move_t move);

    void print_stats(int score, int depth, tt::Bound bound);

    // Shared structures
    evaluator_t &evaluator;
    tt::hash_t *tt;
    std::chrono::steady_clock::time_point start;
    unsigned int threads;
    search_limits_t limits;

    // PV table (main thread only)
    int pv_table_len[MAX_PLY] = {};
    move_t pv_table[MAX_PLY][MAX_PLY] = {{}};
    int last_pv_len = 0;
    move_t last_pv[MAX_PLY] = {};

    // Main search context
    search_context_t main_context;

    // Global stats
    U64 nodes = 0;
    U64 fhf = 0;
    U64 fh = 0;
    U64 nulltries = 0;
    U64 nullcuts = 0;
    int sel_depth;
};

#endif //TOPPLE_SEARCH_H
