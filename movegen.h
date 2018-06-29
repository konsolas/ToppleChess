//
// Created by Vincent on 23/09/2017.
//

#ifndef TOPPLE_MOVEGEN_H
#define TOPPLE_MOVEGEN_H

#include "move.h"
#include "board.h"
#include "search.h"

constexpr int CAPT_BASE = 100000;
constexpr int KILLER_BASE = 80000;

class board_t;

enum GenStage {
    GEN_NONE = 0,
    GEN_HASH = 1,
    GEN_GOOD_CAPT = 2,
    GEN_KILLERS = 3,
    GEN_QUIETS = 4,
    GEN_BAD_CAPT = 5
};

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

    /**
     * Generate next move (selection sort)
     *
     * @param stage current generation stage
     * @param search search to generate for
     * @param hash_move move from hash table
     * @param ply current depth
     * @return next move
     */
    move_t next(GenStage &stage, int &score, search_t &search, move_t hash_move, int ply);

    /**
     * Generate next move without sorting
     * @return next move
     */
    move_t next();

    /**
     * Check if this generator has any moves remaining
     *
     * @return true if there are more moves (and next() should be called)
     */
    bool has_next();
private:
    board_t &board;

    void buf_swap(int i, int j);

    int buf_size = 0;
    int idx = 0;
    move_t buf[256] = {0};
    bool scored = false;
    int buf_scores[256] = {0};

    Team team;
    Team x_team;
    game_record_t record;

    void gen_castling();
    void gen_ep();

    template <Piece TYPE> void gen_piece_quiets(move_t move, U64 mask);
    template <Piece TYPE> void gen_piece_caps(move_t move);
};


#endif //TOPPLE_MOVEGEN_H
