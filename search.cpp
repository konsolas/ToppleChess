//
// Created by Vincent on 30/09/2017.
//

#include <utility>
#include <vector>

#include "search.h"
#include "eval.h"
#include "movegen.h"
#include "move.h"

const int TIMEOUT = -INF * 2;

search_t::search_t(board_t board, tt::hash_t *tt, unsigned int threads, search_limits_t limits)
        : board(board), tt(tt), threads(threads), limits(std::move(limits)) {}

move_t search_t::think(const std::atomic_bool &aborted) {
    nodes = 0;
    start = engine_clock::now();
    if (threads > 0) {
        // Helper threads
        std::vector<std::future<int>> helper_threads(threads - 1);
        std::vector<board_t> helper_boards(threads - 1);
        for (unsigned int i = 0; i < threads - 1; i++) {
            helper_boards[i] = board;
        }
        std::atomic_bool helper_thread_aborted;

        int prev_score = 0;
        for (int depth = 1; keep_searching(depth); depth++) {
            sel_depth = 0;
            helper_thread_aborted = false;

            // Start helper threads (lazy SMP)
            if(depth > 5) {
                for (unsigned int i = 0; i < threads - 1; i++) {
                    helper_threads[i] = std::async(std::launch::async,
                                                   &search_t::search_ab<true>, this,
                                                   std::ref(helper_boards[i]), -INF, INF, 0, depth, true, EMPTY_MOVE,
                                                   std::ref(helper_thread_aborted));
                }
            }

            prev_score = search_aspiration(prev_score, depth, aborted);

            // Stop helper threads
            helper_thread_aborted = true;
            for (unsigned int i = 0; i < threads - 1; i++) {
                helper_threads[i] = {};
            }

            if (is_aborted(aborted)) {
                break;
            }
        }
    }

    return last_pv[0];
}

int search_t::search_aspiration(int prev_score, int depth, const std::atomic_bool &aborted) {
    const int ASPIRATION_SIZE[] = {40, 100, INF};

    int alpha;
    int beta;

    if(depth > 4) {
        alpha = prev_score - ASPIRATION_SIZE[0];
        beta = prev_score + ASPIRATION_SIZE[0];
    } else {
        alpha = -INF, beta = INF;
    }

    int researches = 0;
    int score = search_root(board, alpha, beta, depth, aborted);

    if(score == TIMEOUT) return score;

    // Check if we need to search again
    while (score >= beta || score <= alpha) {
        researches++;
        if (score >= beta) {
            print_stats(score, depth, tt::LOWER);
            beta = prev_score + ASPIRATION_SIZE[researches];
        } else if (score <= alpha) {
            print_stats(score, depth, tt::UPPER);
            alpha = prev_score - ASPIRATION_SIZE[researches];
        }

        score = search_root(board, alpha, beta, depth, aborted);
        if(score == TIMEOUT) return score;
    }

    save_pv();
    print_stats(score, depth, tt::EXACT);

    return score;
}

int search_t::search_root(board_t &board, int alpha, int beta, int depth, const std::atomic_bool &aborted) {
    nodes++;
    pv_table_len[0] = 0;

    // Check extension
    bool in_check = board.is_incheck();
    if (in_check) depth++;

    // Search variables
    int score;
    const int old_alpha = alpha;
    move_t best_move{};

    // Probe transposition table
    tt::entry_t h = {0}; tt->probe(board.record[board.now].hash, h);

    GenStage stage = GEN_NONE;
    int move_score;
    movegen_t gen(board);
    gen.gen_normal();
    int n_legal = 0;
    while (gen.has_next()) {
        move_t move = gen.next(stage, move_score, *this, h.move, 0);

        if(!is_root_move(move)) {
            continue;
        }

        board.move(move);
        if (board.is_illegal()) {
            board.unmove();
            continue;
        } else {
            n_legal++; // Legal move

            // Display current move information
            if(CHRONO_DIFF(start, engine_clock::now()) > 2000) {
                std::cout << "info currmove " << move << " currmovenumber " << n_legal << std::endl;
            }

            if (n_legal == 1) {
                score = -search_ab<false>(board, -beta, -alpha, 1, depth - 1, true, EMPTY_MOVE, aborted);
            } else {
                score = -search_ab<false>(board, -alpha - 1, -alpha, 1, depth - 1, true, EMPTY_MOVE, aborted);
                if (alpha < score && score < beta) {
                    // Research if the zero window search failed.
                    score = -search_ab<false>(board, -beta, -alpha, 1, depth - 1, true, EMPTY_MOVE, aborted);
                }
            }
            board.unmove();

            if (is_aborted(aborted)) return TIMEOUT;

            if (score > alpha) {
                alpha = score;
                best_move = move;

                pv_table[0][0] = move;
                for (int i = 1; i < pv_table_len[1]; i++) {
                    pv_table[0][i] = pv_table[1][i];
                }
                pv_table_len[0] = pv_table_len[1];

                if (score >= beta) {
                    if (n_legal == 1) {
                        fhf++;
                    }
                    fh++;

                    tt->save(tt::LOWER, board.record[board.now].hash, depth, 0, score, best_move);

                    if (!move.info.is_capture) {
                        int len;
                        move_t *moves = gen.get_searched(len);
                        history_heur.update(move, moves, len, depth);
                        killer_heur.update(move, 0);
                    }

                    return beta; // Fail hard
                }
            }
        }
    }

    if (n_legal == 0) {
        if (in_check) {
            return -TO_MATE_SCORE(0);
        } else {
            return 0;
        }
    }

    if (alpha > old_alpha) {
        tt->save(tt::EXACT, board.record[board.now].hash, depth, 0, alpha, best_move);
        if (!best_move.info.is_capture) history_heur.update(best_move, nullptr, 0, depth);
    } else {
        tt->save(tt::UPPER, board.record[board.now].hash, depth, 0, alpha, best_move);
    }

    return alpha;
}

template<bool H>
int search_t::search_ab(board_t &board, int alpha, int beta, int ply, int depth, bool can_null, move_t excluded,
                        const std::atomic_bool &aborted) {
    nodes++;
    if (!H) pv_table_len[ply] = ply;

    const bool PV = alpha + 1 < beta;

    if (is_aborted(aborted)) {
        return TIMEOUT;
    }

    // Quiescence search
    if (depth < 1) return search_qs<H>(board, alpha, beta, ply + 1, aborted);

    // Search variables
    int score;
    const int old_alpha = alpha;
    move_t best_move{};

    // Game state
    if (board.record[board.now].halfmove_clock >= 100
        || board.is_repetition_draw(ply, 2)
           || board.is_repetition_draw(100, 3)) return 0;

    // Mate distance pruning
    if (ply) {
        alpha = std::max(-TO_MATE_SCORE(ply), alpha);
        if(beta <= -TO_MATE_SCORE(ply)) return -TO_MATE_SCORE(ply);
        beta = std::min(TO_MATE_SCORE(ply), beta);
        if(alpha >= TO_MATE_SCORE(ply)) return TO_MATE_SCORE(ply);
    }

    bool in_check = board.is_incheck();

    // Probe transposition table
    tt::entry_t h = {0};
    int stand_pat;
    if (excluded == EMPTY_MOVE && tt->probe(board.record[board.now].hash, h)) {
        stand_pat = score = h.value(ply);

        if (!PV && h.depth >= depth) {
            if (h.bound == tt::LOWER && score >= beta) return score;
            if (h.bound == tt::UPPER && score <= alpha) return score;
            if (h.bound == tt::EXACT) return score;
        }
    } else {
        stand_pat = eval(board);
    }

    // Null move pruning
    int null_score = 0;
    if(can_null && !in_check && !PV && excluded == EMPTY_MOVE && stand_pat >= beta) {
        if((board.bb_side[board.record[board.now].next_move] ^ board.bb_pieces[board.record[board.now].next_move][PAWN])
                != 0) {
            board.move(EMPTY_MOVE);

            int R = 2 + depth / 4;
            null_score = -search_ab<H>(board, -beta, -beta + 1, ply + 1, depth - R - 1, false, EMPTY_MOVE, aborted);
            board.unmove();

            if(is_aborted(aborted)) return TIMEOUT;
            nulltries++;

            if(null_score >= beta) {
                nullcuts++;
                return beta;
            }
        }
    }

    // Internal iterative deepening
    if(depth > 6 && h.move == EMPTY_MOVE) {
        search_ab<H>(board, alpha, beta, ply, depth - 6, can_null, EMPTY_MOVE, aborted);
        if (is_aborted(aborted)) return TIMEOUT;
        tt->probe(board.record[board.now].hash, h);
    }

    GenStage stage = GEN_NONE;
    int move_score;
    movegen_t gen(board);
    gen.gen_normal();
    int n_legal = 0;
    while (gen.has_next()) {
        move_t move = gen.next(stage, move_score, *this, h.move, ply);
        if(excluded == move) {
            continue;
        }

        int ex = 0;

        // Singular extension
        if (depth >= 8 && move == h.move
                 && (h.bound == tt::LOWER || h.bound == tt::EXACT)
                 && excluded == EMPTY_MOVE
                 && h.depth >= depth - 3
                 && abs(h.value(ply) < MINCHECKMATE)) {
            int reduced_beta = (h.value(ply)) - depth;
            score = search_ab<H>(board, reduced_beta - 1, reduced_beta, ply + 1, depth / 2,
                                 can_null, move, aborted);
            if (is_aborted(aborted)) return TIMEOUT;

            if (score < reduced_beta) {
                ex = 1;
            }
        }

        board.move(move);
        if (board.is_illegal()) {
            board.unmove();
            continue;
        } else {
            n_legal++; // Legal move

            bool move_is_check = board.is_incheck();

            // Futility pruning
            if(!in_check && !move_is_check) {
                if(stand_pat + 64 < alpha && (stage == GEN_QUIETS || stage == GEN_BAD_CAPT) ) {
                    board.unmove();
                    break;
                }
            }

            // Check
            if (move_is_check) {
                ex = 1;
            }

            // LMR
            bool normal_search = true;
            if(depth > 3 && n_legal > 1 && !move_is_check && stage == GEN_QUIETS) {
                int R = !PV + depth / 8 + n_legal / 8 - 1;

                if(R > 0) {
                    score = -search_ab<H>(board, -alpha - 1, -alpha, ply + 1, depth - R - 1 + ex,
                            can_null, EMPTY_MOVE, aborted);
                    normal_search = score > alpha;
                }
            }

            if(normal_search) {
                if (PV && n_legal == 1) {
                    score = -search_ab<H>(board, -beta, -alpha, ply + 1, depth - 1 + ex,
                            can_null, EMPTY_MOVE, aborted);
                } else {
                    score = -search_ab<H>(board, -alpha - 1, -alpha, ply + 1, depth - 1 + ex,
                            can_null, EMPTY_MOVE, aborted);
                    if (alpha < score && score < beta) {
                        // Research if the zero window search failed.
                        score = -search_ab<H>(board, -beta, -alpha, ply + 1, depth - 1 + ex,
                                can_null, EMPTY_MOVE, aborted);
                    }
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
    } else if (excluded == EMPTY_MOVE) {
        tt->save(tt::UPPER, board.record[board.now].hash, depth, ply, alpha, best_move);
    }

    return alpha;
}

template<bool H>
int search_t::search_qs(board_t &board, int alpha, int beta, int ply, const std::atomic_bool &aborted) {
    nodes++;

    if (!H) pv_table_len[ply] = ply;
    const bool PV = alpha + 1 < beta;

    sel_depth = std::max(sel_depth, ply);

    if (is_aborted(aborted)) return TIMEOUT;

    int stand_pat = eval(board);

    if (stand_pat >= beta) return beta;
    if (alpha < stand_pat) alpha = stand_pat;

    GenStage stage;
    int move_score;
    movegen_t gen(board);
    gen.gen_caps();
    while (gen.has_next()) {
        move_t move = gen.next(stage, move_score, *this, EMPTY_MOVE, ply);

        if (move.info.captured_type == KING) return INF; // Ignore this position in case of a king capture
        if (stage == GEN_BAD_CAPT) break; // SEE Pruning
        if (stage + move_score - CAPT_BASE < alpha - 128) break; // Delta pruning

        board.move(move);
        if (board.is_illegal()) {
            board.unmove();
            continue;
        } else {
            int score = -search_qs<H>(board, -beta, -alpha, ply + 1, aborted);
            board.unmove();

            if (is_aborted(aborted)) return TIMEOUT;

            if (score >= beta) {
                return beta;
            }
            if (score > alpha) {
                alpha = score;

                if (!H && PV) {
                    pv_table[ply][ply] = move;
                    for (int i = ply + 1; i < pv_table_len[ply + 1]; i++) {
                        pv_table[ply][i] = pv_table[ply + 1][i];
                    }
                    pv_table_len[ply] = pv_table_len[ply + 1];
                }
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
    return last_pv[0] != EMPTY_MOVE && last_pv_len > 1;
}

bool search_t::keep_searching(int depth) {
    return nodes <= limits.node_limit
           && depth <= limits.depth_limit
           && CHRONO_DIFF(start, engine_clock::now()) <= limits.time_limit * 0.9;
}

bool search_t::is_aborted(const std::atomic_bool &aborted) {
    return aborted && has_pv();
}

bool search_t::is_root_move(move_t move) {
    return limits.root_moves.empty() ||
           (std::find(limits.root_moves.begin(), limits.root_moves.end(), move) != limits.root_moves.end());
}

void search_t::print_stats(int score, int depth, tt::Bound bound) {
    auto current = engine_clock::now();
    auto time = CHRONO_DIFF(start, current);
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
    if (time > 2000) {
        std::cout << " hashfull " << tt->hash_full()
                  << " beta " << ((double) fhf / fh)
                  << " null " << ((double) nullcuts / nulltries);
    }
    std::cout << " pv ";
    for (int i = 0; i < last_pv_len; i++) {
        std::cout << from_sq(last_pv[i].info.from) << from_sq(last_pv[i].info.to) << " ";
    }

    std::cout << std::endl;
}
