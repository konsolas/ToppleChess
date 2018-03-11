//
// Created by Vincent on 23/09/2017.
//

#ifndef TOPPLE_MOVEGEN_H
#define TOPPLE_MOVEGEN_H

#include "move.h"
#include "board.h"

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
    int gen_normal();

    /**
     * If the side to move is in check, generate evasions
     *
     * Moves generated by this method are not guaranteed to be fully legal, but are more likely to be legal than those
     * returned by gen_normal.
     *
     * @return the number of moves in {@code buf}
     */
    int gen_evasions();

    /**
     * Generate captures only, ordered MVV/LVA
     *
     * @return the number of moves in {@code buf}
     */
    int gen_caps();

    /**
     * Generate non-captures only, unordered.
     *
     * @return the number of moves in {@code buf}
     */
    int gen_quiets();

    void add_move(move_t move);

    move_t buf[256];
private:
    board_t &board;

    int idx;

    Team team;
    Team x_team;
    game_record_t record;

    void gen_castling();
    void gen_ep();
};


#endif //TOPPLE_MOVEGEN_H
