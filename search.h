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
    explicit search_limits_t(int time, int inc, int moves_to_go) {
        game_situation = true;

        // Set other limits
        depth_limit = MAX_PLY;
        node_limit = UINT64_MAX;
        search_moves = std::vector<move_t>();

        // Use the same time management for repeating and fixed time controls
        if(moves_to_go == 0) moves_to_go = 60;

        suggested_time_limit = (time / (moves_to_go + 1)) + inc;
        hard_time_limit = std::min(suggested_time_limit * 8, time / std::min(4, moves_to_go));

        // Clamp both time limits
        hard_time_limit = std::max(10, std::min(hard_time_limit, time));
        suggested_time_limit = std::max(1, std::min(suggested_time_limit, hard_time_limit));
    }

    // Custom search
    explicit search_limits_t(int move_time, int depth, U64 max_nodes, std::vector<move_t> search_moves) :
            hard_time_limit(move_time), depth_limit(depth), node_limit(max_nodes), search_moves(std::move(search_moves)) {
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
    std::vector<move_t> search_moves;
    size_t syzygy_resolve = 512;
};

struct search_result_t {
    move_t best_move;
    move_t ponder;
};

class search_t {
    struct worker_t {
        size_t tid;
        evaluator_t evaluator;

        board_t board;
        pvs::context_t context;
        std::atomic_bool *aborted = nullptr;

        std::thread thread;
        std::promise<void> promise;

        bool searching = false;
        bool terminated = false;
        std::mutex mutex;
        std::condition_variable cv;

        worker_t(size_t tid, const processed_params_t &eval_params, size_t pawn_hash_size, const std::function<void(worker_t*)>& runnable) :
            tid(tid), evaluator(eval_params, pawn_hash_size) {
            thread = std::thread(runnable, this);
        }
        worker_t(const worker_t &) = delete;
    };
public:
    explicit search_t(tt::hash_t *tt, const processed_params_t &params, int threads, bool silent = false);
    ~search_t();
    search_t(const search_t&) = delete;
    search_t(const search_t&&) = delete;

    search_result_t think(board_t &board, const search_limits_t &limits, std::atomic_bool &aborted);
    void enable_timer();
    void wait_for_timer();
    void reset_timer();
private:
    void thread_start(pvs::context_t &context, const std::atomic_bool &aborted, worker_t *worker);
    int search_aspiration(pvs::context_t &context, int prev_score, int depth, const std::atomic_bool &aborted, size_t tid);

    bool keep_searching(int depth);

    U64 count_nodes();
    U64 count_tb_hits();
    void print_stats(board_t &board, int score, int depth, tt::Bound bound, const std::atomic_bool &aborted);

    bool silent;

    // Shared structures
    tt::hash_t *tt;
    const processed_params_t &params;
    search_limits_t const *limits;
    std::vector<move_t> root_moves;

    // Workers
    std::vector<std::unique_ptr<worker_t>> workers;

    // Timing
    std::mutex timer_mtx;
    std::condition_variable timer_cnd;
    bool timer_started = false;
    std::chrono::steady_clock::time_point timer_start;
    std::chrono::steady_clock::time_point start;
};

#endif //TOPPLE_SEARCH_H
