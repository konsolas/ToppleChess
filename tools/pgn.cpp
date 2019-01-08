//
// Created by Vincent Tang on 2019-01-05.
//

#include "pgn.h"
#include "../move.h"

pgn_t::pgn_t(std::string &pgn_file) : pgn_file(std::stringstream(pgn_file)) {

}

std::vector<game_t> pgn_t::read_games() {
    std::vector<board_t> games;


}

void pgn_t::skip_pgn_header(std::stringstream &stream) {
    std::string next_line;
    do {
        std::getline(stream, next_line);
    } while (next_line[0] == '[');
}

move_t pgn_t::read_san(board_t &board, std::string san_move) {
    move_t result{};
    result.info.team = board.record[board.now].next_move;

    // Handle castling moves
    if (san_move == "0-0") {
        result.info.piece = KING;
        result.info.from = result.info.team ? E8 : E1;
        result.info.to = result.info.team ? G8 : G1;
        result.info.castle = 1;
        result.info.castle_side = 0;

        return result;
    } else if (san_move == "0-0-0") {
        result.info.piece = KING;
        result.info.from = result.info.team ? E8 : E1;
        result.info.to = result.info.team ? C8 : C1;
        result.info.castle = 1;
        result.info.castle_side = 1;

        return result;
    }

    switch (san_move[0]) {
        case 'N':
            result.info.piece = KNIGHT;
            break;
        case 'B':
            result.info.piece = BISHOP;
            break;
        case 'R':
            result.info.piece = ROOK;
            break;
        case 'Q':
            result.info.piece = QUEEN;
            break;
        case 'K':
            result.info.piece = KING;
            break;
        default:
            // Pawn move
            result.info.piece = PAWN;
            if (san_move[1] == 'x') {
                // Pawn capture
                result.info.to = to_sq(san_move[2], san_move[3]);
                result.info.is_capture = 1;
                result.info.captured_type = board.sq_data[result.info.to].piece;
                result.info.is_ep = uint16_t(board.record[board.now].ep_square == result.info.to);
                if (result.info.is_ep) {
                    result.info.captured_type = PAWN;
                }

                auto from_1 = uint8_t(result.info.to + rel_offset(Team(result.info.team), D_SE));
                auto from_2 = uint8_t(result.info.to + rel_offset(Team(result.info.team), D_SW));

                if (board.sq_data[from_1].occupied
                    && board.sq_data[from_1].team == result.info.team
                    && board.sq_data[from_1].piece == PAWN
                    && file_index(from_1) == (san_move[0] - 'a')) {
                    result.info.from = from_1;
                } else if (board.sq_data[from_2].occupied
                           && board.sq_data[from_2].team == result.info.team
                           && board.sq_data[from_2].piece == PAWN
                           && file_index(from_2) == (san_move[0] - 'a')) {
                    result.info.from = from_2;
                } else {
                    throw std::invalid_argument("Invalid pawn capture " + san_move);
                }
            } else {
                // Normal pawn move
                result.info.to = to_sq(san_move[0], san_move[1]);

                auto from_1 = uint8_t(result.info.to + rel_offset(Team(result.info.team), D_S));
                auto from_2 = uint8_t(from_1 + rel_offset(Team(result.info.team), D_S));

                if (board.sq_data[from_1].occupied
                    && board.sq_data[from_1].team == result.info.team
                    && board.sq_data[from_1].piece == PAWN) {
                    result.info.from = from_1;
                } else if ((rank_index(from_2) == 1 || rank_index(from_2) == 6)
                            && board.sq_data[from_2].occupied
                           && board.sq_data[from_2].team == result.info.team
                           && board.sq_data[from_2].piece == PAWN) {
                    result.info.from = from_2;
                } else {
                    throw std::invalid_argument("Invalid pawn move " + san_move);
                }
            }

            // Handle promotions
            if(rank_index(result.info.to) == 0 || rank_index(result.info.to) == 8) {
                result.info.is_promotion = 1;
                std::string::size_type loc = san_move.find('=', 0);
                if(loc != std::string::npos) {
                    char type = san_move[loc + 1];
                    switch (type) {
                        case 'N':
                            result.info.promotion_type = KNIGHT;
                            break;
                        case 'B':
                            result.info.promotion_type = BISHOP;
                            break;
                        case 'R':
                            result.info.promotion_type = ROOK;
                            break;
                        case 'Q':
                            result.info.promotion_type = QUEEN;
                            break;
                        case 'K':
                            result.info.promotion_type = KING;
                            break;
                        default:
                            throw std::invalid_argument("Invalid promotion type");
                    }
                } else {
                    throw std::invalid_argument("Missing promotion type" + san_move);
                }
            }

            return result;
    }

    // TODO: Parse disambiguation
    // TODO: Parse the rest of the move
    // TODO: Handle the end of moves, such as ++, # and +
}
