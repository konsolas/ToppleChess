#include <utility>

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
#include <mutex>
#include <condition_variable>

#include "board.h"
#include "move.h"
#include "eval.h"
#include "movesort.h"
#include "pvs.h"

struct search_limits_t {
    // Game situation
    explicit search_limits_t(int now, int time, int inc, int moves_to_go) {
        game_situation = true;

        // Set other limits
        depth_limit = MAX_PLY;
        node_limit = UINT64_MAX;
        root_moves = std::vector<move_t>();

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
            suggested_time_limit = (time / std::max(60 - now, 40));
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

    // Time limits
    bool game_situation;
    int suggested_time_limit;
    int hard_time_limit;

    // Other limits
    int depth_limit;
    U64 node_limit;
    std::vector<move_t> root_moves;

    // Resource limits
    size_t threads = 1;
    size_t syzygy_resolve = 512;
};

struct search_result_t {
    move_t best_move{};
    move_t ponder{};
    int root_depth = 0;
};

class search_t {
public:
    explicit search_t(board_t board, tt::hash_t *tt, evaluator_t &evaluator, const search_limits_t &limits);

    search_result_t think(std::atomic_bool &aborted);
    void enable_timer();
    void wait_for_timer();
private:
    void thread_start(pvs::context_t &context, const std::atomic_bool &aborted, int tid);
    int search_aspiration(pvs::context_t &context, int prev_score, int depth, const std::atomic_bool &aborted, int tid);

    bool keep_searching(int depth);

    U64 count_nodes();
    U64 count_tb_hits();
    void print_stats(board_t &board, int score, int depth, tt::Bound bound, const std::atomic_bool &aborted);

    board_t board;

    // Shared structures
    tt::hash_t *tt;
    evaluator_t &evaluator;
    const search_limits_t &limits;

    // Threads
    std::vector<board_t> boards;
    std::vector<pvs::context_t> contexts;

    // Root moves
    std::vector<move_t> root_moves;

    // Timing
    std::mutex timer_mtx;
    std::condition_variable timer_cnd;
    bool timer_started = false;
    std::chrono::steady_clock::time_point timer_start;
    std::chrono::steady_clock::time_point start;
};

#endif //TOPPLE_SEARCH_H
