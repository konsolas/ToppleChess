//
// Created by Vincent on 30/09/2017.
//

#include <utility>
#include <vector>
#include <algorithm>

#include "search.h"
#include "eval.h"
#include "movesort.h"
#include "move.h"

const int TIMEOUT = -INF * 2;

search_t::search_t(board_t board, evaluator_t &evaluator, tt::hash_t *tt, unsigned int threads, search_limits_t limits)
        : evaluator(evaluator), tt(tt), threads(threads), limits(std::move(limits)) {
    main_context.board = board;
}

search_result_t search_t::think(const std::atomic_bool &aborted) {
    nodes = 0;
    start = engine_clock::now();

    int prev_score, score = 0, depth;

    if (threads > 0) {
        // Helper threads
        std::vector<std::future<int>> helper_threads(threads - 1);
        std::vector<context_t> helper_contexts(threads - 1);
        for (unsigned int i = 0; i < threads - 1; i++) {
            helper_contexts[i] = main_context;
            helper_contexts[i].tid = i + 1;
        }
        std::atomic_bool helper_thread_aborted;

        prev_score = 0;
        for (depth = 1; keep_searching(depth); depth++) {
            sel_depth = 0;
            helper_thread_aborted = false;

            // Start helper threads (lazy SMP)
            if (depth > 5) {
                for (unsigned int i = 0; i < threads - 1; i++) {
                    helper_threads[i] = std::async(std::launch::async,
                                                   &search_t::search_ab<true, true>, this,
                                                   std::ref(helper_contexts[i]), -INF, INF, 0, depth, true, EMPTY_MOVE,
                                                   std::ref(helper_thread_aborted));
                }
            }

            int n_legal = 0;
            score = search_aspiration(prev_score, depth, aborted, n_legal);

            // Stop helper threads
            helper_thread_aborted = true;
            for (unsigned int i = 0; i < threads - 1; i++) {
                helper_threads[i] = {};
            }

            // Check if search was aborted
            if (is_aborted(aborted)) {
                break;
            }

            // If in game situation, try and manage time
            if (limits.game_situation && timer_started && has_pv()) {
                auto elapsed = CHRONO_DIFF(timer_start, engine_clock::now());

                // If there is only one legal move, return immediately
                if (n_legal <= 1) {
                    break;
                }

                // Scale the search time based on the complexity of the position
                int junk;
                double tapering_factor = evaluator.eval_material(main_context.board, junk, junk);
                double complexity = std::clamp(n_legal / 30.0, 0.5, 3.0) * tapering_factor + (1 - tapering_factor);
                int adjusted_suggestion = static_cast<int>(complexity * limits.suggested_time_limit);

                // See if we can justify ending the search early, as long as we're not doing badly
                if (depth <= 5 || score > prev_score - 50) {
                    // If it's unlikely that we'll search deeper
                    if (elapsed > adjusted_suggestion * 0.75) {
                        break;
                    }
                }
            }

            prev_score = score;
        }
    } else {
        throw std::invalid_argument("threads cannot be 0");
    }

    wait_for_timer();

    return {last_pv[0], last_pv[1], score, depth, main_context.heur};
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

int search_t::search_aspiration(int prev_score, int depth, const std::atomic_bool &aborted, int &n_legal) {
    const int ASPIRATION_DELTA = 15;

    int alpha, beta, delta = ASPIRATION_DELTA;

    if (depth > 4) {
        alpha = std::max(-INF, prev_score - delta);
        beta = std::min(INF, prev_score + delta);
    } else {
        alpha = -INF, beta = INF;
    }

    int researches = 0;
    int score;

    // Check if we need to search again
    while (true) {
        score = search_root(main_context, alpha, beta, depth, aborted, n_legal);
        if (score == TIMEOUT) return score;

        researches++;
        if (score >= beta) {
            print_stats(score, depth, tt::LOWER);
            beta = std::min(INF, beta + delta);
        } else if (score <= alpha) {
            print_stats(score, depth, tt::UPPER);
            beta = (alpha + beta) / 2;
            alpha = std::max(-INF, alpha - delta);
        } else {
            break;
        }

        delta = delta + delta / 2;
    }

    save_pv();
    print_stats(score, depth, tt::EXACT);

    return score;
}

int search_t::search_root(context_t &context, int alpha, int beta, int depth, const std::atomic_bool &aborted,
                          int &n_legal) {
    nodes++;
    pv_table_len[0] = 0;

    // Check extension
    bool in_check = context.board.is_incheck();
    if (in_check) depth++;

    // Search variables
    int score, best_score = -INF;
    const int old_alpha = alpha;
    move_t best_move{};

    // Probe transposition table
    tt::entry_t h = {0};
    tt->probe(context.board.record[context.board.now].hash, h);

    int eval = evaluator.evaluate(context.board);

    GenStage stage = GEN_NONE;
    move_t move{};
    int move_score;
    movesort_t gen(NORMAL, context, context.board.to_move(h.move), 0);
    n_legal = 0;
    while ((move = gen.next(stage, move_score)) != EMPTY_MOVE) {
        if (!is_root_move(move)) {
            continue;
        }

        context.board.move(move);
        if (context.board.is_illegal()) {
            context.board.unmove();
            continue;
        } else {
            n_legal++; // Legal move

            // Display current move information
            if (CHRONO_DIFF(start, engine_clock::now()) > 1000) {
                std::cout << "info currmove " << move << " currmovenumber " << n_legal << std::endl;
            }

            if (n_legal == 1) {
                score = -search_ab<true, false>(context, -beta, -alpha, 1, depth - 1, true, EMPTY_MOVE, aborted);
            } else {
                score = -search_ab<false, false>(context, -alpha - 1, -alpha, 1, depth - 1, true, EMPTY_MOVE, aborted);
                if (alpha < score && score < beta) {
                    // Research if the zero window search failed.
                    score = -search_ab<true, false>(context, -beta, -alpha, 1, depth - 1, true, EMPTY_MOVE, aborted);
                }
            }
            context.board.unmove();

            if (is_aborted(aborted)) return TIMEOUT;

            if (score > best_score) {
                best_score = score;
                best_move = move;
                if (score > alpha) {
                    if (score >= beta) {
                        if (n_legal == 1) {
                            fhf++;
                        }
                        fh++;

                        tt->save(tt::LOWER, context.board.record[context.board.now].hash, depth, 0, eval, score,
                                 best_move);

                        if (!move.info.is_capture) {
                            context.heur.history.good_history(move, depth);
                            context.heur.killers.update(move, 0);
                        }

                        return beta; // Fail hard
                    }

                    alpha = score;
                    update_pv(0, move);

                    if (depth > 1 && CHRONO_DIFF(start, engine_clock::now()) > 1000) {
                        save_pv();
                        print_stats(score, depth, tt::LOWER);
                    }
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
        tt->save(tt::EXACT, context.board.record[context.board.now].hash, depth, 0, eval, alpha, best_move);
        if (!best_move.info.is_capture) context.heur.history.good_history(best_move, depth);
    } else {
        tt->save(tt::UPPER, context.board.record[context.board.now].hash, depth, 0, eval, alpha, best_move);
    }

    return alpha;
}

template<bool PV, bool H>
int search_t::search_ab(context_t &context, int alpha, int beta, int ply, int depth, bool can_null, move_t excluded,
                        const std::atomic_bool &aborted) {
    nodes++;
    if (!H) pv_table_len[ply] = ply;

    if (is_aborted(aborted)) {
        return TIMEOUT;
    } else if (ply >= MAX_PLY - 2) {
        return std::clamp(evaluator.evaluate(context.board), alpha, beta);
    }

    // Quiescence search
    if (depth < 1) return search_qs<PV, H>(context, alpha, beta, ply, aborted);

    // Search variables
    int score, best_score = -INF;
    const int old_alpha = alpha;
    move_t best_move{};

    // Game state
    if (context.board.record[context.board.now].halfmove_clock >= 100
        || context.board.is_repetition_draw(ply, 2)
        || context.board.is_repetition_draw(100, 3)) {
        return std::clamp(0, alpha, beta);
    }

    // Mate distance pruning
    if (ply && !PV) {
        alpha = std::max(-TO_MATE_SCORE(ply), alpha);
        beta = std::min(TO_MATE_SCORE(ply + 1), beta);
        if (alpha >= beta) return beta;
    }

    bool in_check = context.board.is_incheck();

    // Probe transposition table
    tt::entry_t h = {0};
    tt::Bound h_bound = tt::NONE;
    int eval;
    move_t tt_move = EMPTY_MOVE;
    if (excluded == EMPTY_MOVE && tt->probe(context.board.record[context.board.now].hash, h)) {
        score = h.value(ply);
        eval = h.static_eval;
        h_bound = h.bound();

        if (!PV && h.depth() >= depth) {
            if (h_bound == tt::LOWER && score >= beta) return score;
            if (h_bound == tt::UPPER && score <= alpha) return score;
            if (h_bound == tt::EXACT) return score;
        }

        tt_move = context.board.to_move(h.move);
    } else {
        score = -INF;
        eval = evaluator.evaluate(context.board);
    }

    // Null move pruning
    int null_score = 0;
    if (can_null && !in_check && !PV && excluded == EMPTY_MOVE && eval >= beta) {
        if (context.board.non_pawn_material(context.board.record[context.board.now].next_move) != 0) {
            context.board.move(EMPTY_MOVE);

            int R = 2 + depth / 4;
            null_score = -search_ab<false, H>(context, -beta, -beta + 1, ply + 1, depth - R - 1, false, EMPTY_MOVE,
                                              aborted);
            context.board.unmove();

            if (is_aborted(aborted)) return TIMEOUT;
            nulltries++;

            if (null_score >= beta) {
                nullcuts++;
                return beta;
            }
        }
    }

    // Internal iterative deepening
    if (depth > 6 && tt_move == EMPTY_MOVE) {
        search_ab<false, H>(context, alpha, beta, ply, depth - 6, can_null, EMPTY_MOVE, aborted);
        if (is_aborted(aborted)) return TIMEOUT;
        if (tt->probe(context.board.record[context.board.now].hash, h)) {
            score = h.value(ply);
            eval = h.static_eval;
            h_bound = h.bound();
            tt_move = context.board.to_move(h.move);
        }
    }

    GenStage stage = GEN_NONE;
    move_t move{};
    int move_score;
    movesort_t gen(NORMAL, context, tt_move, ply);
    int n_legal = 0;
    while ((move = gen.next(stage, move_score)) != EMPTY_MOVE) {
        if (excluded == move) {
            continue;
        }

        // SMP Desync threads for better scaling
        if (!ply && H && (n_legal + context.tid) % (1 + (context.tid % 4)) == 0) {
            continue;
        }

        int ex = 0;

        // Singular extension
        if (depth >= 8 && move == tt_move
            && (h_bound == tt::LOWER || h_bound == tt::EXACT)
            && excluded == EMPTY_MOVE
            && h.depth() >= depth - 3
            && abs(h.value(ply)) < MINCHECKMATE) {
            int reduced_beta = (h.value(ply)) - 2 * depth;
            score = search_ab<false, H>(context, reduced_beta - 1, reduced_beta, ply + 1, depth / 2,
                                        can_null, move, aborted);
            if (is_aborted(aborted)) return TIMEOUT;

            if (score < reduced_beta) {
                ex = 1;
            }
        }

        context.board.move(move);
        if (context.board.is_illegal()) {
            context.board.unmove();
            continue;
        } else {
            n_legal++; // Legal move

            // Prefetch
            tt->prefetch(context.board.record[context.board.now].hash);

            bool move_is_check = context.board.is_incheck();

            // Check
            if (move_is_check) {
                ex = 1;
            }

            // Futility pruning
            if (!in_check && ex == 0 && n_legal > 1) {
                if (depth <= 6 && eval + 64 + 32 * depth < alpha && stage == GEN_QUIETS) {
                    context.board.unmove();
                    break;
                }
            }

            bool normal_search = true;
            if (n_legal > 1 && !move_is_check && stage == GEN_QUIETS) {
                if (depth >= 3) {
                    // LMR
                    int R = !PV + depth / 8 + n_legal / 8;
                    if (move_score <= n_legal) R++;
                    if (tt_move.info.is_capture) R++;
                    if (R >= 1 && context.board.see(reverse(move)) < 0) R--;

                    if (R > 0) {
                        score = -search_ab<false, H>(context, -alpha - 1, -alpha, ply + 1, depth - R - 1 + ex,
                                                     can_null, EMPTY_MOVE, aborted);
                        normal_search = score > alpha;
                    }
                } else if (ply) {
                    // History pruning
                    if (n_legal > move_score) {
                        context.board.unmove();
                        break;
                    }
                }
            }

            if (normal_search) {
                if (PV && n_legal == 1) {
                    score = -search_ab<PV, H>(context, -beta, -alpha, ply + 1, depth - 1 + ex,
                                              can_null, EMPTY_MOVE, aborted);
                } else {
                    score = -search_ab<false, H>(context, -alpha - 1, -alpha, ply + 1, depth - 1 + ex,
                                                 can_null, EMPTY_MOVE, aborted);
                    if (alpha < score && score < beta) {
                        // Research if the zero window search failed.
                        score = -search_ab<true, H>(context, -beta, -alpha, ply + 1, depth - 1 + ex,
                                                    can_null, EMPTY_MOVE, aborted);
                    }
                }
            }

            context.board.unmove();

            if (is_aborted(aborted)) return TIMEOUT;

            if (score > best_score) {
                best_score = score;
                best_move = move;
                if (score > alpha) {
                    alpha = score;

                    if (score >= beta) {
                        if (n_legal == 1) {
                            fhf++;
                        }
                        fh++;

                        tt->save(tt::LOWER, context.board.record[context.board.now].hash, depth, ply, eval, score,
                                 best_move);

                        if (!move.info.is_capture) {
                            int n_prev_quiets;
                            move_t *prev_quiets = gen.generated_quiets(n_prev_quiets);
                            for (int i = 0; i < n_prev_quiets; i++) {
                                context.heur.history.bad_history(prev_quiets[i], depth);
                            }

                            context.heur.history.good_history(move, depth);
                            context.heur.killers.update(move, ply);
                        }

                        return beta; // Fail hard
                    }

                    if (!H && PV) {
                        update_pv(ply, move);
                    }
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
        tt->save(tt::EXACT, context.board.record[context.board.now].hash, depth, ply, eval, alpha, best_move);
        if (!best_move.info.is_capture) context.heur.history.good_history(best_move, depth);
    } else if (excluded == EMPTY_MOVE) {
        tt->save(tt::UPPER, context.board.record[context.board.now].hash, depth, ply, eval, alpha, best_move);
    }

    return alpha;
}

template<bool PV, bool H>
int search_t::search_qs(context_t &context, int alpha, int beta, int ply, const std::atomic_bool &aborted) {
    nodes++;

    if (!H) pv_table_len[ply] = ply;

    sel_depth = std::max(sel_depth, ply);

    if (is_aborted(aborted)) {
        return TIMEOUT;
    } else if (ply >= MAX_PLY - 2) {
        return std::clamp(evaluator.evaluate(context.board), alpha, beta);
    }

    int stand_pat = evaluator.evaluate(context.board);

    if (stand_pat >= beta) return beta;
    if (alpha < stand_pat) alpha = stand_pat;

    GenStage stage = GEN_NONE;
    move_t move{};
    int move_score;
    movesort_t gen(QUIESCENCE, context, EMPTY_MOVE, 0);
    while ((move = gen.next(stage, move_score)) != EMPTY_MOVE) {
        if (move.info.captured_type == KING) return INF; // Ignore this position in case of a king capture
        if (stand_pat + move_score < alpha - 128) break; // Delta pruning

        context.board.move(move);
        if (context.board.is_illegal()) {
            context.board.unmove();
            continue;
        } else {
            int score = -search_qs<PV, H>(context, -beta, -alpha, ply + 1, aborted);
            context.board.unmove();

            if (is_aborted(aborted)) return TIMEOUT;

            if (score >= beta) {
                return beta;
            }
            if (score > alpha) {
                alpha = score;

                if (!H && PV) {
                    update_pv(ply, move);
                }
            }
        }
    }

    return alpha;
}


void search_t::update_pv(int ply, move_t move) {
    pv_table[ply][ply] = move;
    for (int i = ply + 1; i < pv_table_len[ply + 1]; i++) {
        pv_table[ply][i] = pv_table[ply + 1][i];
    }
    pv_table_len[ply] = pv_table_len[ply + 1];
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
    return !has_pv()
           || (nodes <= limits.node_limit
               && depth <= limits.depth_limit
               && (!timer_started || CHRONO_DIFF(timer_start, engine_clock::now()) <= limits.hard_time_limit));
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
        std::cout << last_pv[i] << " ";
    }

    std::cout << std::endl;
}
