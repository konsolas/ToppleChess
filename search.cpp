//
// Created by Vincent on 30/09/2017.
//

#include "search.h"
#include "eval.h"
#include "movegen.h"

int from_hash_score(const int v, int ply) {
    return (v < LOW ? v + ply : (v > HIGH ? v - ply : v));
}

int to_hash_score(const int v, int ply) {
    return (v < LOW ? v - ply : (v > HIGH ? v + ply : v));
}

move_t search_t::think(int n_threads, int max_depth) {
    nodes = 0;
    // Run sync if nthreads = 0
    int alpha = -INF, beta = INF;
    auto start = engine_clock::now();
    U64 prev_nodes = 1;
    if (n_threads == 0) {
        for (int depth = 1; depth <= max_depth; depth++) {
            int score = searchAB<true>(alpha, beta, 0, depth, true);
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

            prev_nodes = nodes;
            std::cout << std::endl;
        }
    }

    return last_pv[0];
}

search_t::search_t(board_t board, tt::hash_t *tt) : board(board), tt(tt) {}

template<bool PV>
int search_t::searchAB(int alpha, int beta, int ply, int depth, bool can_null) {
    nodes++;
    pv_table_len[ply] = ply;

    // Quiescence search
    if (depth < 1) return searchQS<PV>(alpha, beta, ply + 1);

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
    hashhit++;
    tt::entry_t h = {0};
    if (tt->probe(board.record[board.now].hash, h)) {
        score = from_hash_score(h.value, ply);

        if ((h.depth >= depth || score <= ply - MATE(0) || score > MATE(0) - ply - 1)) {
            if (h.bound == tt::LOWER && score >= beta) return score;
            if (h.bound == tt::UPPER && score <= alpha) return score;
            if (!PV && h.bound == tt::EXACT) return score;
        }
    }

    if(h.move.move_bytes) hashcut++;

    // Check extension
    bool in_check = board.is_incheck();
    if (in_check) depth++;

    // Forward pruning
    /*
    if (!in_check && !PV) {
        // Stand pat
        int stand_pat = eval(board);

        // Eval-based pruning at frontier and pre-frontier nodes
        if(depth < 4) {
            int delta = 256 * depth - 128;
            if (stand_pat > beta + delta) return beta;
            if (stand_pat < alpha - delta && searchQS<false>(alpha - delta, alpha - delta + 1, ply) < alpha - delta)
                return alpha;
        }

        // Null move pruning
        if (stand_pat >= beta) {
            int reduction = 2 + depth / 4;
            board.move(EMPTY_MOVE);
            int null_score = -searchAB<false>(-beta, -beta + 1, ply, depth - reduction, false);
            board.unmove();

            nulltries++;

            if (null_score >= beta) {
                if (searchAB<false>(beta - 1, beta, ply, depth - reduction, false) >= beta) {
                    nullcuts++;
                    return beta;
                }
            }
        }
    }
    */

    movegen_t gen(board);
    //if (h.move.move_bytes && board.is_pseudo_legal(h.move)) gen.add_move(h.move);
    int moves = gen.gen_normal();
    int n_legal = 0;
    for (int idx = 0; idx < moves; idx++) {
        if (idx != 0 && gen.buf[idx] == h.move) continue;

        board.move(gen.buf[idx]);
        if (board.is_illegal()) {
            board.unmove();
            continue;
        } else {
            n_legal++; // Legal move

            if (pv) {
                score = -searchAB<true>(-beta, -alpha, ply + 1, depth - 1, can_null);
            } else {
                score = -searchAB<false>(-alpha - 1, -alpha, ply + 1, depth - 1, can_null);
                if (score > alpha) {
                    pv = true;
                    // We'd rather not need to search again, but :/
                    score = -searchAB<true>(-beta, -alpha, ply + 1, depth - 1, can_null);
                }
            }

            board.unmove();

            if (score > best_score) {
                best_score = score;
                best_move = gen.buf[idx];

                if (score >= beta) {
                    if (n_legal == 1) {
                        fhf++;
                    }
                    fh++;

                    tt->save(tt::LOWER, board.record[board.now].hash, depth, to_hash_score(best_score, ply), best_move);

                    return beta; // Fail hard
                }

                if (score > alpha) {
                    alpha = score;
                    pv = false;

                    if (PV) {
                        pv_table[ply][ply] = gen.buf[idx];
                        for (int i = ply + 1; i < pv_table_len[ply + 1]; i++) {
                            pv_table[ply][i] = pv_table[ply + 1][i];
                        }
                        pv_table_len[ply] = pv_table_len[ply + 1];
                    }
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

    if (best_move.move_bytes) {
        if (alpha > old_alpha) {
            tt->save(tt::EXACT, board.record[board.now].hash, depth, to_hash_score(best_score, ply), best_move);
        } else {
            tt->save(tt::UPPER, board.record[board.now].hash, depth, to_hash_score(best_score, ply), best_move);
        }
    }

    return alpha;
}

template<bool PV>
int search_t::searchQS(int alpha, int beta, int ply) {
    nodes++;

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
            int score = -searchQS<PV>(-beta, -alpha, ply + 1);

            board.unmove();

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

