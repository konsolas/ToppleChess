//
// Created by Vincent Tang on 2019-05-12.
//

#include "pvs.h"
#include "movesort.h"

#include "syzygy/tbprobe.h"

namespace pvs {
    struct pv_move_t {
        move_t move;
        int move_number;
        int reduced_depth;
        int depth;

        pv_move_t(move_t move, int move_number, int reduced_depth, int depth)
                : move(move), move_number(move_number), reduced_depth(reduced_depth), depth(depth) {}
    };

    int context_t::search_root(std::vector<move_t> root_moves,
            const std::function<void(int)> &output_info, const std::function<void(int, move_t)> &output_currmove,
            int alpha, int beta, int depth, const std::atomic_bool &aborted) {
        nodes++;
        pv_table_len[0] = 0;

        // Search variables
        int score, best_score = -INF;
        const int old_alpha = alpha;
        move_t best_move{};

        bool in_check = board->is_incheck();

        // Probe transposition table
        tt::entry_t h = {0};
        tt::Bound h_bound = tt::NONE;
        int eval;
        move_t tt_move = EMPTY_MOVE;
        if (tt->probe(board->record[board->now].hash, h)) {
            eval = h.static_eval;
            h_bound = h.bound();
            tt_move = board->to_move(h.move);
        } else {
            eval = evaluator->evaluate(*board);
        }

        // Internal iterative deepening
        if (depth > 6 && tt_move == EMPTY_MOVE) {
            search_pv(alpha, beta, 0, depth - 6, aborted);
            if (aborted) return TIMEOUT;
            if (tt->probe(board->record[board->now].hash, h)) {
                eval = h.static_eval;
                h_bound = h.bound();
                tt_move = board->to_move(h.move);
            }
        }

        std::vector<pv_move_t> move_list;
        move_list.reserve(64);
        GenStage stage = GEN_NONE;
        int move_score;

        // Generate, sort, and determine search parameters
        int n_legal = 0;
        movesort_t gen(NORMAL, heur, *board, board->to_move(h.move), EMPTY_MOVE, 0);
        for (move_t move = gen.next(stage, move_score); move != EMPTY_MOVE; move = gen.next(stage, move_score)) {
            if (std::find(root_moves.begin(), root_moves.end(), move) != root_moves.end() && board->is_legal(move)) {
                n_legal++;

                bool move_is_check = board->gives_check(move);
                int ex = move_is_check;

                // Singular extension
                if (depth >= 8 && move == tt_move
                    && (h_bound == tt::LOWER || h_bound == tt::EXACT)
                    && h.depth() >= depth - 2) {
                    int reduced_beta = (h.value(0)) - depth;
                    score = search_zw(reduced_beta - 1, reduced_beta, 0, depth / 2,
                                      true, move, true, aborted);
                    if (aborted) return TIMEOUT;

                    if (score < reduced_beta) {
                        ex = 1;
                    }
                }

                int reduction = 0;
                // Late move reductions
                if (n_legal > 1 && !move_is_check && stage == GEN_QUIETS) {
                    if (depth >= 3) {
                        // LMR
                        reduction = depth / 8 + n_legal / 8;
                        if (reduction >= 1 && board->see(reverse(move)) < 0) reduction -= 2;
                    }
                }

                move_list.emplace_back(move, n_legal, depth - reduction - 1 + ex, depth - 1 + ex);
            }
        }
        // Keep searching until we prove that all other moves are bad.
        while (!move_list.empty()) {
            // Search the first move in the move list as the PV move, and then prove the rest with a zero window search.
            board->move(move_list[0].move);

            if(output_currmove) {
                output_currmove(move_list[0].move_number, move_list[0].move);
            }

            score = -search_pv(-beta, -alpha, 0 + 1, move_list[0].depth, aborted);
            board->unmove();

            if (aborted) return TIMEOUT;

            // Check if the best move changed our bounds
            if (score > best_score) {
                best_score = score;
                best_move = move_list[0].move;

                if (score > alpha) {
                    alpha = score;
                    update_pv(0, move_list[0].move);
                    if (score >= beta) {
                        tt->save(tt::LOWER, board->record[board->now].hash, depth, 0, eval, score,
                                 best_move);

                        if (!move_list[0].move.info.is_capture) {
                            heur.history.good_history(move_list[0].move, depth);
                            heur.killers.update(move_list[0].move, 0);
                        }

                        return beta; // Fail hard
                    }

                    if(output_info) {
                        output_info(score);
                    }
                }
            }

            // Search remaining moves in parallel
            bool failed_high = false;
            for (auto it = move_list.cbegin() + 1; it < move_list.cend(); it++) {
                board->move(it->move);

                if(output_currmove) {
                    output_currmove(it->move_number, it->move);
                }

                bool full_search = true;
                if (it->reduced_depth < it->depth) {
                    score = -search_zw(-alpha - 1, -alpha, 0 + 1, it->reduced_depth,
                                       true, EMPTY_MOVE, true, aborted);
                    full_search = score > alpha;
                }

                if (aborted) {
                    return TIMEOUT;
                }

                if (full_search) {
                    score = -search_zw(-alpha - 1, -alpha, 0 + 1, it->depth,
                                       true, EMPTY_MOVE, true, aborted);
                }
                board->unmove();
                if (aborted) return TIMEOUT;

                if (score > alpha) {
                    failed_high = true;
                    move_list.erase(move_list.cbegin(), it);
                    break;
                }
            }

            if (!failed_high) break;
        }

        // Check for checkmate and stalemate
        if (n_legal == 0) {
            if (in_check) {
                return -TO_MATE_SCORE(0);
            } else {
                return 0;
            }
        }

        if (alpha > old_alpha) {
            tt->save(tt::EXACT, board->record[board->now].hash, depth, 0, eval, alpha, best_move);
            if (!best_move.info.is_capture) heur.history.good_history(best_move, depth);
        } else {
            tt->save(tt::UPPER, board->record[board->now].hash, depth, 0, eval, alpha, best_move);
        }

        return alpha;
    }

    int context_t::search_pv(int alpha, int beta, int ply, int depth, const std::atomic_bool &aborted) {
        nodes++;
        pv_table_len[ply] = ply;

        if (aborted) {
            return TIMEOUT;
        } else if (ply >= MAX_PLY - 1) {
            return evaluator->evaluate(*board);
        }

        // Quiescence search
        if (depth < 1) return search_qs<true>(alpha, beta, ply, aborted);

        // Search variables
        int score, best_score = -INF;
        const int old_alpha = alpha;
        move_t best_move{};

        // Game state
        if (board->record[board->now].halfmove_clock >= 100
            || board->is_repetition_draw(ply)) {
            return 0;
        }

        bool in_check = board->is_incheck();

        // Probe transposition table
        tt::entry_t h = {0};
        tt::Bound h_bound = tt::NONE;
        int eval;
        move_t tt_move = EMPTY_MOVE;
        if (tt->probe(board->record[board->now].hash, h)) {
            eval = h.static_eval;
            h_bound = h.bound();
            tt_move = board->to_move(h.move);
        } else {
            eval = evaluator->evaluate(*board);
        }

        // Probe endgame tablebases
        if (ply && board->record[board->now].halfmove_clock == 0 && pop_count(board->bb_all) <= use_tb) {
            int success;
            int value = probe_wdl(*board, &success);
            if (success) {
                tb_hits++;
                tt::Bound bound;
                if (value < 0) {
                    bound = tt::UPPER;
                    value = -TO_MATE_SCORE(MAX_TB_PLY + ply + 1);
                } else if (value > 0) {
                    bound = tt::LOWER;
                    value = TO_MATE_SCORE(MAX_TB_PLY + ply + 1);
                } else {
                    bound = tt::EXACT;
                }

                if (bound == tt::EXACT
                    || (bound == tt::LOWER && value >= beta)
                    || (bound == tt::UPPER && value <= alpha)) {
                    tt->save(bound, board->record[board->now].hash, MAX_PLY - 1, ply, eval, value, EMPTY_MOVE);
                    return value;
                }
            }
        }

        // Internal iterative deepening
        if (depth > 6 && tt_move == EMPTY_MOVE) {
            search_pv(alpha, beta, ply, depth - 6, aborted);
            if (aborted) return TIMEOUT;
            if (tt->probe(board->record[board->now].hash, h)) {
                eval = h.static_eval;
                h_bound = h.bound();
                tt_move = board->to_move(h.move);
            }
        }

        std::vector<pv_move_t> move_list;
        move_list.reserve(64);
        GenStage stage = GEN_NONE;
        int move_score;

        // Generate, sort, and determine search parameters
        int n_legal = 0;
        movesort_t gen(NORMAL, heur, *board, board->to_move(h.move), EMPTY_MOVE, ply);
        for (move_t move = gen.next(stage, move_score); move != EMPTY_MOVE; move = gen.next(stage, move_score)) {
            if (board->is_legal(move)) {
                n_legal++;

                bool move_is_check = board->gives_check(move);
                int ex = move_is_check;

                // Singular extension
                if (depth >= 8 && move == tt_move
                    && (h_bound == tt::LOWER || h_bound == tt::EXACT)
                    && h.depth() >= depth - 2) {
                    int reduced_beta = (h.value(ply)) - depth;
                    score = search_zw(reduced_beta - 1, reduced_beta, ply, depth / 2,
                                      true, move, true, aborted);
                    if (aborted) return TIMEOUT;

                    if (score < reduced_beta) {
                        ex = 1;
                    }
                }

                int reduction = 0;
                // Late move reductions
                if (n_legal > 1 && !move_is_check && stage == GEN_QUIETS) {
                    if (depth >= 3) {
                        // LMR
                        reduction = depth / 8 + n_legal / 8;
                        if (reduction >= 1 && board->see(reverse(move)) < 0) reduction -= 2;
                    }
                }

                move_list.emplace_back(move, n_legal, depth - reduction - 1 + ex, depth - 1 + ex);
            }
        }
        // Keep searching until we prove that all other moves are bad.
        while (!move_list.empty()) {
            // Search the first move in the move list as the PV move, and then prove the rest with a zero window search.
            board->move(move_list[0].move);
            score = -search_pv(-beta, -alpha, ply + 1, move_list[0].depth, aborted);
            board->unmove();

            if (aborted) return TIMEOUT;

            // Check if the best move changed our bounds
            if (score > best_score) {
                best_score = score;
                best_move = move_list[0].move;
                if (score > alpha) {
                    alpha = score;
                    update_pv(ply, move_list[0].move);

                    if (score >= beta) {
                        tt->save(tt::LOWER, board->record[board->now].hash, depth, ply, eval, score,
                                 best_move);

                        if (!move_list[0].move.info.is_capture) {
                            heur.history.good_history(move_list[0].move, depth);
                            heur.killers.update(move_list[0].move, ply);
                        }

                        return beta; // Fail hard
                    }
                }
            }

            // Search remaining moves in parallel
            bool failed_high = false;
            for (auto it = move_list.cbegin() + 1; it < move_list.cend(); it++) {
                board->move(it->move);
                bool full_search = true;
                if (it->reduced_depth < it->depth) {
                    score = -search_zw(-alpha - 1, -alpha, ply + 1, it->reduced_depth,
                                       true, EMPTY_MOVE, true, aborted);
                    full_search = score > alpha;
                }

                if (aborted) {
                    return TIMEOUT;
                }

                if (full_search) {
                    score = -search_zw(-alpha - 1, -alpha, ply + 1, it->depth,
                                       true, EMPTY_MOVE, true, aborted);
                }
                board->unmove();
                if (aborted) return TIMEOUT;

                if (score > alpha) {
                    failed_high = true;
                    move_list.erase(move_list.cbegin(), it);
                    break;
                }
            }

            if (!failed_high) break;
        }

        // Check for checkmate and stalemate
        if (n_legal == 0) {
            if (in_check) {
                return -TO_MATE_SCORE(ply);
            } else {
                return 0;
            }
        }

        if (alpha > old_alpha) {
            tt->save(tt::EXACT, board->record[board->now].hash, depth, ply, eval, alpha, best_move);
            if (!best_move.info.is_capture) heur.history.good_history(best_move, depth);
        } else {
            tt->save(tt::UPPER, board->record[board->now].hash, depth, ply, eval, alpha, best_move);
        }

        return alpha;
    }

    template<bool PV>
    int context_t::search_qs(int alpha, int beta, int ply, const std::atomic_bool &aborted) {

        nodes++;

        if (PV) pv_table_len[ply] = ply;

        sel_depth = std::max(sel_depth, ply);

        if (aborted) {
            return TIMEOUT;
        } else if (ply >= MAX_PLY - 1) {
            return evaluator->evaluate(*board);
        }

        int stand_pat = evaluator->evaluate(*board);

        // Probe endgame tablebases
        if (ply && board->record[board->now].halfmove_clock == 0 && pop_count(board->bb_all) <= use_tb) {
            int success;
            int value = probe_wdl(*board, &success);
            if (success) {
                tb_hits++;
                tt::Bound bound;
                if (value < 0) {
                    bound = tt::UPPER;
                    value = -TO_MATE_SCORE(MAX_TB_PLY + ply + 1);
                } else if (value > 0) {
                    bound = tt::LOWER;
                    value = TO_MATE_SCORE(MAX_TB_PLY + ply + 1);
                } else {
                    bound = tt::EXACT;
                }

                if (bound == tt::EXACT
                    || (bound == tt::LOWER && value >= beta)
                    || (bound == tt::UPPER && value <= alpha)) {
                    tt->save(bound, board->record[board->now].hash, MAX_PLY - 1, ply, stand_pat, value, EMPTY_MOVE);
                    return value;
                }
            }
        }

        if (stand_pat >= beta) return beta;
        if (alpha < stand_pat) alpha = stand_pat;

        GenStage stage = GEN_NONE;
        move_t move{};
        int move_score;
        movesort_t gen(QUIESCENCE, heur, *board, EMPTY_MOVE, EMPTY_MOVE, 0);
        while ((move = gen.next(stage, move_score)) != EMPTY_MOVE) {
            if (stand_pat + move_score < alpha - 128) break; // Delta pruning

            board->move(move);
            if (board->is_illegal()) {
                board->unmove();
                continue;
            } else {
                int score = -search_qs<PV>(-beta, -alpha, ply + 1, aborted);
                board->unmove();

                if (aborted) return TIMEOUT;

                if (score >= beta) {
                    return beta;
                }
                if (score > alpha) {
                    alpha = score;

                    if (PV) {
                        update_pv(ply, move);
                    }
                }
            }
        }

        return alpha;
    }

    int context_t::search_zw(int alpha, int beta, int ply, int depth, bool can_null,
                             move_t excluded, bool cut, const std::atomic_bool &aborted) {
        nodes++;

        if (aborted) {
            return TIMEOUT;
        } else if (ply >= MAX_PLY - 1) {
            return evaluator->evaluate(*board);
        }

        // Quiescence search
        if (depth < 1) return search_qs<false>(alpha, beta, ply, aborted);

        // Search variables
        int score, best_score = -INF;
        const int old_alpha = alpha;
        move_t best_move{};

        // Game state
        if (board->record[board->now].halfmove_clock >= 100
            || board->is_repetition_draw(ply)) {
            return 0;
        }

        // Mate distance pruning
        if (ply) {
            alpha = std::max(-TO_MATE_SCORE(ply), alpha);
            beta = std::min(TO_MATE_SCORE(ply + 1), beta);
            if (alpha >= beta) return beta;
        }

        // Probe transposition table
        tt::entry_t h = {0};
        tt::Bound h_bound = tt::NONE;
        int eval;
        move_t tt_move = EMPTY_MOVE;
        if (excluded == EMPTY_MOVE && tt->probe(board->record[board->now].hash, h)) {
            score = h.value(ply);
            eval = h.static_eval;
            h_bound = h.bound();

            if (h.depth() >= depth) {
                if (h_bound == tt::LOWER && score >= beta) return score;
                if (h_bound == tt::UPPER && score <= alpha) return score;
                if (h_bound == tt::EXACT) return score;
            }

            tt_move = board->to_move(h.move);
        } else {
            score = -INF;
            eval = evaluator->evaluate(*board);
        }

        // Probe endgame tablebases
        if (ply && board->record[board->now].halfmove_clock == 0 && pop_count(board->bb_all) <= use_tb) {
            int success;
            int value = probe_wdl(*board, &success);
            if (success) {
                tb_hits++;
                tt::Bound bound;
                if (value < 0) {
                    bound = tt::UPPER;
                    value = -TO_MATE_SCORE(MAX_TB_PLY + ply + 1);
                } else if (value > 0) {
                    bound = tt::LOWER;
                    value = TO_MATE_SCORE(MAX_TB_PLY + ply + 1);
                } else {
                    bound = tt::EXACT;
                }

                if (bound == tt::EXACT
                    || (bound == tt::LOWER && value >= beta)
                    || (bound == tt::UPPER && value <= alpha)) {
                    tt->save(bound, board->record[board->now].hash, MAX_PLY - 1, ply, eval, value, EMPTY_MOVE);
                    return value;
                }
            }
        }

        bool in_check = board->is_incheck();

        // Razoring
        if (!in_check && depth <= 1 && eval + 500 < alpha) {
            return search_qs<false>(alpha, beta, ply, aborted);
        }

        // Null move pruning
        int null_score = 0;
        bool non_pawn_material = multiple_bits(board->non_pawn_material(
                board->record[board->now].next_move));
        move_t refutation = EMPTY_MOVE;
        if (can_null && !in_check && excluded == EMPTY_MOVE && eval >= beta && non_pawn_material) {
            board->move(EMPTY_MOVE);

            int R = 2 + depth / 6;
            null_score = -search_zw(-beta, -beta + 1, ply + 1, depth - R - 1, false, EMPTY_MOVE, !cut,
                                    aborted);

            tt::entry_t null_entry = {};
            if (tt->probe(board->record[board->now].hash, null_entry)) {
                refutation = board->to_move(null_entry.move);
            }

            board->unmove();

            if (aborted) return TIMEOUT;

            if (null_score >= beta) {
                return beta;
            }
        }

        // Internal iterative deepening
        if (depth > 7 && tt_move == EMPTY_MOVE) {
            search_zw(alpha, beta, ply, depth - 7, can_null, EMPTY_MOVE, cut, aborted);
            if (aborted) return TIMEOUT;
            if (tt->probe(board->record[board->now].hash, h)) {
                score = h.value(ply);
                eval = h.static_eval;
                h_bound = h.bound();
                tt_move = board->to_move(h.move);
            }
        }

        GenStage stage = GEN_NONE;
        move_t move{};
        int move_score;
        movesort_t gen(NORMAL, heur, *board, tt_move, refutation, ply);
        int n_legal = 0;
        while ((move = gen.next(stage, move_score)) != EMPTY_MOVE) {
            if (excluded == move || !board->is_legal(move)) {
                continue;
            }

            int ex = 0;

            // Early pruning
            if (best_score > -MINCHECKMATE && non_pawn_material && !in_check) {
                // NB: We can only break here to skip quiets because quiets are ordered last in the current system
                if (depth <= 6 && stage == GEN_QUIETS && eval + 95 * depth <= alpha) {
                    // Has the effect of skipping all quiets
                    break;
                }

                // History leaf pruning
                if (depth <= 2 && stage == GEN_QUIETS && n_legal > 8) {
                    if (move_score < 0) {
                        continue;
                    }
                }

                // SEE pruning
                int see_score = (stage == GEN_GOOD_CAPT || stage == GEN_BAD_CAPT) ? move_score : board->see(move);
                if (stage == GEN_BAD_CAPT && see_score < -16 * depth * depth) {
                    continue;
                } else if (stage == GEN_QUIETS && see_score < -100 * depth) {
                    continue;
                }
            }

            // Singular extension
            if (depth >= 8 && move == tt_move
                && (h_bound == tt::LOWER || h_bound == tt::EXACT)
                && excluded == EMPTY_MOVE
                && h.depth() >= depth - 2) {
                int reduced_beta = (h.value(ply)) - depth;
                score = search_zw(reduced_beta - 1, reduced_beta, ply, depth / 2,
                                  can_null, move, cut, aborted);
                if (aborted) return TIMEOUT;

                if (score < reduced_beta) {
                    ex = 1;
                }
            }

            board->move(move);
            if (board->is_illegal()) {
                board->unmove();
                continue;
            } else {
                n_legal++; // Legal move

                // Prefetch
                tt->prefetch(board->record[board->now].hash);
                evaluator->prefetch(board->record[board->now].pawn_hash);

                bool move_is_check = board->is_incheck();

                // Check and castling extensions
                if (move_is_check) {
                    ex = 1;
                }

                bool normal_search = true;
                if (n_legal > 1 && !move_is_check && stage == GEN_QUIETS) {
                    if (depth >= 3) {
                        // LMR
                        int R = 1 + depth / 8 + n_legal / 8;
                        if (cut) R += 2;
                        if (R >= 1 && board->see(reverse(move)) < 0) R -= 2;

                        if (R > 0) {
                            score = -search_zw(-alpha - 1, -alpha, ply + 1, depth - R - 1 + ex,
                                               can_null, EMPTY_MOVE, !cut, aborted);
                            normal_search = score > alpha;
                        }
                    }
                }

                if (normal_search) {
                    score = -search_zw(-alpha - 1, -alpha, ply + 1, depth - 1, can_null, EMPTY_MOVE, !cut, aborted);
                }

                board->unmove();

                if (aborted) return TIMEOUT;

                if (score > best_score) {
                    best_score = score;
                    best_move = move;

                    if (score > alpha) {
                        alpha = score;

                        if (score >= beta) {
                            tt->save(tt::LOWER, board->record[board->now].hash, depth, ply, eval, score,
                                     best_move);

                            if (!move.info.is_capture) {
                                int n_prev_quiets;
                                move_t *prev_quiets = gen.generated_quiets(n_prev_quiets);
                                for (int i = 0; i < n_prev_quiets; i++) {
                                    heur.history.bad_history(prev_quiets[i], depth);
                                }

                                heur.history.good_history(move, depth);
                                heur.killers.update(move, ply);
                            }

                            return beta; // Fail hard
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
            tt->save(tt::EXACT, board->record[board->now].hash, depth, ply, eval, alpha, best_move);
            if (!best_move.info.is_capture) heur.history.good_history(best_move, depth);
        } else if (excluded == EMPTY_MOVE) {
            tt->save(tt::UPPER, board->record[board->now].hash, depth, ply, eval, alpha, best_move);
        }

        return alpha;
    }
}