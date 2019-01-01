//
// Created by Vincent on 22/09/2017.
//

#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>

#include "board.h"
#include "testing/catch.hpp"
#include "move.h"

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
        auto team = Team(move.info.team);
        auto x_team = Team(!move.info.team);

        // Update halfmove clock
        if (move.info.piece == PAWN || move.info.is_capture) {
            record[now].halfmove_clock = 0;
        } else {
            record[now].halfmove_clock++;
        }

        if (move.info.is_capture && move.info.captured_type == ROOK) {
            if (move.info.to == (x_team ? H8 : H1) && record[now].castle[x_team][0]) {
                record[now].castle[x_team][0] = false;
                record[now].hash ^= zobrist::castle[x_team][0];
            } else if (move.info.to == (x_team ? A8 : A1) && record[now].castle[x_team][1]) {
                record[now].castle[x_team][1] = false;
                record[now].hash ^= zobrist::castle[x_team][1];
            }
        }

        if (move.info.piece == PAWN) {
            if (move.info.is_ep) {
                // Remove captured pawn
                switch_piece<true>(x_team, PAWN, team ? move.info.to + uint8_t(8) : move.info.to - uint8_t(8));
            } else if (move.info.is_capture) {
                switch_piece<true>(x_team, (Piece) move.info.captured_type, move.info.to);
            }

            if (move.info.is_promotion) {
                switch_piece<true>(team, (Piece) move.info.piece, move.info.from);
                switch_piece<true>(team, (Piece) move.info.promotion_type, move.info.to);
            } else {
                switch_piece<true>(team, (Piece) move.info.piece, move.info.from);
                switch_piece<true>(team, (Piece) move.info.piece, move.info.to);
            }

            // Update en-passant square
            if (team ? move.info.to - move.info.from == -16 : move.info.to - move.info.from == 16) {
                record[now].ep_square = team ? move.info.to + uint8_t(8) : move.info.to - uint8_t(8);
                record[now].hash ^= zobrist::ep[record[now].ep_square];
            }
        } else {
            // Update castling hashes for moving rook
            if (move.info.piece == ROOK) {
                if (team) {
                    if (move.info.from == H8 && record[now].castle[team][0]) {
                        record[now].hash ^= zobrist::castle[team][0];
                        record[now].castle[team][0] = false;
                    } else if (move.info.from == A8 && record[now].castle[team][1]) {
                        record[now].hash ^= zobrist::castle[team][1];
                        record[now].castle[team][1] = false;
                    }
                } else {
                    if (move.info.from == H1 && record[now].castle[team][0]) {
                        record[now].hash ^= zobrist::castle[team][0];
                        record[now].castle[team][0] = false;
                    } else if (move.info.from == A1 && record[now].castle[team][1]) {
                        record[now].hash ^= zobrist::castle[team][1];
                        record[now].castle[team][1] = false;
                    }
                }
            } else if (move.info.piece == KING) {
                // Update castling hashes
                if (record[now].castle[team][0]) {
                    record[now].hash ^= zobrist::castle[team][0];
                    record[now].castle[team][0] = false;
                }
                if (record[now].castle[team][1]) {
                    record[now].hash ^= zobrist::castle[team][1];
                    record[now].castle[team][1] = false;
                }

                if (move.info.castle) {
                    // Move rook
                    switch_piece<true>(team, ROOK,
                                       move.info.castle_side ? (team ? A8 : A1) : (team ? H8 : H1));
                    switch_piece<true>(team, ROOK,
                                       move.info.castle_side ? (team ? D8 : D1) : (team ? F8 : F1));
                }
            }

            if (move.info.is_capture) {
                switch_piece<true>(x_team, (Piece) move.info.captured_type, move.info.to);
            }
            switch_piece<true>(team, (Piece) move.info.piece, move.info.from);
            switch_piece<true>(team, (Piece) move.info.piece, move.info.to);
        }
    }
}

void board_t::unmove() {
    move_t move = record[now].prev_move;
    now -= 1;

    if (move != EMPTY_MOVE) {
        if (move.info.piece == PAWN) {
            if (move.info.is_promotion) {
                switch_piece<false>((Team) move.info.team, (Piece) move.info.piece, move.info.from);
                switch_piece<false>((Team) move.info.team, (Piece) move.info.promotion_type, move.info.to);
            } else {
                switch_piece<false>((Team) move.info.team, (Piece) move.info.piece, move.info.from);
                switch_piece<false>((Team) move.info.team, (Piece) move.info.piece, move.info.to);
            }

            if (move.info.is_ep) {
                // Replace captured pawn
                switch_piece<false>((Team) !move.info.team, PAWN,
                                    move.info.team ? move.info.to + uint8_t(8) : move.info.to - uint8_t(8));
            } else if (move.info.is_capture) {
                switch_piece<false>((Team) !move.info.team, (Piece) move.info.captured_type, move.info.to);
            }
        } else {
            if (move.info.castle) {
                // Move rook
                switch_piece<false>((Team) move.info.team, ROOK,
                                    move.info.castle_side ? (move.info.team ? A8 : A1) : (move.info.team ? H8 : H1));
                switch_piece<false>((Team) move.info.team, ROOK,
                                    move.info.castle_side ? (move.info.team ? D8 : D1) : (move.info.team ? F8 : F1));
            }

            switch_piece<false>((Team) move.info.team, (Piece) move.info.piece, move.info.from);
            switch_piece<false>((Team) move.info.team, (Piece) move.info.piece, move.info.to);

            if (move.info.is_capture) {
                switch_piece<false>((Team) !move.info.team, (Piece) move.info.captured_type, move.info.to);
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
// If the move cannot be parsed, 0000 is returned
move_t board_t::parse_move(const std::string &str) {
    move_t move = {};

    if (str.length() != 4 && str.length() != 5) {
        return EMPTY_MOVE;
    }

    // Location
    try {
        move.info.from = to_sq(str[0], str[1]);
        move.info.to = to_sq(str[2], str[3]);
    } catch (std::runtime_error &e) {
        return EMPTY_MOVE;
    }

    // Piece
    move.info.team = sq_data[move.info.from].team;
    move.info.piece = sq_data[move.info.from].piece;

    // Capture
    move.info.is_capture = static_cast<uint16_t>(sq_data[move.info.to].occupied);
    move.info.captured_type = sq_data[move.info.to].occupied ? sq_data[move.info.to].piece : 0;

    // Promotion
    move.info.is_promotion = static_cast<uint16_t>(str.length() == 5);
    if (move.info.is_promotion) {
        switch (str[4]) {
            case 'n':
                move.info.promotion_type = KNIGHT;
                break;
            case 'b':
                move.info.promotion_type = BISHOP;
                break;
            case 'r':
                move.info.promotion_type = ROOK;
                break;
            case 'q':
                move.info.promotion_type = QUEEN;
                break;
            default:
                return EMPTY_MOVE;
        }
    }

    // Castling
    if (move.info.piece == KING) {
        if (move.info.from == E1 || move.info.from == E8) {
            if (move.info.to == G1 || move.info.to == G8) {
                move.info.castle = 1;
                move.info.castle_side = 0;
            } else if (move.info.to == C1 || move.info.to == C8) {
                move.info.castle = 1;
                move.info.castle_side = 1;
            }
        }
    }

    // EP
    move.info.is_ep = static_cast<uint16_t>(record[now].ep_square != 0
                                            && move.info.piece == PAWN
                                            && move.info.to == record[now].ep_square);

    return move;
}

bool board_t::is_illegal() {
    Team side = record[now].next_move;
    uint8_t king_square = bit_scan(bb_pieces[!side][KING]);

    return is_attacked(king_square, side);
}


bool board_t::is_incheck() {
    Team side = record[now].next_move;
    if(bb_pieces[side][KING] == 0) {
        std::cout << *this << std::endl;
    }
    uint8_t king_square = bit_scan(bb_pieces[side][KING]);

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

    if (HASH) { // Update hash
        record[now].hash ^= zobrist::squares[sq][side][piece];
    }
}

U64 board_t::attacks_to(uint8_t sq, Team side) {
    auto x_side = Team(!side);
    U64 attacks =
            (find_moves<QUEEN>(x_side, sq, bb_all) & bb_pieces[side][QUEEN]) |
            (find_moves<ROOK>(x_side, sq, bb_all) & bb_pieces[side][ROOK]) |
            (find_moves<KNIGHT>(x_side, sq, bb_all) & bb_pieces[side][KNIGHT]) |
            (find_moves<BISHOP>(x_side, sq, bb_all) & bb_pieces[side][BISHOP]) |
            (find_moves<PAWN>(x_side, sq, bb_all) & bb_pieces[side][PAWN]) |
            (find_moves<KING>(x_side, sq, bb_all) & bb_pieces[side][KING]);

    return attacks;
}

bool board_t::is_attacked(uint8_t sq, Team side) {
    auto x_side = Team(!side);
    return (find_moves<QUEEN>(x_side, sq, bb_all) & bb_pieces[side][QUEEN]) ||
           (find_moves<ROOK>(x_side, sq, bb_all) & bb_pieces[side][ROOK]) ||
           (find_moves<KNIGHT>(x_side, sq, bb_all) & bb_pieces[side][KNIGHT]) ||
           (find_moves<BISHOP>(x_side, sq, bb_all) & bb_pieces[side][BISHOP]) ||
           (find_moves<PAWN>(x_side, sq, bb_all) & bb_pieces[side][PAWN]) ||
           (find_moves<KING>(x_side, sq, bb_all) & bb_pieces[side][KING]);
}

bool board_t::is_pseudo_legal(move_t move) {
    if (move == EMPTY_MOVE) return false;

    auto team = Team(move.info.team);
    auto x_team = Team(!move.info.team);

    if (record[now].next_move != team) return false;

    if (move.info.castle) {
        if (record[now].castle[move.info.team][move.info.castle_side] == 0) return false;
        if (move.info.castle_side == 0) {
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

    if (move.info.is_capture) {
        if ((bb_pieces[x_team][move.info.captured_type] & single_bit(move.info.to)) == 0) return false;
    } else {
        if ((bb_pieces[x_team][move.info.captured_type] & single_bit(move.info.to)) != 0) return false;
    }

    if(move.info.is_ep) {
        if(record[now].ep_square == 0 || move.info.to != record[now].ep_square) {
            return false;
        }
    } else {
        if ((find_moves(Piece(move.info.piece), team, move.info.from, bb_all) & single_bit(move.info.to)) == 0) {
            return false;
        }
    }

    return (bb_pieces[team][move.info.piece] & single_bit(move.info.from)) &&
           !(bb_side[team] & single_bit(move.info.to));
}

bool board_t::is_repetition_draw(int ply, int reps) {
    int rep = 1;

    int last = std::max(now - ply, 0);

    for (int i = now - 2; i > last; i -= 2) {
        if (record[i].hash == record[now].hash) rep++;
        if (rep >= reps) {
            return true;
        }
    }

    return false;
}

void board_t::mirror() {
    // Mirror side
    record[now].next_move = (Team) !record[now].next_move;
    record[now].hash ^= zobrist::side;

    // Mirror en-passant
    if (record[now].ep_square != 0) {
        record[now].hash ^= zobrist::ep[record[now].ep_square];
        record[now].ep_square = MIRROR_TABLE[record[now].ep_square];
        record[now].hash ^= zobrist::ep[record[now].ep_square];
    }

    // Mirror pieces
    for (uint8_t sq = 0; sq < 64; sq++) {
        Team team = sq_data[sq].team;
        Piece piece = sq_data[sq].piece;

        switch_piece<false>(team, piece, sq);
        switch_piece<false>(Team(!team), piece, MIRROR_TABLE[sq]);
    }
}

int board_t::see(move_t move) {
    if(move == EMPTY_MOVE)
        return 0;

    // State
    U64 attackers = attacks_to(move.info.to, Team(move.info.team)) | attacks_to(move.info.to, Team(!move.info.team));
    U64 occupation_mask = ONES;
    int current_target_val = 0;
    bool prom_rank = rank_index(move.info.to) == 0 || rank_index(move.info.to) == 7;
    auto next_move = Team(move.info.team);

    // Material table
    int num_capts = 0;
    int material[32] = {0};

    // Eval move
    material[num_capts] = sq_data[move.info.to].occupied ? VAL[sq_data[move.info.to].piece] : 0;
    current_target_val = VAL[move.info.piece];
    if (prom_rank && move.info.piece == PAWN) {
        material[num_capts] += VAL[move.info.promotion_type] - VAL[PAWN];
        current_target_val += VAL[move.info.promotion_type] - VAL[PAWN];
    }
    num_capts++;

    // Remove attacker
    attackers &= ~single_bit(move.info.from);
    occupation_mask &= ~single_bit(move.info.from);

    // Reveal next attacker
    attackers |= (find_moves<QUEEN>(next_move, move.info.to, bb_all & occupation_mask)
                  & (bb_pieces[WHITE][QUEEN] | bb_pieces[BLACK][QUEEN])) |
                 (find_moves<BISHOP>(next_move, move.info.to, bb_all & occupation_mask)
                  & (bb_pieces[WHITE][BISHOP] | bb_pieces[BLACK][BISHOP])) |
                 (find_moves<ROOK>(next_move, move.info.to, bb_all & occupation_mask)
                  & (bb_pieces[WHITE][ROOK] | bb_pieces[BLACK][ROOK]));
    attackers &= occupation_mask;

    next_move = Team(!next_move);

    while (attackers) {
        // Pick LVA
        Square from;
        if (!prom_rank && attackers & bb_pieces[next_move][PAWN])
            from = Square(bit_scan(attackers & bb_pieces[next_move][PAWN]));
        else if (attackers & bb_pieces[next_move][KNIGHT])
            from = Square(bit_scan(attackers & bb_pieces[next_move][KNIGHT]));
        else if (attackers & bb_pieces[next_move][BISHOP])
            from = Square(bit_scan(attackers & bb_pieces[next_move][BISHOP]));
        else if (attackers & bb_pieces[next_move][ROOK])
            from = Square(bit_scan(attackers & bb_pieces[next_move][ROOK]));
        else if (prom_rank && attackers & bb_pieces[next_move][PAWN])
            from = Square(bit_scan(attackers & bb_pieces[next_move][PAWN]));
        else if (attackers & bb_pieces[next_move][QUEEN])
            from = Square(bit_scan(attackers & bb_pieces[next_move][QUEEN]));
        else if (attackers & bb_pieces[next_move][KING] && !(attackers & bb_side[!next_move]))
            from = Square(bit_scan(attackers & bb_pieces[next_move][KING]));
        else break;

        // Eval move
        material[num_capts] = -material[num_capts - 1] + current_target_val;
        current_target_val = VAL[sq_data[from].piece];
        if (prom_rank && sq_data[from].piece == PAWN) {
            material[num_capts] += VAL[QUEEN] - VAL[PAWN];
            current_target_val = VAL[QUEEN] - VAL[PAWN];
        }
        num_capts++;

        // Remove attacker
        attackers &= ~single_bit(from);
        occupation_mask &= ~single_bit(from);

        // Reveal next attacker
        attackers |= (find_moves<QUEEN>(next_move, move.info.to, bb_all & occupation_mask)
                      & (bb_pieces[WHITE][QUEEN] | bb_pieces[BLACK][QUEEN])) |
                     (find_moves<BISHOP>(next_move, move.info.to, bb_all & occupation_mask)
                      & (bb_pieces[WHITE][BISHOP] | bb_pieces[BLACK][BISHOP])) |
                     (find_moves<ROOK>(next_move, move.info.to, bb_all & occupation_mask)
                      & (bb_pieces[WHITE][ROOK] | bb_pieces[BLACK][ROOK]));
        attackers &= occupation_mask;

        next_move = Team(!next_move);
    }

    // Calculate SEE of first capture
    while (--num_capts)
        if (material[num_capts] > -material[num_capts - 1])
            material[num_capts - 1] = -material[num_capts];

    return material[0];
}

U64 board_t::non_pawn_material(Team side) {
    return (bb_side[side] ^ bb_pieces[side][PAWN] ^ bb_pieces[side][KING]);
}
