//
// Created by Vincent on 30/06/2018.
//

#include "movesort.h"
#include "move.h"

movesort_t::movesort_t(GenMode mode,  const heuristic_set_t &heuristics, const board_t &board, move_t hash_move, move_t refutation, int ply) :
        mode(mode), heur(heuristics), board(board), hash_move(hash_move), refutation(refutation), gen(movegen_t(board)) {
    killer_1 = heur.killers.primary(ply);
    killer_2 = heur.killers.secondary(ply);
    killer_3 = ply > 2 ? heur.killers.primary(ply - 2) : EMPTY_MOVE;
}

// Pray that the compiler optimises tail recursion here - this is a very hot method
move_t movesort_t::next(GenStage &stage, int &score, bool skip_quiets) {
    switch (stage) {
        case GEN_NONE:
            stage = GEN_HASH;
            if (board.is_pseudo_legal(hash_move)) {
                score = INF;
                return hash_move;
            }
        case GEN_HASH:
            // Generate captures
            capt_buf_size = gen.gen_noisy(capt_buf);

            stage = GEN_GOOD_NOISY;
        case GEN_GOOD_NOISY:
            // Pick best capture if there are any left.
            if (capt_idx < capt_buf_size) {
                int best_idx = capt_idx;
                for (int i = capt_idx; i < capt_buf_size; i++) {
                    if (VAL[capt_buf[i].info.captured_type] > VAL[capt_buf[best_idx].info.captured_type]) {
                        best_idx = i;
                    }
                }

                buf_swap_capt(best_idx, capt_idx);

                if (capt_buf[capt_idx] == hash_move) {
                    capt_idx++;
                    return next(stage, score, skip_quiets);
                }

                if ((score = board.see(capt_buf[capt_idx])) >= 0) {
                    return capt_buf[capt_idx++];
                } else {
                    capt_buf[bad_capt_buf_size++] = capt_buf[capt_idx++];
                    return next(stage, score, skip_quiets);
                }
            }

            // Generate quiets if we're not in quiescence
            if(mode != QUIESCENCE) stage = GEN_QUIETS;
            else return EMPTY_MOVE;

            if(!skip_quiets) {
                // Generate quiets in main buffer
                main_buf_size = gen.gen_quiets(main_buf);

                // Score quiets
                for (int i = 0; i < main_buf_size; i++) {
                    if (main_buf[i] == hash_move) {
                        main_scores[i] = -1000000000;
                    } else if (main_buf[i] == killer_1) {
                        main_scores[i] = 1000000003;
                    } else if (main_buf[i] == killer_2) {
                        main_scores[i] = 1000000002;
                    } else if (main_buf[i] == killer_3) {
                        main_scores[i] = 1000000001;
                    } else {
                        main_scores[i] = heur.history.get(main_buf[i]);
                        if (refutation.info.is_capture && main_buf[i].info.from == refutation.info.to) {
                            main_scores[i] += 800;
                        }
                    }
                }
            }
        case GEN_QUIETS:
            // Pick best quiet until there are none left
            if (!skip_quiets && main_idx < main_buf_size) {
                int best_main_idx = main_idx;
                for (int i = main_idx; i < main_buf_size; i++) {
                    if (main_scores[i] > main_scores[best_main_idx]) {
                        best_main_idx = i;
                    }
                }
                buf_swap_main(best_main_idx, main_idx);

                if (main_buf[main_idx] == hash_move) {
                    main_idx++;
                    return next(stage, score, skip_quiets);
                }

                else stage = GEN_QUIETS;
                score = main_scores[main_idx];
                return main_buf[main_idx++];
            }
            stage = GEN_BAD_NOISY;
        case GEN_BAD_NOISY:
            // Bad captures are already sorted by MVV
            if (bad_capt_idx < bad_capt_buf_size) {
                if (capt_buf[bad_capt_idx] == hash_move) {
                    bad_capt_idx++;
                    return next(stage, score, skip_quiets);
                }

                score = 0;
                return capt_buf[bad_capt_idx++];
            }

            return EMPTY_MOVE;
        default:
            throw std::runtime_error("illegal move generation stage");
    }
}

void movesort_t::buf_swap_main(int a, int b) {
    move_t temp = main_buf[a];
    main_buf[a] = main_buf[b];
    main_buf[b] = temp;

    int score = main_scores[a];
    main_scores[a] = main_scores[b];
    main_scores[b] = score;
}

void movesort_t::buf_swap_capt(int a, int b) {
    move_t temp = capt_buf[a];
    capt_buf[a] = capt_buf[b];
    capt_buf[b] = temp;
}

move_t *movesort_t::generated_quiets(size_t &count) {
    count = main_idx;
    return main_buf;
}
