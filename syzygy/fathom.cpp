//
// Created by Vincent on 28/08/2021.
//

#include "fathom.h"
#include "tbprobe.h"
#include "../board.h"

namespace {
    WDL read_wdl(unsigned tb_result) {
        switch (TB_GET_WDL(tb_result)) {
            case TB_LOSS:
                return WDL::LOSS;
            case TB_BLESSED_LOSS:
                return WDL::BLESSED_LOSS;
            case TB_DRAW:
                return WDL::DRAW;
            case TB_CURSED_WIN:
                return WDL::CURSED_WIN;
            case TB_WIN:
                return WDL::WIN;
            default:
                return WDL::INVALID;
        }
    }

    packed_move_t read_move(unsigned tb_result) {
        packed_move_t move = {};
        move.from = TB_GET_FROM(tb_result);
        move.to = TB_GET_TO(tb_result);
        switch (TB_GET_PROMOTES(tb_result)) {
            case TB_PROMOTES_NONE:
                move.type = 0;
                break;
            case TB_PROMOTES_KNIGHT:
                move.type = KNIGHT;
                break;
            case TB_PROMOTES_BISHOP:
                move.type = BISHOP;
                break;
            case TB_PROMOTES_ROOK:
                move.type = ROOK;
                break;
            case TB_PROMOTES_QUEEN:
                move.type = QUEEN;
                break;
        }
        return move;
    }
}

unsigned tb_largest() {
    return TB_LARGEST;
}

void init_tb(const std::string &path) {
    if (!tb_init(path.c_str()))
        throw std::runtime_error("failed to initialise syzygy tablebases");
}

std::vector<move_t> probe_root(const board_t &board) {
    unsigned results[TB_MAX_MOVES];
    unsigned result = tb_probe_root(
            board.side(WHITE), board.side(BLACK),
            board.pieces(WHITE, KING) | board.pieces(BLACK, KING),
            board.pieces(WHITE, QUEEN) | board.pieces(BLACK, QUEEN),
            board.pieces(WHITE, ROOK) | board.pieces(BLACK, ROOK),
            board.pieces(WHITE, BISHOP) | board.pieces(BLACK, BISHOP),
            board.pieces(WHITE, KNIGHT) | board.pieces(BLACK, KNIGHT),
            board.pieces(WHITE, PAWN) | board.pieces(BLACK, PAWN),
            board.now().halfmove_clock,
            board.now().castle[WHITE][0] * TB_CASTLING_K
            | board.now().castle[WHITE][1] * TB_CASTLING_Q
            | board.now().castle[BLACK][0] * TB_CASTLING_k
            | board.now().castle[BLACK][1] * TB_CASTLING_q,
            board.now().ep_square,
            board.now().next_move == WHITE,
            results
    );

    WDL best_wdl = read_wdl(result);
    std::vector<move_t> moves;

    for (unsigned *r = results; *r != TB_RESULT_FAILED; ++r) {
        if (read_wdl(*r) == best_wdl) {
            moves.push_back(board.to_move(read_move(*r)));
        }
    }

    return moves;
}

std::optional<WDL> probe_wdl(const board_t &board) {
    unsigned result = tb_probe_wdl(
            board.side(WHITE), board.side(BLACK),
            board.pieces(WHITE, KING) | board.pieces(BLACK, KING),
            board.pieces(WHITE, QUEEN) | board.pieces(BLACK, QUEEN),
            board.pieces(WHITE, ROOK) | board.pieces(BLACK, ROOK),
            board.pieces(WHITE, BISHOP) | board.pieces(BLACK, BISHOP),
            board.pieces(WHITE, KNIGHT) | board.pieces(BLACK, KNIGHT),
            board.pieces(WHITE, PAWN) | board.pieces(BLACK, PAWN),
            board.now().halfmove_clock,
            board.now().castle[WHITE][0] * TB_CASTLING_K
            | board.now().castle[WHITE][1] * TB_CASTLING_Q
            | board.now().castle[BLACK][0] * TB_CASTLING_k
            | board.now().castle[BLACK][1] * TB_CASTLING_q,
            board.now().ep_square,
            board.now().next_move == WHITE
    );

    if (result == TB_RESULT_FAILED)
        return std::nullopt;
    return read_wdl(result);
}
