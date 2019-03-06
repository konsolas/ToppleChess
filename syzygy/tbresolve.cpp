//
// Created by Vincent on 02/03/2019.
//

#include "tbresolve.h"
#include "tbprobe.h"

#include "../movegen.h"
#include "../move.h"

static int wdl_to_dtz[5] = {
        -1, -101, 0, 101, 1
};

bool resolve_pv(board_t &pos, evaluator_t &evaluator, std::vector<move_t> &pv,
        int &score, size_t max_ply, const std::atomic_bool &aborted) {
    // Probe DTZ tables
    int prev_success = false, success = false;
    int dtz = probe_dtz(pos, &prev_success);

    // Play out the PV
    for(auto it = pv.begin(); it < pv.end(); it++) {
        int cnt50 = pos.record[pos.now].halfmove_clock;

        pos.move(*it);
        int new_dtz = -probe_dtz(pos, &success);

        if(success && prev_success && ((dtz > 0 && (new_dtz + cnt50 > 99 || new_dtz <= 0)) || (dtz == 0 && new_dtz != 0))) {
            pos.unmove();
            pv.erase(it, pv.end());

            std::cout << "dropped end of PV: " << (pv.end() - it) << std::endl;

            break;
        }

        prev_success = success;
        dtz = -new_dtz;
    }

    size_t pv_start_len = pv.size();

    // Build the extended PV
    while(success && (dtz != 0 || pv.size() < MAX_PLY) && pv.size() - pv_start_len < max_ply) {
        movegen_t gen(pos);
        move_t stack[128];
        move_t *moves, *end = stack + gen.gen_normal(stack);

        // Probe all moves to find the dtz-optimal choice
        move_t best_move{}; int best_score = INF;
        for(moves = stack; moves < end; moves++) {
            move_t move = *moves;
            if(!pos.is_legal(move)) continue;

            pos.move(move);
            int v = 0;
            if (pos.is_incheck() && dtz > 0) {
                move_t s[192];
                movegen_t checkmate_gen(pos);
                int n_generated = checkmate_gen.gen_normal(s);
                bool found_legal = false;
                for(int j = 0; j < n_generated; j++) {
                    if(pos.is_legal(s[j])){
                        found_legal = true;
                        break;
                    }
                }
                if (!found_legal) {
                    v = 1;
                }
            }
            if (!v) {
                if (pos.record[pos.now].halfmove_clock != 0) {
                    v = -probe_dtz(pos, &success);
                    if (v > 0) v++;
                    else if (v < 0) v--;
                } else {
                    v = -probe_wdl(pos, &success);
                    v = wdl_to_dtz[v + 2];
                }
            }
            pos.unmove();
            if(!success || aborted) break;

            if(v == 1) {
                best_move = move;
                break;
            }

            if(dtz > 0) {
                if(0 < v && v < best_score) {
                    best_score = v;
                    best_move = move;
                    if (dtz > 1 && v == dtz - 1) {
                        break;
                    }
                }
            } else if(dtz < 0) {
                if(best_move == EMPTY_MOVE) {
                    best_move = move;
                }
                if(v == dtz || v == dtz - 1) {
                    best_move = move;
                    break;
                }
            } else {
                if(v == 0) {
                    best_move = move;
                    break;
                }
            }
        }

        if(!success || aborted || best_move == EMPTY_MOVE) {
            break;
        }

        // Add best move to the PV
        pv.push_back(best_move);
        pos.move(best_move);

        dtz = probe_dtz(pos, &success);
    }

    int final_dtz = probe_dtz(pos, &success);
    int final_eval = evaluator.evaluate(pos);

    // Restore the board
    for(size_t i = 0; i < pv.size(); i++) {
        pos.unmove();
    }

    // Get an appropriate score
    if(success) {
        if (final_dtz == -1) {
            score = -TO_MATE_SCORE(static_cast<int>(pv.size()));
        } else if(final_dtz == 0) {
            score = 0;
        } else if (final_dtz > 0) {
            score = std::max(final_eval + 1000, 1000);
        } else {
            score = std::min(final_eval - 1000, -1000);
        }

        score = (pv.size() % 2 == 0) ? score : -score;

        return true;
    } else {
        return false;
    }
}
