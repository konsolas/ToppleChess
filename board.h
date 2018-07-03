//
// Created by Vincent on 22/09/2017.
//

#ifndef TOPPLE_BOARD_H
#define TOPPLE_BOARD_H

#include <string>
#include <iostream>

#include "move.h"
#include "types.h"
#include "bb.h"
#include "hash.h"

// Piece values: pnbrqk
// Used for SEE
constexpr int VAL[] = {100, 300, 300, 500, 900, INF};

/**
 * Represents a state in the game. It contains the move used to reach the state, and necessary variables within the state.
 */
struct game_record_t {
    move_t prev_move;

    Team next_move; // Who moves next?
    bool castle[2][2]; // [Team][0 for kingside, 1 for queenside]
    uint8_t ep_square; // Target square for en-passant after last double pawn move
    int halfmove_clock; // Moves since last pawn move or capture
    U64 hash;
};

/**
 * Represents the attacks on a certain square on the board. The team and piece fields are only meaningful if the
 * square is occupied - the occupied field is true.
 */
struct sq_data_t {
    bool occupied;
    Team team;
    Piece piece;
};

/**
 * Internal board representation used in the Topple engine.
 * Uses bitboard representation only
 */
struct board_t {
    board_t() = default;
    explicit board_t(std::string fen);

    void move(move_t move);
    void unmove();

    move_t parse_move(const std::string &str);

    bool is_illegal();
    bool is_incheck();

    bool is_attacked(uint8_t sq, Team side);
    U64 attacks_to(uint8_t sq, Team side); // Attacks to sq from side
    bool is_pseudo_legal(move_t move);

    bool is_repetition_draw(int ply, int reps);

    int see(move_t move);
    U64 non_pawn_material(Team side);

    void mirror();

    /* Board representation (Bitboard) */
    U64 bb_pieces[2][6] = {}; // [Team][Piece]
    U64 bb_side[2] = {}; // [Team]
    U64 bb_all = {}; // All occupied squares
    /* Board representation (Square array) */
    sq_data_t sq_data[64] = {{}};

    /* Game history */
    int now = 0; // Index for record array
    game_record_t record[4096] = {{}}; // Record (supporting games of up to 4096 moves)

    /* Internal methods */
    template<bool HASH>
    void switch_piece(Team side, Piece piece, uint8_t sq);
};

inline std::ostream &operator<<(std::ostream &stream, const board_t &board) {
    stream << std::endl;

    for (int i = 1; i <= board.now; i++) {
        if (i % 2 != 0) {
            stream << " " << ((i + 1) / 2) << ". ";
        }

        stream << board.record[i].prev_move << " ";
    }

    stream << std::endl;

    for (int i = 7; i >= 0; i--) {
        for (int j = 0; j < 8; j++) {
            auto file = uint8_t(j);
            auto rank = uint8_t(i);
            if (board.sq_data[square_index(file, rank)].occupied) {
                switch (board.sq_data[square_index(file, rank)].piece) {
                    case PAWN:
                        stream << (board.sq_data[square_index(file, rank)].team ? "p" : "P");
                        break;
                    case KNIGHT:
                        stream << (board.sq_data[square_index(file, rank)].team ? "n" : "N");
                        break;
                    case BISHOP:
                        stream << (board.sq_data[square_index(file, rank)].team ? "b" : "B");
                        break;
                    case ROOK:
                        stream << (board.sq_data[square_index(file, rank)].team ? "r" : "R");
                        break;
                    case QUEEN:
                        stream << (board.sq_data[square_index(file, rank)].team ? "q" : "Q");
                        break;
                    case KING:
                        stream << (board.sq_data[square_index(file, rank)].team ? "k" : "K");
                        break;
                }
            } else {
                stream << ".";
            }
        }

        stream << std::endl;
    }

    stream << std::endl;

    return stream;
}

#endif //TOPPLE_BOARD_H
