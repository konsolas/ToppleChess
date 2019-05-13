//
// Created by Vincent Tang on 2019-05-12.
//

#ifndef TOPPLE_PVS_H
#define TOPPLE_PVS_H

#include <atomic>
#include <vector>

#include "board.h"
#include "move.h"
#include "eval.h"
#include "movesort.h"

namespace pvs {
    class context_t {
    public:
        // Constructor
        context_t(board_t *board, evaluator_t *evaluator, tt::hash_t *tt, int use_tb)
                : board(board), evaluator(evaluator), tt(tt), use_tb(use_tb) {
        }

        // Search
        int search_root(std::vector<move_t> root_moves,
                const std::function<void(int)> &output_info, const std::function<void(int, move_t)> &output_currmove,
                int alpha, int beta, int depth, const std::atomic_bool &aborted);

        int search_pv(int alpha, int beta, int ply, int depth, const std::atomic_bool &aborted);

        template<bool PV>
        int search_qs(int alpha, int beta, int ply, const std::atomic_bool &aborted);

        // Principal variation
        std::vector<move_t> get_current_pv() {
            return std::vector<move_t>(pv_table[0], pv_table[0] + pv_table_len[0]);
        }

        std::vector<move_t> get_saved_pv() {
            return saved_pv;
        }

        void save_pv() {
            saved_pv = get_current_pv();
        }

        board_t *get_board() {
            return board;
        }

        // Stats
        U64 get_nodes() const {
            return nodes;
        }

        int get_sel_depth() const {
            return sel_depth;
        }

        U64 get_tb_hits() const {
            return tb_hits;
        }
    private:
        int search_zw(int alpha, int beta, int ply, int depth, bool can_null, move_t excluded,
                      bool cut, const std::atomic_bool &aborted);

        void update_pv(int ply, move_t move) {
            pv_table[ply][ply] = move;
            for (int i = ply + 1; i < pv_table_len[ply + 1]; i++) {
                pv_table[ply][i] = pv_table[ply + 1][i];
            }
            pv_table_len[ply] = pv_table_len[ply + 1];
        }

        board_t *board; // Board representation
        evaluator_t *evaluator; // Pointer to shared evaluator
        tt::hash_t *tt; // Pointer to shared transposition table
        int use_tb; // Max pieces before probing tablebases

        // Principal variation table
        int pv_table_len[MAX_PLY] = {};
        move_t pv_table[MAX_PLY][MAX_PLY] = {{}};

        std::vector<move_t> saved_pv;

        // Search heuristics
        heuristic_set_t heur;

        // Statistics
        U64 nodes = 0;
        int sel_depth = 0;
        U64 tb_hits = 0;
    };
}

#endif //TOPPLE_PVS_H
