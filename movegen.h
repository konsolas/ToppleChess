//
// Created by Vincent on 23/09/2017.
//

#ifndef TOPPLE_MOVEGEN_H
#define TOPPLE_MOVEGEN_H

#include "move.h"
#include "board.h"
#include "search.h"

class board_t;

class movegen_t {
public:
    /**
     * Create a move generator which generates moves for the given board instance.
     *
     * @param board board to generate moves for
     */
    explicit movegen_t(board_t &board);

    /**
     * Generate moves in the current state of the board and save them to {@code buf}, overwriting it from index 0.
     *
     * @return the number of moves in {@code buf}
     */
    int gen_normal(move_t *buf);

    /**
     * Generate captures only, ordered MVV/LVA
     *
     * @return the number of moves in {@code buf}
     */
    int gen_caps(move_t *buf);

    /**
     * Generate non-captures only, unordered.
     *
     * @return the number of moves in {@code buf}
     */
    int gen_quiets(move_t *buf);
private:
    board_t &board;

    Team team;
    Team x_team;
    game_record_t record;

    int gen_castling(move_t *buf);
    int gen_ep(move_t *buf);

    template <Piece TYPE> void gen_piece_quiets(move_t *buf, int &buf_size, move_t move, U64 mask);
    template <Piece TYPE> void gen_piece_caps(move_t *buf, int &buf_size, move_t move);
};


#endif //TOPPLE_MOVEGEN_H
