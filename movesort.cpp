//
// Created by Vincent on 30/06/2018.
//

#include "movesort.h"

movesort_t::movesort_t(GenMode mode, search_context_t &context, move_t hash_move, int ply) :
        mode(mode), context(context), hash_move(hash_move), ply(ply), gen(movegen_t(context.board)) {}

move_t movesort_t::next(GenStage &stage, int &score) {
    retry:
    switch (stage) {
        case GEN_NONE:
            stage = GEN_HASH;
            if (context.board.is_pseudo_legal(hash_move)) {
                score = INF;
                return hash_move;
            }
        case GEN_HASH:
            // Generate captures
            capt_buf_size = gen.gen_caps(capt_buf);

            // Score captures (SEE)
            for (int i = 0; i < capt_buf_size; i++) {
                int see_score = context.board.see(capt_buf[i]);
                capt_scores[i] = see_score >= 0 ? (see_score +
                                  (context.board.record[context.board.now].next_move ?
                                   rank_index(capt_buf[i].info.to) + 1 : 8 - rank_index(capt_buf[i].info.to)))
                                 : see_score;
            }

            stage = GEN_GOOD_CAPT;
        case GEN_GOOD_CAPT:
            // Pick best capture if there are any left.
            if (capt_idx < capt_buf_size) {
                int best_idx = capt_idx;
                for (int i = capt_idx; i < capt_buf_size; i++) {
                    if (capt_scores[i] > capt_scores[best_idx]) {
                        best_idx = i;
                    }
                }

                buf_swap_capt(best_idx, capt_idx);

                if (capt_buf[capt_idx] == hash_move) {
                    capt_idx++;
                    goto retry;
                }

                if (capt_scores[capt_idx] >= 0) {
                    score = capt_scores[capt_idx];
                    return capt_buf[capt_idx++];
                }
            }

            // Out of captures, move on to quiets.
            if (mode == QUIESCENCE) return EMPTY_MOVE;

            // Generate quiets in main buffer
            main_buf_size = gen.gen_quiets(main_buf);

            // Score quiets
            for (int i = 0; i < main_buf_size; i++) {
                if (context.h_killer.primary(ply) == main_buf[i]) {
                    main_scores[i] = KILLER_BASE + 3;
                } else if (context.h_killer.secondary(ply) == main_buf[i]) {
                    main_scores[i] = KILLER_BASE + 2;
                } else if (ply >= 2 && (context.h_killer.primary(ply - 2) == main_buf[i])) {
                    main_scores[i] = KILLER_BASE + 1;
                } else if (ply >= 2 && (context.h_killer.secondary(ply - 2) == main_buf[i])) {
                    main_scores[i] = KILLER_BASE;
                } else {
                    main_scores[i] = context.h_history.get(main_buf[i]);
                }
            }

            stage = GEN_KILLER;
        case GEN_KILLER:
        case GEN_QUIETS:
            // Pick best quiet until there are none left
            if (main_idx < main_buf_size) {
                int best_main_idx = main_idx;
                for (int i = main_idx; i < main_buf_size; i++) {
                    if (main_scores[i] > main_scores[best_main_idx]) {
                        best_main_idx = i;
                    }
                }
                buf_swap_main(best_main_idx, main_idx);

                if (main_buf[main_idx] == hash_move) {
                    main_idx++;
                    goto retry;
                }

                if(main_scores[main_idx] >= KILLER_BASE) stage = GEN_KILLER;
                else stage = GEN_QUIETS;
                score = main_scores[main_idx];
                return main_buf[main_idx++];
            }

            stage = GEN_BAD_CAPT;
        case GEN_BAD_CAPT:
            // Pick best capture if there are any left.
            if (capt_idx < capt_buf_size) {
                int best_capt_idx = capt_idx;
                for (int i = capt_idx; i < capt_buf_size; i++) {
                    if (capt_scores[i] > capt_scores[best_capt_idx]) {
                        best_capt_idx = i;
                    }
                }
                buf_swap_capt(best_capt_idx, capt_idx);

                if (capt_buf[capt_idx] == hash_move) {
                    capt_idx++;
                    goto retry;
                }

                score = capt_scores[capt_idx];
                return capt_buf[capt_idx++];
            } else {
                return EMPTY_MOVE;
            }
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

    int score = capt_scores[a];
    capt_scores[a] = capt_scores[b];
    capt_scores[b] = score;
}
