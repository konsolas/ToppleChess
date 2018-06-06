//
// Created by Vincent on 30/09/2017.
//

#include <vector>

#include "search.h"
#include "eval.h"
#include "movegen.h"

const int TIMEOUT = -INF * 2;

void print_score(int score) {
    if(score > MINCHECKMATE) {
        std::cout << " score mate " << ((TO_MATE_PLY(score) + 1) / 2);
    } else if (score < -MINCHECKMATE) {
        std::cout << " score mate -" << ((TO_MATE_PLY(-score) + 1) / 2);
    } else {
        std::cout << " score cp " << score;
    }
}

move_t search_t::think(unsigned int n_threads, int max_depth, const std::atomic_bool &aborted) {
    nodes = 0;
    // Run sync if nthreads = 0
    int alpha = -INF, beta = INF;
    auto start = engine_clock::now();
    if (n_threads > 0) {
        // Helper threads
        std::vector<std::future<int>> helper_threads(n_threads - 1);
        std::vector<board_t> helper_boards(n_threads - 1);
        for (int i = 0; i < n_threads - 1; i++) {
            helper_boards[i] = board;
        }
        std::atomic_bool helper_thread_aborted;

        for (int depth = 1; depth <= max_depth; depth++) {
            helper_thread_aborted = false;

            // Start helper threads (lazy SMP)
            for (int i = 0; i < n_threads - 1; i++) {
                helper_threads[i] = std::async(std::launch::async,
                                               &search_t::searchAB<true>, this,
                                               std::ref(helper_boards[i]), alpha, beta, 0, depth, true,
                                               std::ref(helper_thread_aborted));
            }

            int score = searchAB<false>(board, alpha, beta, 0, depth, true, aborted);

            // Abort helper threads
            helper_thread_aborted = true;

            if (!is_aborted(aborted)) {
                save_pv();

                auto current = engine_clock::now();
                auto time = CHRONO_DIFF(start, current);
                std::cout << "info depth " << depth;
                print_score(score);
                std::cout << " time " << time
                          << " nodes " << nodes
                          << " nps " << (nodes / (time + 1)) * 1000;
                /*
                if (time > 2000) {
                    std::cout << " hashfull " << tt->hash_full()
                              << " hashhitrate " << ((double) hit / hash_try)
                              << " hashcutrate " << ((double) cut / hit)
                              << " mbf " << ((double) nodes / ntnodes)
                              << " betacutoff " << ((double) fhf / fh);
                }
                */
                std::cout << " pv ";
                for (int i = 0; i < last_pv_len; i++) {
                    std::cout << from_sq(last_pv[i].info.from) << from_sq(last_pv[i].info.to) << " ";
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

template<bool H>
int search_t::searchAB(board_t &board, int alpha, int beta, int ply, int depth, bool can_null,
                       const std::atomic_bool &aborted) {
    nodes++;
    if (!H) pv_table_len[ply] = ply;

    const bool PV = alpha + 1 < beta;

    if (is_aborted(aborted)) {
        return TIMEOUT;
    }

    // Quiescence search
    if (depth < 1) return searchQS(board, alpha, beta, ply + 1, aborted);
    else ntnodes++;

    // Search variables
    int score; const int old_alpha = alpha; move_t best_move{};

    // Game state
    if (board.record[board.now].halfmove_clock >= 100 || board.is_repetition_draw()) return 0;

    // Mate distance pruning
    if (ply) {
        alpha = std::max(-TO_MATE_SCORE(ply), alpha);
        beta = std::min(TO_MATE_SCORE(ply + 1), beta);
    }

    // Check extension
    bool in_check = board.is_incheck();
    if (in_check) depth++;

    // Probe transposition table
    tt::entry_t h = {0};
    hash_try++;
    if (tt->probe(board.record[board.now].hash, h)) {
        score = h.value(ply);
        hit++;

        if (!PV && h.depth >= depth) {
            if (h.bound == tt::LOWER && score >= beta) return score;
            if (h.bound == tt::UPPER && score <= alpha) return score;
            if (h.bound == tt::EXACT) return score;
        }
    }

    GenStage stage = GEN_NONE;
    movegen_t gen(board);
    gen.gen_normal();
    int n_legal = 0;
    while (gen.has_next()) {
        move_t move = gen.next(stage, *this, h.move, ply);
        board.move(move);
        if (board.is_illegal()) {
            board.unmove();
            continue;
        } else {
            n_legal++; // Legal move

            if (PV && n_legal == 1) {
                score = -searchAB<H>(board, -beta, -alpha, ply + 1, depth - 1, can_null, aborted);
            } else {
                score = -searchAB<H>(board, -alpha - 1, -alpha, ply + 1, depth - 1, can_null, aborted);
                if (alpha < score && score < beta) {
                    // Research if the zero window search failed.
                    score = -searchAB<H>(board, -beta, -alpha, ply + 1, depth - 1, can_null, aborted);
                }
            }
            board.unmove();

            if (is_aborted(aborted)) return TIMEOUT;

            if (score > alpha) {
                alpha = score;
                best_move = move;

                if (!H && PV) {
                    pv_table[ply][ply] = move;
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

                    tt->save(tt::LOWER, board.record[board.now].hash, depth, ply, score, best_move);

                    if (!move.info.is_capture) {
                        int len;
                        move_t *moves = gen.get_searched(len);
                        history_heur.update(move, moves, len, depth);
                        killer_heur.update(move, ply);
                    }

                    return beta; // Fail hard
                }
            }
        }
    }

    if (n_legal == 0) {
        if (in_check) {
            return -TO_MATE_SCORE(ply);
        } else {
            return 0;
        }
    }

    if (alpha > old_alpha) {
        tt->save(tt::EXACT, board.record[board.now].hash, depth, ply, alpha, best_move);
        if (!best_move.info.is_capture) history_heur.update(best_move, nullptr, 0, depth);
    } else {
        tt->save(tt::UPPER, board.record[board.now].hash, depth, ply, alpha, best_move);
    }

    return alpha;
}

int search_t::searchQS(board_t &board, int alpha, int beta, int ply, const std::atomic_bool &aborted) {
    nodes++;

    if (is_aborted(aborted)) return TIMEOUT;

    int stand_pat = eval(board);

    if (stand_pat >= beta) return beta;
    if (alpha < stand_pat) alpha = stand_pat;

    GenStage stage;
    movegen_t gen(board);
    gen.gen_caps();
    while (gen.has_next()) {
        move_t move = gen.next(stage, *this, EMPTY_MOVE, ply);
        if (stage == GEN_BAD_CAPT) break;

        board.move(move);
        if (board.is_illegal()) {
            board.unmove();
            continue;
        } else {
            int score = -searchQS(board, -beta, -alpha, ply + 1, aborted);
            board.unmove();

            if (is_aborted(aborted)) return TIMEOUT;

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

bool search_t::has_pv() {
    return last_pv[0] != EMPTY_MOVE;
}

bool search_t::is_aborted(const std::atomic_bool &aborted) {
    return aborted && has_pv();
}
