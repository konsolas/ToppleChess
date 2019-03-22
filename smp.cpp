//
// Created by Vincent on 07/03/2019.
//

#include "smp.h"
#include "search.h"

parallel_move_searcher_t::parallel_move_searcher_t(bool parallel_enabled, semaphore_t &semaphore, search_t &search,
        int ply, int alpha, int beta, const std::atomic_bool &aborted)
        : parallel_enabled(parallel_enabled), semaphore(semaphore), search(search), ply(ply), alpha(alpha), beta(beta), aborted(aborted) {
    failed_vec.reserve(32);
}

std::atomic_ulong created;

void parallel_move_searcher_t::queue_zws(smp_move_t move) {
    if(parallel_enabled) {
        // Copy the main context
        search_t::context_t copy = search.main_context;

        semaphore.wait();

        futures.emplace_back(std::async(std::launch::async | std::launch::deferred, [this, move]
                (search_t::context_t sub_context, size_t index) {
            sub_context.nodes = 0;

            int score;
            bool full_search = true;
            if (move.reduced_depth < move.depth) {
                score = -search.search_zw(sub_context, -alpha - 1, -alpha, ply + 1, move.reduced_depth,
                                          true, EMPTY_MOVE, true, aborted);
                full_search = score > alpha;
            }

            if (aborted) {
                search.main_context.nodes += sub_context.nodes;
                semaphore.release();
                return;
            }

            if (full_search) {
                score = -search.search_zw(sub_context, -alpha - 1, -alpha, ply + 1, move.depth,
                                          true, EMPTY_MOVE, true, aborted);
            }
            search.main_context.nodes += sub_context.nodes;

            if (!aborted && alpha < score) {
                fail_high(move);
            }

            semaphore.release();
        }, copy, futures.size()));
    } else {
        // Just search it with the main context
        int score;
        bool full_search = true;
        if (move.reduced_depth < move.depth) {
            score = -search.search_zw(search.main_context, -alpha - 1, -alpha, ply + 1, move.reduced_depth,
                                      true, EMPTY_MOVE, true, aborted);
            full_search = score > alpha;
        }

        if (aborted) {
            return;
        }

        if (full_search) {
            score = -search.search_zw(search.main_context, -alpha - 1, -alpha, ply + 1, move.depth,
                                      true, EMPTY_MOVE, true, aborted);
        }

        if (!aborted && alpha < score) {
            fail_high(move);
        }
    }
}

void parallel_move_searcher_t::fail_high(smp_move_t move) {
    std::lock_guard<decltype(failed_vec_mutex)> lock(failed_vec_mutex);
    failed_vec.push_back(move);
}

std::vector<smp_move_t> parallel_move_searcher_t::get_failed_high() {
    futures.clear();
    return failed_vec;
}

bool parallel_move_searcher_t::has_failed_high() {
    return !failed_vec.empty();
}
