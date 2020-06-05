//
// Created by Vincent Tang on 2019-05-12.
//

#include "pvs.h"

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
                               const std::function<void(int)> &output_info,
                               const std::function<void(int, move_t)> &output_currmove,
                               int alpha, int beta, int depth, const std::atomic_bool &aborted) {
        nodes++;
        pv_table_len[0] = 0;

        // Search variables
        int score, best_score = -INF;
        const int old_alpha = alpha;
        move_t best_move{};

        bool in_check = board->is_incheck();

        // Probe transposition table
        tt::entry_t h = {};
        tt::Bound h_bound = tt::NONE;
        move_t tt_move = EMPTY_MOVE;
        if (tt->probe(board->record.back().hash, h)) {
            stack[0].eval = h.info.static_eval;
            h_bound = h.bound();
            tt_move = board->to_move(h.info.move);
        } else {
            stack[0].eval = evaluator->evaluate(*board);
        }

        std::vector<pv_move_t> move_list;
        move_list.reserve(64);
        GenStage stage = GEN_NONE;
        int move_score;

        // Generate, sort, and determine search parameters
        int n_legal = 0;
        movesort_t gen(NORMAL, heur, *board, tt_move, EMPTY_MOVE, 0);
        for (move_t move = gen.next(stage, move_score, false);
             move != EMPTY_MOVE; move = gen.next(stage, move_score, false)) {
            if (std::find(root_moves.begin(), root_moves.end(), move) != root_moves.end() && board->is_legal(move)) {
                n_legal++;

                bool move_is_check = board->gives_check(move);
                int ex = move_is_check;

                // PV extension
                if (n_legal == 1) ex = 1;
                move_list.emplace_back(move, n_legal, depth - 1 + ex, depth - 1 + ex);
            }
        }
        // Keep searching until we prove that all other moves are bad.
        while (!move_list.empty()) {
            // Search the first move in the move list as the PV move, and then prove the rest with a zero window search.
            board->move(move_list[0].move);

            if (output_currmove) {
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
                        tt->save(tt::LOWER, board->record.back().hash, depth, 0, stack[0].eval, score, best_move);

                        if (!move_list[0].move.info.is_capture) {
                            heur.history.update(move_list[0].move, depth * depth);
                            heur.killers.update(move_list[0].move, 0);
                        }

                        return beta; // Fail hard
                    }

                    if (output_info) {
                        output_info(score);
                    }
                }
            }

            // Search remaining moves in parallel
            bool failed_high = false;
            for (auto it = move_list.cbegin() + 1; it < move_list.cend(); it++) {
                if (output_currmove) {
                    output_currmove(it->move_number, it->move);
                }

                board->move(it->move);

                bool full_search = true;
                if (it->reduced_depth < it->depth) {
                    score = -search_zw(-alpha, 0 + 1, it->reduced_depth, aborted);
                    full_search = score > alpha;
                }

                if (aborted) {
                    return TIMEOUT;
                }

                if (full_search) {
                    score = -search_zw(-alpha, 0 + 1, it->depth, aborted);
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
            tt->save(tt::EXACT, board->record.back().hash, depth, 0, stack[0].eval, alpha, best_move);
            if (!best_move.info.is_capture) heur.history.update(best_move, depth * depth);
        } else {
            tt->save(tt::UPPER, board->record.back().hash, depth, 0, stack[0].eval, alpha, best_move);
        }

        return alpha;
    }

    int context_t::search_pv(int alpha, int beta, const int ply, const int depth, const std::atomic_bool &aborted) {
        pv_table_len[ply] = ply;

        if (aborted) {
            return TIMEOUT;
        } else if (ply > MAX_PLY) {
            return evaluator->evaluate(*board);
        }

        // Quiescence search
        if (depth < 1) return search_qs<true>(alpha, beta, ply, aborted);

        // Count node if we didn't go into quiescence search
        nodes++;

        // Search variables
        int score, best_score = -INF;
        const int old_alpha = alpha;
        move_t best_move{};

        // Game state
        if (board->record.back().halfmove_clock >= 100
            || board->is_repetition_draw(ply)
            || board->is_material_draw()) {
            return 0;
        }

        // Probe transposition table
        tt::entry_t h = {};
        tt::Bound h_bound = tt::NONE;
        move_t tt_move = EMPTY_MOVE;
        if (tt->probe(board->record.back().hash, h)) {
            stack[ply].eval = h.info.static_eval;
            h_bound = h.bound();
            tt_move = board->to_move(h.info.move);
        } else {
            stack[ply].eval = evaluator->evaluate(*board);
        }

        bool in_check = board->is_incheck();
        bool improving = !in_check && (ply <= 1 || stack[ply].eval > stack[ply - 2].eval);

        // Probe endgame tablebases
        if (ply && board->record.back().halfmove_clock == 0 && pop_count(board->bb_all) <= use_tb) {
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
                    tt->save(bound, board->record.back().hash, MAX_PLY, ply, stack[ply].eval, value, EMPTY_MOVE);
                    return value;
                }

                if (bound == tt::LOWER) alpha = std::max(alpha, value);
                if (bound == tt::UPPER) beta = std::min(beta, value);
            }
        }

        // Internal iterative deepening
        if (depth > 6 && tt_move == EMPTY_MOVE) {
            search_pv(alpha, beta, ply, depth - 6, aborted);
            if (aborted) return TIMEOUT;
            if (tt->probe(board->record.back().hash, h)) {
                stack[ply].eval = h.info.static_eval;
                h_bound = h.bound();
                tt_move = board->to_move(h.info.move);
            }
        }

        std::vector<pv_move_t> move_list;
        move_list.reserve(64);
        GenStage stage = GEN_NONE;
        int move_score;

        // Generate, sort, and determine search parameters
        int n_legal = 0;
        movesort_t gen(NORMAL, heur, *board, tt_move, EMPTY_MOVE, ply);
        for (move_t move = gen.next(stage, move_score, false);
             move != EMPTY_MOVE; move = gen.next(stage, move_score, false)) {
            if (board->is_legal(move)) {
                n_legal++;

                bool move_is_check = board->gives_check(move);
                int ex = move_is_check;

                // Singular extension
                if (depth >= 8 && move == tt_move
                    && (h_bound == tt::LOWER || h_bound == tt::EXACT)
                    && h.depth() >= depth - 2) {
                    int reduced_beta = (h.value(ply)) - depth;
                    score = search_zw(reduced_beta, ply, depth / 2, aborted, move);
                    if (aborted) return TIMEOUT;

                    if (score < reduced_beta) {
                        ex = 1;
                    }
                }

                int R = 0;
                // Late move reductions
                if (depth >= 3 && n_legal > 1) {
                    // LMR
                    R = depth / 8 + n_legal / 8 - improving;
                    if (stage == GEN_QUIETS && move_score < 0) R++;
                    if (R >= 1 && board->see(reverse(move)) < 0) R -= 2;
                }

                move_list.emplace_back(move, n_legal, depth - R - 1 + ex, depth - 1 + ex);
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
                        tt->save(tt::LOWER, board->record.back().hash, depth, ply, stack[ply].eval, score, best_move);

                        if (!move_list[0].move.info.is_capture) {
                            heur.history.update(move_list[0].move, depth * depth);
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
                    score = -search_zw(-alpha, ply + 1, it->reduced_depth, aborted);
                    full_search = score > alpha;
                }

                if (aborted) {
                    return TIMEOUT;
                }

                if (full_search) {
                    score = -search_zw(-alpha, ply + 1, it->depth, aborted);
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
            tt->save(tt::EXACT, board->record.back().hash, depth, ply, stack[ply].eval, alpha, best_move);
            if (!best_move.info.is_capture) heur.history.update(best_move, depth * depth);
        } else {
            tt->save(tt::UPPER, board->record.back().hash, depth, ply, stack[ply].eval, alpha, best_move);
        }

        return alpha;
    }

    template<bool PV>
    int context_t::search_qs(int alpha, int beta, const int ply, const std::atomic_bool &aborted) {
        nodes++;

        if (PV) pv_table_len[ply] = ply;

        sel_depth = std::max(sel_depth, ply);

        if (aborted) {
            return TIMEOUT;
        } else if (ply > MAX_PLY) {
            return evaluator->evaluate(*board);
        }

        if (board->is_material_draw())
            return 0;

        // Probe transposition table
        tt::entry_t h = {};
        if (tt->probe(board->record.back().hash, h)) {
            int score = h.value(ply);
            stack[ply].eval = h.info.static_eval;
            tt::Bound h_bound = h.bound();

            if (h_bound == tt::LOWER && score >= beta) return score;
            if (h_bound == tt::UPPER && score <= alpha) return score;
            if (h_bound == tt::EXACT) return score;
        } else {
            stack[ply].eval = evaluator->evaluate(*board);
        }

        // Probe endgame tablebases
        if (ply && board->record.back().halfmove_clock == 0 && pop_count(board->bb_all) <= use_tb) {
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
                    tt->save(bound, board->record.back().hash, MAX_PLY - 1, ply, stack[ply].eval, value,
                             EMPTY_MOVE);
                    return value;
                }
            }
        }

        if (stack[ply].eval >= beta) return beta;
        if (alpha < stack[ply].eval) alpha = stack[ply].eval;

        GenStage stage = GEN_NONE;
        move_t move{};
        int move_score;
        movesort_t gen(QUIESCENCE, heur, *board, EMPTY_MOVE, EMPTY_MOVE, 0);
        while ((move = gen.next(stage, move_score, true)) != EMPTY_MOVE) {
            if (stack[ply].eval + move_score < alpha - 128) break; // Delta pruning

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

    int context_t::search_zw(int beta, const int ply, const int depth, const std::atomic_bool &aborted, move_t excluded) {
        if (aborted) {
            return TIMEOUT;
        } else if (ply > MAX_PLY) {
            return evaluator->evaluate(*board);
        }

        // Quiescence search
        if (depth < 1) return search_qs<false>(beta - 1, beta, ply, aborted);

        // Count node if we didn't go into quiescence search
        nodes++;

        // Search variables
        int score, best_score = -INF;
        move_t best_move = EMPTY_MOVE;

        // Game state
        if (board->record.back().halfmove_clock >= 100
            || board->is_repetition_draw(ply)
            || board->is_material_draw()) {
            return 0;
        }

        // Mate distance pruning
        beta = std::min(TO_MATE_SCORE(ply + 1), beta);
        if (std::max(-TO_MATE_SCORE(ply), beta - 1) >= beta) return beta;

        // Probe transposition table
        tt::entry_t h = {0};
        tt::Bound h_bound = tt::NONE;
        move_t tt_move = EMPTY_MOVE;
        if (excluded == EMPTY_MOVE && tt->probe(board->record.back().hash, h)) {
            score = h.value(ply);
            stack[ply].eval = h.info.static_eval;
            h_bound = h.bound();

            if (h.depth() >= depth) {
                if (h_bound == tt::LOWER && score >= beta) return beta;
                if (h_bound == tt::UPPER && score <= beta - 1) return beta - 1;
                if (h_bound == tt::EXACT) return std::clamp(score, beta - 1, beta);
            }

            tt_move = board->to_move(h.info.move);
        } else {
            score = -INF;
            stack[ply].eval = evaluator->evaluate(*board);
        }

        // Probe endgame tablebases
        if (ply && board->record.back().halfmove_clock == 0 && pop_count(board->bb_all) <= use_tb) {
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
                    || (bound == tt::UPPER && value <= beta - 1)) {
                    tt->save(bound, board->record.back().hash, MAX_PLY - 1, ply, stack[ply].eval, value,
                             EMPTY_MOVE);
                    return value;
                }
            }
        }

        bool in_check = board->is_incheck();
        bool improving = !in_check && (ply <= 1 || stack[ply].eval > stack[ply - 2].eval);
        bool non_pawn_material = multiple_bits(board->non_pawn_material(board->record.back().next_move));

        // Capture refutation from the null move, if relevant
        move_t refutation = EMPTY_MOVE;

        // Child node pruning
        if (!in_check && excluded == EMPTY_MOVE) {
            // Razoring
            if (depth <= 1 && stack[ply].eval + 350 < beta) {
                return search_qs<false>(beta - 1, beta, ply, aborted);
            }

            // Static null move pruning
            if (depth <= 6 && stack[ply].eval - 85 * depth > beta) {
                return stack[ply].eval;
            }

            // Null move pruning
            int null_score = 0;
            if (board->record.back().prev_move != EMPTY_MOVE && stack[ply].eval >= beta && non_pawn_material) {
                board->move(EMPTY_MOVE);

                int R = 2 + depth / 4;
                null_score = -search_zw(1 - beta, ply + 1, depth - R - 1, aborted);

                tt::entry_t null_entry = {};
                if (tt->probe(board->record.back().hash, null_entry)) {
                    refutation = board->to_move(null_entry.info.move);
                }

                board->unmove();

                if (aborted) return TIMEOUT;

                if (null_score >= beta) {
                    return beta;
                }
            }
        }

        bool futility_pruning = depth <= 6 && stack[ply].eval + 95 * depth < beta;
        bool skip_quiets = false;

        GenStage stage = GEN_NONE;
        move_t move{};
        int move_score;
        movesort_t gen(NORMAL, heur, *board, tt_move, refutation, ply);
        int searched = 0;
        while ((move = gen.next(stage, move_score, skip_quiets)) != EMPTY_MOVE) {
            if (excluded == move || !board->is_legal(move)) {
                continue;
            }

            int ex = 0;

            bool move_is_check = board->gives_check(move);

            // Early pruning
            if (best_score > -MINCHECKMATE && non_pawn_material && !in_check && !move_is_check) {
                if (stage == GEN_QUIETS) {
                    // Futility pruning and history leaf pruning
                    if (futility_pruning || (depth <= 1 && move_score < 0)) {
                        skip_quiets = true;
                        continue;
                    }

                    // Late move pruning
                    if (improving) {
                        skip_quiets = searched > 4 + depth * depth;
                    } else {
                        skip_quiets = searched > 2 + depth * depth / 2;
                    }
                } else if (stage == GEN_BAD_NOISY && depth <= 4) continue;
            }

            // Singular extension
            if (depth >= 8 && move == tt_move
                && (h_bound == tt::LOWER || h_bound == tt::EXACT)
                && excluded == EMPTY_MOVE
                && h.depth() >= depth - 2) {
                int reduced_beta = (h.value(ply)) - depth;
                score = search_zw(reduced_beta, ply, depth / 2, aborted, move);
                if (aborted) return TIMEOUT;

                if (score < reduced_beta) {
                    ex = 1;
                }
            }

            board->move(move);
            searched++;

            // Prefetch
            tt->prefetch(board->record.back().hash);
            evaluator->prefetch(board->record.back().kp_hash);

            // Check and castling extensions
            if (move_is_check) {
                ex = 1;
            }

            bool normal_search = true;
            if (depth >= 3 && searched > 1) {
                // LMR
                int R = 1 + depth / 8 + searched / 8 - improving;
                if (stage == GEN_QUIETS && move_score < 0) R++;
                if (R >= 1 && board->see(reverse(move)) < 0) R -= 2;

                if (R > 0) {
                    score = -search_zw(1 - beta, ply + 1, depth - R - 1 + ex, aborted);
                    normal_search = score >= beta;
                }
            }

            if (normal_search) {
                score = -search_zw(1 - beta, ply + 1, depth - 1 + ex, aborted);
            }

            board->unmove();

            if (aborted) return TIMEOUT;

            if (score > best_score) {
                best_score = score;
                best_move = move;

                if (score >= beta) {
                    tt->prefetch(board->record.back().hash);

                    if (!move.info.is_capture) {
                        size_t n_prev_quiets;
                        move_t *prev_quiets = gen.generated_quiets(n_prev_quiets);
                        int bonus = depth * depth;
                        for (size_t i = 0; i < n_prev_quiets; i++) {
                            heur.history.update(prev_quiets[i], -bonus);
                        }

                        heur.history.update(move, bonus);
                        heur.killers.update(move, ply);
                    }

                    tt->save(tt::LOWER, board->record.back().hash, depth, ply, stack[ply].eval, score, best_move);

                    return beta; // Fail hard
                }
            }
        }

        if (searched == 0) {
            if (in_check) {
                return -TO_MATE_SCORE(ply);
            } else {
                return 0;
            }
        }

        if (excluded == EMPTY_MOVE) {
            tt->save(tt::UPPER, board->record.back().hash, depth, ply, stack[ply].eval, beta - 1, best_move);
        }

        return beta - 1;
    }
}