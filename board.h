//
// Created by Vincent on 22/09/2017.
//

#ifndef TOPPLE_BOARD_H
#define TOPPLE_BOARD_H

#include <string>
#include <iostream>
#include <vector>

#include "move.h"
#include "types.h"
#include "bb.h"
#include "hash.h"

/**
 * Represents a state in the game. It contains the move used to reach the state, and necessary variables within the state.
 */
struct game_record_t {
    move_t prev_move; // Last move before this position - can be EMPTY_MOVE
    Team next_move; // Who moves next?
    bool castle[2][2]; // castling rights for [team][kingside, queenside]
    uint8_t ep_square; // Target square for en-passant after last double pawn move
    int halfmove_clock; // Moves since last pawn move or capture
    U64 hash; // Zobrist hash of current position
    U64 kp_hash; // Zobrist hash of only king and pawns in current position
    material_data_t material; // Material counts in current position
};

/**
 * Represents the attacks on a certain square on the board. The team and piece fields are only meaningful if the
 * square is occupied - the occupied field is true. Packed into a byte.
 */
class sq_data_t {
    uint8_t occ : 1, t : 1, pt : 6;
public:
    [[nodiscard]] inline bool occupied() const { return occ; }
    [[nodiscard]] inline Team team() const { return Team(t); }
    [[nodiscard]] inline Piece piece() const { return Piece(pt); }
    inline void occupied(bool new_occ) { this->occ = new_occ; }
    inline void team(Team new_t) { this->t = new_t; }
    inline void piece(Piece new_pt) { this->pt = new_pt; }
};
static_assert(sizeof(sq_data_t) == 1);

/**
 * Topple Board Representation. Uses a square array, multiple bitboards, and a history vector.
 */
class alignas(64) board_t {
public:
    // Leaves the board in an empty, inconsistent state - used to construct a board to be assigned to later
    board_t() = default;

    /**
     * Construct a board from a string in Forsyth–Edwards Notation.
     * e.g. rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
     * is the starting position
     *
     * @param fen initial position
     */
    explicit board_t(const std::string &fen);

    /**
     * Make the given move on the board. The move must be pseudo-legal or the resulting
     * state of the board is undefined. Consider is_pseudo_legal for validation if the
     * move comes from an external source.
     *
     * @param move move to make
     */
    void move(move_t move);

    /**
     * Undo the previous move. Undefined behaviour if no previous move exists.
     */
    void unmove();

    /**
     * Parses a move given in long algebraic notation in the context of this position. The same string may refer to
     * different moves in different positions. This method does not check for legality or pseudo-legality.
     * Returns EMPTY_MOVE if the move is not well formed or is not applicable to this position.
     *
     * @param str move string in long algebraic notation (4-5 chars)
     * @return parsed move, or EMPTY_MOVE
     */
    [[nodiscard]] move_t parse_move(const std::string &str) const;

    /**
     * Converts a packed move (2 bytes, from|to|promotion) stored in the transposition table into a
     * full move (4 bytes) in the context of this position. The same packed move may refer to different moves in
     * different positions. This method does not check for legality or pseudo-legality.
     * Returns EMPTY_MOVE if the packed move is not applicable to this position.
     *
     * @param packed_move packed move
     * @return full move, or EMPTY_MOVE
     */
    [[nodiscard]] move_t to_move(packed_move_t packed_move) const;

    /**
     * Checks if the side to move is attacking the opponent's king (and therefore could theoretically capture it).
     * Does not check any other aspects of position legality.
     *
     * @return true if the position is illegal, false otherwise
     */
    [[nodiscard]] bool is_illegal() const;

    /**
     * Checks if the side to move's king is attacked by the opponent, that is, they are in check
     *
     * @return true if the side to move is in check, false otherwise
     */
    [[nodiscard]] bool is_incheck() const;

    /**
     * Checks if the given square is attacked by any pieces of the given team.
     * Equivalent to is_attacked(sq, side, all()).
     *
     * @param sq square to check
     * @param side team to check
     * @return true if any of side's pieces attack sq
     */
    [[nodiscard]] bool is_attacked(uint8_t sq, Team side) const;

    /**
     * Checks if the given square is attacked by any pieces of the given team,
     * given an occupancy bitboard for slider occlusion.
     *
     * @param sq square to check
     * @param side team to check
     * @return true if any of side's pieces attack sq
     */
    [[nodiscard]] bool is_attacked(uint8_t sq, Team side, U64 occupied) const;

    /**
     * Computes a bitboard representing the set of pieces on side that attack sq.
     *
     * @param sq square to check
     * @param side team to check
     * @return set of attackers of sq from side
     */
    [[nodiscard]] U64 attacks_to(uint8_t sq, Team side) const;

    /**
     * Checks if a move is pseudo-legal and therefore will leave the board in a
     * consistent state if passed to move(). Designed to validate moves returned by
     * parse_move or to_move
     *
     * @param move move to check
     * @return true if the move is pseudo-legal
     */
    [[nodiscard]] bool is_pseudo_legal(move_t move) const;

    /**
     * Checks if a pseudo-legal move is legal: that the move does not leave the side
     * to move's king in check. The return value of this method is unspecified if it is
     * called on a move that is not pseudo-legal
     *
     * @param move move to check
     * @return true if the move is legal, false otherwise
     */
    [[nodiscard]] bool is_legal(move_t move) const;

    /**
     * Checks if a pseudo-legal move gives check. Unspecified return value if the move is
     * not pseudo-legal
     *
     * @param move move to check
     * @return true if the move gives check, false otherwise
     */
    [[nodiscard]] bool gives_check(move_t move) const;

    /**
     * Checks if the current position is a draw by threefold repetition, given that the position
     * search_ply plies ago was not a draw by threefold repetition. This method returns true for
     * a twofold repetition if the repetition occurs within search_ply.
     *
     * @param search_ply search ply
     * @return true if the position is a threefold repetition or a twofold repetition within search_ply
     */
    [[nodiscard]] bool is_repetition_draw(int search_ply) const;

    /**
     * Naively checks if the position is a material draw - there exists no sequence of legal moves
     * that ends with checkmate.
     *
     * @return true if the position is a basic material draw
     */
    [[nodiscard]] bool is_material_draw() const;

    /**
     * Computes the static exchange evaluation of the given move - evaluates how good a capture
     * is likely to be without mutating the board. Takes into account batteries but does not
     * take into account pins.
     *
     * @param move move to evaluate
     * @return static exchange evaluation
     */
    [[nodiscard]] int see(move_t move) const;

    /**
     * Finds the set of pieces of side excluding pawns and kings. Equivalent to:
     * side(side) ^ pieces(side, PAWN) ^ pieces(side, KING)
     *
     * @param side side to check
     * @return set of non pawn material
     */
    [[nodiscard]] U64 non_pawn_material(Team side) const;

    /**
     * Mirrors the board in the horizontal axis, effectively flipping the colours of white and black pieces.
     */
    void mirror();

    // Accessors (all const)

    [[nodiscard]] inline U64 pieces(Team team, Piece piece) const { return bb_pieces[team][piece]; }
    [[nodiscard]] inline U64 side(Team team) const { return bb_side[team]; }
    [[nodiscard]] inline U64 all() const { return bb_all; }
    [[nodiscard]] inline sq_data_t sq(uint8_t sq) const { return sq_data[sq]; }
    [[nodiscard]] inline sq_data_t sq(uint8_t file, uint8_t rank) const { return sq_data[square_index(file, rank)]; }
    [[nodiscard]] inline std::vector<game_record_t> const& history() const { return record; }
    [[nodiscard]] inline game_record_t const& now() const { return record.back(); }
private:
    // Square array
    sq_data_t sq_data[64] = {{}};

    // Bitboards
    U64 bb_pieces[2][6] = {}; // [Team][Piece]
    U64 bb_side[2] = {}; // [Team]
    U64 bb_all = {}; // All occupied squares

    /* Game history */
    std::vector<game_record_t> record;

    // Toggle the presence of a piece on a particular square.
    // Unsafe - only has consistent behaviour if the piece is present on the square, or the square is empty
    // Optionally updates zobrist hashes
    template<bool HASH>
    void switch_piece(Team side, Piece piece, uint8_t sq);

    // std::cout << *this
    void print();
};

inline std::ostream &operator<<(std::ostream &stream, const board_t &board) {
    stream << std::endl;

    for (int i = 1; i <= board.history().size(); i++) {
        if (i % 2 != 0) {
            stream << " " << ((i + 1) / 2) << ". ";
        }

        stream << board.history()[i].prev_move << " ";
    }

    stream << std::endl;

    for (int i = 7; i >= 0; i--) {
        for (int j = 0; j < 8; j++) {
            auto file = uint8_t(j);
            auto rank = uint8_t(i);
            if (board.sq(file, rank).occupied()) {
                switch (board.sq(file, rank).piece()) {
                    case PAWN:
                        stream << (board.sq(file, rank).team() ? "p" : "P");
                        break;
                    case KNIGHT:
                        stream << (board.sq(file, rank).team() ? "n" : "N");
                        break;
                    case BISHOP:
                        stream << (board.sq(file, rank).team() ? "b" : "B");
                        break;
                    case ROOK:
                        stream << (board.sq(file, rank).team() ? "r" : "R");
                        break;
                    case QUEEN:
                        stream << (board.sq(file, rank).team() ? "q" : "Q");
                        break;
                    case KING:
                        stream << (board.sq(file, rank).team() ? "k" : "K");
                        break;
                    default:
                        assert(false);
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
