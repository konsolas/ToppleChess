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

    struct heuristic_set_t {
        killer_heur_t killers;
        history_heur_t history;
    };
}

struct search_limits_t {
    // Game situation
    explicit search_limits_t(int now, int time, int inc, int moves_to_go) {
        game_situation = true;

        // Set other limits
        depth_limit = MAX_PLY;
        node_limit = UINT64_MAX;

        if(moves_to_go > 0) {
            // Repeating time controls
            suggested_time_limit = (time / (moves_to_go + 1));
            suggested_time_limit += inc / 2;
            hard_time_limit = suggested_time_limit * 3;
            if(moves_to_go > 1 && hard_time_limit > time / 2) {
                hard_time_limit = time / 2;
            }
        } else {
            // Other time controls
            suggested_time_limit = (time / std::max(50 - now, 30));
            suggested_time_limit += inc / 2;
            hard_time_limit = suggested_time_limit * 3;
        }

        // Clamp both time limits
        suggested_time_limit = std::clamp(suggested_time_limit, 10, std::max(10, time - 50));
        hard_time_limit = std::clamp(hard_time_limit, 10, std::max(10, time - 50));
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

struct search_result_t {
    move_t best_move{};
    move_t ponder{};
};

class search_t {
    friend class movesort_t;

    struct context_t {
        // Board representation
        board_t board;

        // Heuristics
        search_heur::heuristic_set_t heur;

        // Evaluator
        evaluator_t evaluator = evaluator_t(eval_params_t(), 16 * MB);

        // SMP
        int tid = 0;
    };
public:
    explicit search_t(board_t board, tt::hash_t *tt, unsigned int threads, size_t syzygy_resolve, search_limits_t limits);

    search_result_t think(const std::atomic_bool &aborted);
    void enable_timer();
    void wait_for_timer();
private:
    int search_aspiration(int prev_score, int depth, const std::atomic_bool &aborted, int &n_legal);
    int search_root(context_t &context, int alpha, int beta, int depth, const std::atomic_bool &aborted, int &n_legal);

    template<bool PV, bool H>
    int search_ab(context_t &context, int alpha, int beta, int ply, int depth, bool can_null, move_t excluded,
                  const std::atomic_bool &aborted);
    template<bool PV, bool H>
    int search_qs(context_t &context, int alpha, int beta, int ply, const std::atomic_bool &aborted);

    void save_pv();
    bool has_pv();
    void update_pv(int ply, move_t move);

    bool keep_searching(int depth);
    bool is_aborted(const std::atomic_bool &aborted);
    bool is_root_move(move_t move);

    void print_stats(board_t &board, int score, int depth, tt::Bound bound, const std::atomic_bool &aborted);

    // Shared structures
    tt::hash_t *tt;
    unsigned int threads;
    size_t syzygy_resolve;
    search_limits_t limits;

    // Timing
    std::mutex timer_mtx;
    std::condition_variable timer_cnd;
    bool timer_started = false;
    std::chrono::steady_clock::time_point timer_start;
    std::chrono::steady_clock::time_point start;

    // PV table (main thread only)
    int pv_table_len[MAX_PLY] = {};
    move_t pv_table[MAX_PLY][MAX_PLY] = {{}};
    int last_pv_len = 0;
    move_t last_pv[MAX_PLY] = {};

    // Main search context
    context_t main_context;

    // Endgame tablebases
    int use_tb;

    // Global stats
    U64 nodes = 0;
    U64 tbhits = 0;
    U64 fhf = 0;
    U64 fh = 0;
    U64 nulltries = 0;
    U64 nullcuts = 0;
    size_t sel_depth;
};

#endif //TOPPLE_SEARCH_H
