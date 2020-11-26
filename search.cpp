//
// Created by Vincent on 30/09/2017.
//

#include <utility>
#include <vector>
#include <algorithm>

#include "search.h"

#include "syzygy/tbprobe.h"
#include "syzygy/tbresolve.h"

search_t::search_t(tt::hash_t *tt, const processed_params_t &params, int threads, bool silent)
        : tt(tt), params(params), limits(nullptr), silent(silent) {

    // Create an evaluator for each thread

    std::function<void(worker_t*)> worker_loop = [this] (worker_t *worker) {
        while (!worker->terminated) {
            std::unique_lock<std::mutex> lock(worker->mutex);
            worker->cv.wait(lock, [worker] () { return worker->searching || worker->terminated; });
            if (worker->terminated) break;
            thread_start(worker->context, *worker->aborted, worker);
            worker->searching = false;
            worker->promise.set_value();
        }
    };

    for (size_t i = 0; i < threads; i++) {
        workers.emplace_back(std::make_unique<worker_t>(i, std::ref(params), 8 * MB, std::ref(worker_loop)));
    }
}

search_t::~search_t() {
    for (auto &worker : workers) {
        {
            std::lock_guard<std::mutex> lock(worker->mutex);
            worker->terminated = true;
        }
        worker->cv.notify_one();
    }

    for (auto &worker : workers) {
        worker->thread.join();
    }
}

search_result_t search_t::think(board_t &board, const search_limits_t &search_limits, std::atomic_bool &aborted) {
    start = engine_clock::now();
    this->limits = &search_limits;

    // Initialise root moves
    root_moves.clear();
    {
        move_t buf[192];
        movegen_t gen(board);
        int pseudo_legal = gen.gen_normal(buf);

        // Copy legal moves to root_moves
        std::copy_if(buf, buf + pseudo_legal, std::back_inserter(root_moves),
                     [board](move_t move) { return board.is_legal(move); });

        // Find intersection of root moves with UCI searchmoves
        if (!search_limits.search_moves.empty()) {
            root_moves.erase(std::remove_if(root_moves.begin(), root_moves.end(), [&search_limits] (move_t move) {
                return std::find(search_limits.search_moves.begin(), search_limits.search_moves.end(), move) ==
                       search_limits.search_moves.end();
            }), root_moves.end());
        }
    }

    // Probe tablebases at root if no root moves specified
    int use_tb = TBlargest;
    int tb_wdl = 0;

    // the UCI searchmoves option overrides tablebase root move filtering
    if (search_limits.search_moves.empty() && pop_count(board.bb_all) <= TBlargest) {
        std::vector<move_t> tb_root_moves = root_probe(board, tb_wdl);

        if (!tb_root_moves.empty()) {
            // DTZ tablebases available: set root moves and don't use tablebases during search.
            root_moves = std::move(tb_root_moves);
            use_tb = 0;
        } else {
            tb_root_moves = root_probe_wdl(board, tb_wdl);

            if (!tb_root_moves.empty()) {
                // WDL tablebases available: set root moves and only use tablebases during search if we're winning.
                root_moves = std::move(tb_root_moves);
                if (tb_wdl <= 0) {
                    use_tb = 0;
                }
            }
        }
    }

    // Check for nonsense
    if (root_moves.empty()) {
        std::cerr << "warn: no legal moves found at the root position" << std::endl;
        return {EMPTY_MOVE, EMPTY_MOVE};
    }

    // Instantly exit if there is only one legal move in a game
    if (limits->game_situation && root_moves.size() == 1) {
        // Try to obtain a ponder move from the transposition table
        board.move(root_moves[0]);

        tt::entry_t h = {};
        move_t ponder_move = EMPTY_MOVE;
        if (tt->probe(board.record.back().hash, h)) {
            ponder_move = board.to_move(h.info.move);
        }

        board.unmove();

        return {root_moves[0], ponder_move};
    }

    // Start workers
    std::vector<std::future<void>> futures;
    for (auto &worker : workers) {
        // Initialise worker
        worker->board = board;
        worker->context = pvs::context_t(&worker->board, &worker->evaluator, tt, use_tb);
        worker->aborted = &aborted;

        std::promise<void> promise = std::promise<void>();
        futures.push_back(promise.get_future());
        worker->promise = std::move(promise);

        // Notify worker thread
        {
            std::lock_guard<std::mutex> lock(worker->mutex);
            worker->searching = true;
        }
        worker->cv.notify_one();
    }

    // Make sure the timer has started for ponder
    wait_for_timer();

    // Limit time for main thread
#ifdef TOPPLE_TUNE
    futures[0].wait();
#else
    if (futures[0].wait_for(std::chrono::milliseconds(search_limits.hard_time_limit)) !=
        std::future_status::ready) {
        aborted = true;
        futures[0].wait();
    }
#endif

    // Then abort and wait for all the helper threads
    aborted = true;
    for (size_t tid = 1; tid < workers.size(); tid++) {
        futures[tid].wait();
    }

    // Read the PV
    std::vector<move_t> pv = workers[0]->context.get_saved_pv();
    if (pv.empty()) {
        std::cerr << "warn: insufficient time to search to depth 1" << std::endl;
        return {EMPTY_MOVE, EMPTY_MOVE};
    }

    // If we don't have a ponder move (e.g. if we're currently failing high but the search was aborted), look in the tt.
    if (pv.size() == 1) {
        board.move(pv[0]);

        tt::entry_t h = {};
        move_t ponder_move = EMPTY_MOVE;
        if (tt->probe(board.record.back().hash, h)) {
            ponder_move = board.to_move(h.info.move);
        }

        board.unmove();

        pv.push_back(ponder_move);
    }

    return {pv[0], pv[1]};
}

void search_t::enable_timer() {
    {
        std::unique_lock<std::mutex> lock(timer_mtx);
        timer_started = true;
        timer_cnd.notify_all();
    }
    timer_start = engine_clock::now();
}

void search_t::wait_for_timer() {
    std::unique_lock<std::mutex> lock(timer_mtx);
    while (!timer_started) timer_cnd.wait(lock);
}

void search_t::reset_timer() {
    timer_started = false;
}

void search_t::thread_start(pvs::context_t &context, const std::atomic_bool &aborted, worker_t *worker) {
    int prev_score = 0;

    // Check if this is the main thread
    if (worker->tid == 0) {
        // Scale search time based on the complexity of the position
        float tapering_factor = worker->evaluator.game_phase(*context.get_board());
        float complexity = std::clamp(root_moves.size() / 30.0F, 0.5F, 8.0F) * tapering_factor + (1 - tapering_factor);
        int adjusted_suggestion = static_cast<int>(complexity * (float) limits->suggested_time_limit);

        for (int depth = 1; depth <= MAX_PLY && (worker->tid != 0 || keep_searching(depth)); depth++) {
            int score = search_aspiration(context, prev_score, depth, aborted, worker->tid);
            if (aborted) break;

            // If in game situation, try and manage time
            if (limits->game_situation && timer_started) {
                auto elapsed = CHRONO_DIFF(timer_start, engine_clock::now());

                // See if we can justify ending the search early, as long as we're not doing badly
                if (depth <= 5 || score > prev_score - 10) {
                    // If it's unlikely that we'll search deeper
                    if (elapsed > adjusted_suggestion * 0.7) {
                        break;
                    }
                }
            }

            prev_score = score;
        }
    } else {
        for (int depth = 1; depth <= MAX_PLY; depth++) {
            int score = search_aspiration(context, prev_score, depth, aborted, worker->tid);
            if (aborted) break;
            prev_score = score;
        }
    }
}

int search_t::search_aspiration(pvs::context_t &context, int prev_score, int depth, const std::atomic_bool &aborted,
                                size_t tid) {
    const int ASPIRATION_DELTA = 15;

    int alpha, beta, delta = ASPIRATION_DELTA;

    if (depth >= 6) {
        alpha = std::max(-INF, prev_score - delta);
        beta = std::min(INF, prev_score + delta);
    } else {
        alpha = -INF, beta = INF;
    }

    auto time = CHRONO_DIFF(start, engine_clock::now());

    int researches = 0;
    int score;

    // Check if we need to search again
    while (true) {
        assert(alpha <= beta);

        if (!silent && tid == 0 && time > 1000) {
            score = context.search_root(root_moves,
                                        [this, &context, tid, depth, &aborted](int score) {
                                            print_stats(*context.get_board(), score, depth, tt::EXACT, aborted);
                                        },
                                        [](int num, move_t move) {
                                            std::cout << "info currmove " << move << " currmovenumber " << num
                                                      << std::endl;
                                        },
                                        alpha, beta, depth, aborted);
        } else {
            score = context.search_root(root_moves, nullptr, nullptr, alpha, beta, depth, aborted);
        }
        if (score == TIMEOUT) return score;

        researches++;
        // Only save the principal variation if the bound is LOWER or EXACT
        if (score >= beta) {
            context.save_pv();
            if (!silent && tid == 0) {
                print_stats(*context.get_board(), score, depth, tt::LOWER, aborted);
            }
            beta = std::min(INF, beta + delta);
        } else if (score <= alpha) {
            if (!silent && tid == 0) {
                print_stats(*context.get_board(), score, depth, tt::UPPER, aborted);
            }
            beta = (alpha + beta) / 2;
            alpha = std::max(-INF, alpha - delta);
        } else {
            context.save_pv();
            break;
        }

        delta = delta + delta / 2;
    }

    if (!silent && tid == 0) {
        print_stats(*context.get_board(), score, depth, tt::EXACT, aborted);
    }

    return score;
}

bool search_t::keep_searching(int depth) {
    return workers[0]->context.get_saved_pv().empty()
            || ((limits->node_limit == UINT64_MAX || count_nodes() <= limits->node_limit)
            && depth <= limits->depth_limit
            && (!timer_started || CHRONO_DIFF(timer_start, engine_clock::now()) <= limits->hard_time_limit));
}

U64 search_t::count_nodes() {
    U64 total_nodes = 0;
    for (auto &worker : workers) {
        total_nodes += worker->context.get_nodes();
    }

    return total_nodes;
}

U64 search_t::count_tb_hits() {
    U64 total_tb_hits = 0;
    for (auto &worker : workers) {
        total_tb_hits += worker->context.get_tb_hits();
    }

    return total_tb_hits;
}

void search_t::print_stats(board_t &pos, int score, int depth, tt::Bound bound, const std::atomic_bool &aborted) {
    U64 nodes = count_nodes();
    auto time = CHRONO_DIFF(start, engine_clock::now());

    // Get an appropriate PV
    std::vector<move_t> pv = bound == tt::EXACT ? workers[0]->context.get_current_pv() : workers[0]->context.get_saved_pv();

    // Try syzygy resolution
    auto sel_depth = size_t(workers[0]->context.get_sel_depth());
    if (limits->syzygy_resolve > 0) {
        resolve_pv(pos, workers[0]->evaluator, pv, score, limits->syzygy_resolve, aborted);
        sel_depth = std::max(pv.size(), sel_depth);
    }

    std::cout << "info depth " << depth << " seldepth " << sel_depth;

    if (score > MINCHECKMATE) {
        std::cout << " score mate " << ((TO_MATE_PLY(score) + 1) / 2);
    } else if (score < -MINCHECKMATE) {
        std::cout << " score mate -" << ((TO_MATE_PLY(-score) + 1) / 2);
    } else {
        std::cout << " score cp " << score;
    }

    if (bound == tt::UPPER) {
        std::cout << " upperbound";
    } else if (bound == tt::LOWER) {
        std::cout << " lowerbound";
    }

    std::cout << " time " << time
              << " nodes " << nodes
              << " nps " << (nodes / (time + 1)) * 1000;
    if (time > 1000) {
        std::cout << " hashfull " << tt->hash_full()
                  << " tbhits " << count_tb_hits();
    }
    std::cout << " pv ";
    for (const auto &move : pv) {
        std::cout << move << " ";
    }

    std::cout << std::endl;
}
