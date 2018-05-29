//
// Created by Vincent on 30/09/2017.
//

#include "search.h"
#include "eval.h"
#include "movegen.h"

const int TIMEOUT = -INF * 2;

move_t search_t::think(int n_threads, int max_depth, const std::atomic_bool &aborted) {
    nodes = 0;
    // Run sync if nthreads = 0
    int alpha = -INF, beta = INF;
    auto start = engine_clock::now();
    if (n_threads > 0) {
        // Helper threads
        std::future<int> helper_threads[n_threads - 1];
        board_t helper_boards[n_threads - 1];
        for (int i = 0; i < n_threads - 1; i++) {
            helper_boards[i] = board;
        }
        std::atomic_bool helper_thread_aborted;

        for (int depth = 1; depth <= max_depth; depth++) {
            helper_thread_aborted = false;

            // Start helper threads (lazy SMP)
            for (int i = 0; i < n_threads - 1; i++) {
                helper_threads[i] = std::async(std::launch::async,
                                               search_t::searchAB<true, true>, this,
                                               std::ref(helper_boards[i]), alpha, beta, 0, depth, true,
                                               std::ref(helper_thread_aborted));
            }

            int score = searchAB<false, true>(board, alpha, beta, 0, depth, true, aborted);

            // Abort helper threads
            helper_thread_aborted = true;

            if (!aborted) {
                save_pv();

                auto current = engine_clock::now();
                std::cout << "info depth " << depth << " score cp " << score
                          << " time " << CHRONO_DIFF(start, current)
                          << " nodes " << nodes
                          << " nps " << (nodes / (CHRONO_DIFF(start, current) + 1)) * 1000
                          << " pv ";
                for (int i = 0; i < last_pv_len; i++) {
                    std::cout << from_sq(last_pv[i].from) << from_sq(last_pv[i].to) << " ";
                }

                std::cout << std::endl;
            } else {
                break;
            }
        }
    }

    return last_pv[0];
}

search_t::search_t(board_t board, tt::hash_t *tt) : board(board), tt(tt) {}

template<bool H, bool PV>
int search_t::searchAB(board_t &board, int alpha, int beta, int ply, int depth, bool can_null,
                       const std::atomic_bool &aborted) {
    nodes++;
    if(!H) pv_table_len[ply] = ply;

    if (aborted) {
        return TIMEOUT;
    }

    // Quiescence search
    if (depth < 1) return searchQS<PV>(board, alpha, beta, ply + 1, aborted);

    // Search variables
    bool pv = PV;
    int score, best_score = -INF;
    const int old_alpha = alpha;
    move_t best_move{};

    // Game state
    if (board.record[board.now].halfmove_clock >= 100 || board.is_repetition_draw()) return 0;

    // Mate distance pruning
    if (ply) {
        alpha = std::max(-MATE(ply), alpha);
        beta = std::min(MATE(ply + 1), beta);
    }

    // Probe transposition table
    tt::entry_t h = {0};
    if (tt->probe(board.record[board.now].hash, h)) {
        score = h.value;

        if ((h.depth >= depth || score <= ply - MATE(0) || score > MATE(0) - ply - 1)) {
            if(!PV) {
                if (h.bound == tt::LOWER && score >= beta) return score;
                if (h.bound == tt::UPPER && score <= alpha) return score;
                if (h.bound == tt::EXACT) return score;
            }
        }
    }

    // Check extension
    bool in_check = board.is_incheck();
    if (in_check) depth++;

    // Forward pruning
    /*
    if (!in_check && !PV) {
        // Stand pat
        int stand_pat = eval(board);

        // Eval-based pruning at frontier nodes
        if(depth < 2) {
            int delta = 256 * depth - 128;
            if (stand_pat > beta + delta) return beta;
            if (stand_pat < alpha - delta && searchQS<false>(board, alpha - delta, alpha - delta + 1, ply, aborted) < alpha - delta)
                return alpha;
        }

        // Null move pruning
        if (can_null && stand_pat >= beta) {
            int reduction = 2;
            board.move(EMPTY_MOVE);
            int null_score = -searchAB<H, false>(board, -beta, -beta + 1, ply, depth - reduction, false, aborted);
            board.unmove();

            if (aborted) {
                return TIMEOUT;
            }

            nulltries++;

            if (null_score >= beta) {
                if (searchAB<H, false>(board, beta - 1, beta, ply, depth - reduction, false, aborted) >= beta) {
                    nullcuts++;
                    return beta;
                }
            }
        }
    }
    */


    movegen_t gen(board);
    int moves = gen.gen_normal();
    int n_legal = 0;
    for (int idx = 0; idx < moves; idx++) {
        board.move(gen.buf[idx]);
        if (board.is_illegal()) {
            board.unmove();
            continue;
        } else {
            n_legal++; // Legal move

            if (pv) {
                score = -searchAB<H, true>(board, -beta, -alpha, ply + 1, depth - 1, can_null, aborted);
            } else {
                score = -searchAB<H, false>(board, -alpha - 1, -alpha, ply + 1, depth - 1, can_null, aborted);
                if (score > alpha) {
                    pv = true;
                    // We'd rather not need to search again, but :/
                    score = -searchAB<H, true>(board, -beta, -alpha, ply + 1, depth - 1, can_null, aborted);
                }
            }
            board.unmove();

            if (aborted) return TIMEOUT;

            if (score > best_score && (best_score = score) > alpha) {
                best_move = gen.buf[idx];

                alpha = score;
                pv = false;
                tt->save(score > beta ? tt::LOWER : tt::EXACT, board.record[board.now].hash, depth, best_score, best_move);

                if (!H && PV) {
                    pv_table[ply][ply] = gen.buf[idx];
                    for (int i = ply + 1; i < pv_table_len[ply + 1]; i++) {
                        pv_table[ply][i] = pv_table[ply + 1][i];
                    }
                    pv_table_len[ply] = pv_table_len[ply + 1];
                }

                if (score >= beta) {
                    if (n_legal == 1) {
                        fhf++;
                    }
                    fh++;

                    tt->save(tt::LOWER, board.record[board.now].hash, depth, best_score, best_move);

                    return beta; // Fail hard
                }
            }
        }
    }

    if (n_legal == 0) {
        if (in_check) {
            return -MATE(ply);
        } else {
            return 0;
        }
    }

    if(best_score < old_alpha){
        tt->save(tt::UPPER, board.record[board.now].hash, depth, best_score, best_move);
    }

    return alpha;
}

template<bool PV>
int search_t::searchQS(board_t &board, int alpha, int beta, int ply, const std::atomic_bool &aborted) {
    nodes++;

    if (aborted) return TIMEOUT;

    int stand_pat;

    stand_pat = eval(board);

    if (stand_pat >= beta) return beta;
    if (alpha < stand_pat) alpha = stand_pat;

    movegen_t gen(board);
    int n_moves = gen.gen_caps();
    for (int idx = 0; idx < n_moves; idx++) {
        board.move(gen.buf[idx]);
        if (board.is_illegal()) {
            board.unmove();
            continue;
        } else {
            int score = -searchQS<PV>(board, -beta, -alpha, ply + 1, aborted);
            board.unmove();

            if (aborted) return TIMEOUT;

            if (score >= beta) {
                return beta;
            }
            if (score > alpha) {
                alpha = score;
            }
        }
    }

    return alpha;
}

void search_t::save_pv() {
    last_pv_len = pv_table_len[0];
    for (int i = 0; i < pv_table_len[0]; i++) {
        last_pv[i] = pv_table[0][i];
    }
}

