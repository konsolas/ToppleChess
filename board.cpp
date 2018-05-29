//
// Created by Vincent on 22/09/2017.
//

#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>

#include "board.h"
#include "testing/catch.hpp"

void board_t::move(move_t move) {
    // Copy the record
    memcpy(&record[now + 1], &record[now], sizeof(game_record_t));
    now += 1;
    record[now].prev_move = move;

    // Update side hash
    record[now].next_move = (Team) !record[now].next_move;
    record[now].hash ^= zobrist::side;

    // Update ep hash
    if (record[now - 1].ep_square != 0) {
        record[now].ep_square = 0;
        record[now].hash ^= zobrist::ep[record[now - 1].ep_square];
    }

    if (move != EMPTY_MOVE) {
        auto team = Team(move.team);
        auto x_team = Team(!move.team);

        // Update halfmove clock
        if (move.piece == PAWN || move.is_capture) {
            record[now].halfmove_clock = 0;
        } else {
            record[now].halfmove_clock++;
        }

        if (move.is_capture && move.captured_type == ROOK) {
            if (move.to == (x_team ? H8 : H1) && record[now].castle[x_team][0]) {
                record[now].castle[x_team][0] = false;
                record[now].hash ^= zobrist::castle[x_team][0];
            } else if (move.to == (x_team ? A8 : A1) && record[now].castle[x_team][1]) {
                record[now].castle[x_team][1] = false;
                record[now].hash ^= zobrist::castle[x_team][1];
            }
        }

        if (move.piece == PAWN) {
            if (move.is_ep) {
                // Remove captured pawn
                switch_piece<true>(x_team, PAWN, team ? move.to + uint8_t(8) : move.to - uint8_t(8));
            } else if (move.is_capture) {
                switch_piece<true>(x_team, (Piece) move.captured_type, move.to);
            }

            if (move.is_promotion) {
                switch_piece<true>(team, (Piece) move.piece, move.from);
                switch_piece<true>(team, (Piece) move.promotion_type, move.to);
            } else {
                switch_piece<true>(team, (Piece) move.piece, move.from);
                switch_piece<true>(team, (Piece) move.piece, move.to);
            }

            // Update en-passant square
            if (team ? move.to - move.from == -16 : move.to - move.from == 16) {
                record[now].ep_square = team ? move.to + uint8_t(8) : move.to - uint8_t(8);
                record[now].hash ^= zobrist::ep[record[now].ep_square];
            }
        } else {
            // Update castling hashes for moving rook
            if (move.piece == ROOK) {
                if (team) {
                    if (move.from == H8 && record[now].castle[team][0]) {
                        record[now].hash ^= zobrist::castle[team][0];
                        record[now].castle[team][0] = false;
                    } else if (move.from == A8 && record[now].castle[team][1]) {
                        record[now].hash ^= zobrist::castle[team][1];
                        record[now].castle[team][1] = false;
                    }
                } else {
                    if (move.from == H1 && record[now].castle[team][0]) {
                        record[now].hash ^= zobrist::castle[team][0];
                        record[now].castle[team][0] = false;
                    } else if (move.from == A1 && record[now].castle[team][1]) {
                        record[now].hash ^= zobrist::castle[team][1];
                        record[now].castle[team][1] = false;
                    }
                }
            } else if (move.piece == KING) {
                // Update castling hashes
                if (record[now].castle[team][0]) {
                    record[now].hash ^= zobrist::castle[team][0];
                    record[now].castle[team][0] = false;
                }
                if (record[now].castle[team][1]) {
                    record[now].hash ^= zobrist::castle[team][1];
                    record[now].castle[team][1] = false;
                }

                if (move.castle) {
                    // Move rook
                    switch_piece<true>(team, ROOK,
                                 move.castle_side ? (team ? A8 : A1) : (team ? H8 : H1));
                    switch_piece<true>(team, ROOK,
                                 move.castle_side ? (team ? D8 : D1) : (team ? F8 : F1));
                }
            }

            if (move.is_capture) {
                switch_piece<true>(x_team, (Piece) move.captured_type, move.to);
            }
            switch_piece<true>(team, (Piece) move.piece, move.from);
            switch_piece<true>(team, (Piece) move.piece, move.to);
        }
    }
}

void board_t::unmove() {
    move_t move = record[now].prev_move;
    now -= 1;

    if (move != EMPTY_MOVE) {
        if (move.piece == PAWN) {
            if (move.is_promotion) {
                switch_piece<false>((Team) move.team, (Piece) move.piece, move.from);
                switch_piece<false>((Team) move.team, (Piece) move.promotion_type, move.to);
            } else {
                switch_piece<false>((Team) move.team, (Piece) move.piece, move.from);
                switch_piece<false>((Team) move.team, (Piece) move.piece, move.to);
            }

            if (move.is_ep) {
                // Replace captured pawn
                switch_piece<false>((Team) !move.team, PAWN, move.team ? move.to + uint8_t(8) : move.to - uint8_t(8));
            } else if (move.is_capture) {
                switch_piece<false>((Team) !move.team, (Piece) move.captured_type, move.to);
            }
        } else {
            if (move.castle) {
                // Move rook
                switch_piece<false>((Team) move.team, ROOK,
                             move.castle_side ? (move.team ? A8 : A1) : (move.team ? H8 : H1));
                switch_piece<false>((Team) move.team, ROOK,
                             move.castle_side ? (move.team ? D8 : D1) : (move.team ? F8 : F1));
            }

            switch_piece<false>((Team) move.team, (Piece) move.piece, move.from);
            switch_piece<false>((Team) move.team, (Piece) move.piece, move.to);

            if (move.is_capture) {
                switch_piece<false>((Team) !move.team, (Piece) move.captured_type, move.to);
            }
        }
    }
}

board_t::board_t(std::string fen) {
    std::istringstream iss(fen);
    std::vector<std::string> split((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

    if (split.size() > 6 || split.size() < 4) {
        throw std::runtime_error(
                std::string("FEN: ") + std::to_string(split.size()) + std::string(" components, 4-6 expected"));
    }

    // Parse board
    uint8_t file = 0, rank = 7;
    for (char ch : split[0]) {
        if (ch > '0' && ch < '9') {
            file += ch - '0';
        } else {
            uint8_t square = square_index(file, rank);
            switch (ch) {
                case '/':
                    rank--;
                    file = 0;
                    break;
                case 'P':
                    switch_piece<true>(WHITE, PAWN, square);
                    break;
                case 'N':
                    switch_piece<true>(WHITE, KNIGHT, square);
                    break;
                case 'B':
                    switch_piece<true>(WHITE, BISHOP, square);
                    break;
                case 'R':
                    switch_piece<true>(WHITE, ROOK, square);
                    break;
                case 'Q':
                    switch_piece<true>(WHITE, QUEEN, square);
                    break;
                case 'K':
                    switch_piece<true>(WHITE, KING, square);
                    break;
                case 'p':
                    switch_piece<true>(BLACK, PAWN, square);
                    break;
                case 'n':
                    switch_piece<true>(BLACK, KNIGHT, square);
                    break;
                case 'b':
                    switch_piece<true>(BLACK, BISHOP, square);
                    break;
                case 'r':
                    switch_piece<true>(BLACK, ROOK, square);
                    break;
                case 'q':
                    switch_piece<true>(BLACK, QUEEN, square);
                    break;
                case 'k':
                    switch_piece<true>(BLACK, KING, square);
                    break;
                default:
                    throw std::runtime_error(std::string("Unrecognised FEN character: '") + ch + "'");
            }

            if (ch != '/') file++;
        }
    }

    // Parse colour
    if (split[1][0] == 'w') {
        record[0].next_move = WHITE;
    } else if (split[1][0] == 'b') {
        record[0].next_move = BLACK;
        record[0].hash ^= zobrist::side;
    } else {
        throw std::runtime_error("Unrecognised descriptor (looking for w/b): " + split[1]);
    }

    // Parse castling
    for (char i : split[2]) {
        if (i == 'K') {
            record[0].castle[WHITE][0] = true;
            record[0].hash ^= zobrist::castle[WHITE][0];
        } else if (i == 'Q') {
            record[0].castle[WHITE][1] = true;
            record[0].hash ^= zobrist::castle[WHITE][1];
        } else if (i == 'k') {
            record[0].castle[BLACK][0] = true;
            record[0].hash ^= zobrist::castle[BLACK][0];
        } else if (i == 'q') {
            record[0].castle[BLACK][1] = true;
            record[0].hash ^= zobrist::castle[BLACK][1];
        }
    }


    // Parse ep square
    if (split[3] != "-") {
        if (split[3].length() != 2) {
            throw std::runtime_error("Length of ep square is not 2");
        } else {
            record[0].ep_square = to_sq(split[3][0], split[3][1]);
        }
    }

    // Parse halfmove clock
    if (split.size() > 4) {
        record[0].halfmove_clock = std::stoi(split[4]);
    } else {
        record[0].halfmove_clock = 0;
    }

    // Ignore fullmove number - nobody cares

}

// This does not properly consider legality:
//  - if promotion is actually possible
//  - if the piece can actually make the move
//  - if the piece actually exists
move_t board_t::parse_move(const std::string &str) {
    move_t move = {0};

    // Location
    move.from = to_sq(str[0], str[1]);
    move.to = to_sq(str[2], str[3]);

    // Piece
    move.team = sq_data[move.from].team;
    move.piece = sq_data[move.from].piece;

    // Capture
    move.is_capture = static_cast<uint16_t>(sq_data[move.to].occupied);
    move.captured_type = sq_data[move.to].occupied ? sq_data[move.to].piece : 0;

    // Promotion
    move.is_promotion = static_cast<uint16_t>(str.length() == 5);
    if(move.is_promotion) {
        switch (str[4]) {
            case 'n':
                move.promotion_type = KNIGHT;
                break;
            case 'b':
                move.promotion_type = BISHOP;
                break;
            case 'r':
                move.promotion_type = ROOK;
                break;
            case 'q':
                move.promotion_type = QUEEN;
                break;
            default:
                throw std::runtime_error("invalid promotion type");
        }
    }

    // Castling
    if(move.piece == KING && (move.from == E1 || move.from == E8)
        && abs(move.to - move.from) == 2) {
        move.castle = 1;

        // 0 = kingside, 1 = queenside
        move.castle_side = static_cast<uint16_t>(move.to == G1 || move.to == G8);
    }

    // EP
    move.is_ep = static_cast<uint16_t>(move.piece == PAWN && move.to == record[now].ep_square);

    return move;
}

bool board_t::is_illegal() {
    Team side = record[now].next_move;
    U64 king_bb = bb_pieces[!side][KING];
    uint8_t king_square = popBit(WHITE, king_bb);

    return is_attacked(king_square, side);
}


bool board_t::is_incheck() {
    Team side = record[now].next_move;
    U64 king_bb = bb_pieces[side][KING];
    uint8_t king_square = popBit(WHITE, king_bb);

    return is_attacked(king_square, Team(!side));
}

template<bool HASH>
void board_t::switch_piece(Team side, Piece piece, uint8_t sq) {
    const U64 bb_square = single_bit(sq);

    // Occupancy
    sq_data[sq].occupied = !sq_data[sq].occupied;
    sq_data[sq].piece = piece;
    sq_data[sq].team = side;

    { // Update bitboards
        // No error checking! We assume that the piece exists
        bb_all ^= bb_square;
        bb_side[side] ^= bb_square;
        bb_pieces[side][piece] ^= bb_square;
    }

    if(HASH) { // Update hash
        record[now].hash ^= zobrist::squares[sq][side][piece];
    }
}

U64 board_t::attacks_to(uint8_t sq, Team side) {
    auto x_side = Team(!side);
    U64 attacks =
            (find_moves(QUEEN, x_side, sq, bb_all) & bb_pieces[side][QUEEN]) |
            (find_moves(ROOK, x_side, sq, bb_all) & bb_pieces[side][ROOK]) |
            (find_moves(KNIGHT, x_side, sq, bb_all) & bb_pieces[side][KNIGHT]) |
            (find_moves(BISHOP, x_side, sq, bb_all) & bb_pieces[side][BISHOP]) |
            (find_moves(PAWN, x_side, sq, bb_all) & bb_pieces[side][PAWN]) |
            (find_moves(KING, x_side, sq, bb_all) & bb_pieces[side][KING]);

    return attacks;
}

bool board_t::is_attacked(uint8_t sq, Team side) {
    auto x_side = Team(!side);
    return (find_moves(QUEEN, x_side, sq, bb_all) & bb_pieces[side][QUEEN]) ||
           (find_moves(ROOK, x_side, sq, bb_all) & bb_pieces[side][ROOK]) ||
           (find_moves(KNIGHT, x_side, sq, bb_all) & bb_pieces[side][KNIGHT]) ||
           (find_moves(BISHOP, x_side, sq, bb_all) & bb_pieces[side][BISHOP]) ||
           (find_moves(PAWN, x_side, sq, bb_all) & bb_pieces[side][PAWN]) ||
           (find_moves(KING, x_side, sq, bb_all) & bb_pieces[side][KING]);
}

bool board_t::is_pseudo_legal(move_t move) {
    auto team = Team(move.team);
    auto x_team = Team(!move.team);

    if (record[now].next_move != team) return false;

    if (move.castle) {
        if (record[now].castle[move.team][move.castle_side] == 0) return false;
        if (move.castle_side == 0) {
            return (bb_all & bits_between(team ? E8 : E1, team ? H8 : H1)) == 0 &&
                   !is_attacked(team ? E8 : E1, x_team) &&
                   !is_attacked(team ? F8 : F1, x_team) &&
                   !is_attacked(team ? G8 : G1, x_team);
        } else {
            return (bb_all & bits_between(team ? E8 : E1, team ? A8 : A1)) == 0 &&
                   !is_attacked(team ? E8 : E1, x_team) &&
                   !is_attacked(team ? D8 : D1, x_team) &&
                   !is_attacked(team ? C8 : C1, x_team);
        }
    }

    if (move.is_capture) {
        if ((bb_pieces[x_team][move.captured_type] & single_bit(move.to)) == 0) return false;
    }

    return (bb_pieces[team][move.piece] & single_bit(move.from)) && !(bb_side[team] & single_bit(move.to));
}

bool board_t::is_repetition_draw() {
    int rep = 1;

    int last = now - 50;

    for(int i = now - 2; i > last; i -= 2) {
        if(record[i].hash == record[now].hash) rep++;
        if(rep >= 3) {
            return true;
        }
    }

    return false;
}

void board_t::mirror() {
    for(uint8_t sq = 0; sq < 64; sq++) {
        Team team = sq_data[sq].team;
        Piece piece = sq_data[sq].piece;


    }
}
