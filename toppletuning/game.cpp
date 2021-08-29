//
// Created by Vincent on 23/05/2020.
//

#include "game.h"

#include "fathom.h"

GameResult invert_result(GameResult result) {
    switch (result) {
        case UNKNOWN:
            return UNKNOWN;
        case WIN:
            return LOSS;
        case DRAW:
            return DRAW;
        case LOSS:
            return WIN;
    }
}

GameResult game_t::play(board_t pos) {
    GameResult result;
    tt::hash_t hash[2] = {tt::hash_t(32 * MB), tt::hash_t(32 * MB)};
    search_t search[2] = {
            search_t(&hash[WHITE], white, 1, true), search_t(&hash[BLACK], black, 1, true)
    };

    while ((result = check_result(pos)) == UNKNOWN) {
        std::atomic_bool aborted = false;
        search_t &player = search[pos.now().next_move];
        player.enable_timer();
        search_result_t search_result = player.think(pos, limits, aborted);
        if (search_result.best_move == EMPTY_MOVE) {
            std::cerr << "search returned empty move" << std::endl;
            return UNKNOWN;
        }
        pos.move(search_result.best_move);
    }

    //std::cout << pos << std::endl;

    return pos.now().next_move == WHITE ? result : invert_result(result);
}

GameResult game_t::check_result(board_t &board) {
    // Game state
    if (board.now().halfmove_clock >= 100
        || board.is_repetition_draw(100)
        || board.is_material_draw()) {
        return DRAW;
    }

    // Probe endgame tablebases
    if (pop_count(board.all()) <= tb_largest()) {
        std::optional<WDL> wdl = probe_wdl(board);
        if (wdl) {
            if (wdl.value() == WDL::LOSS) {
                return LOSS;
            } else if (wdl.value() == WDL::WIN) {
                return WIN;
            } else {
                return DRAW;
            }
        }
    }

    // Check if we have a legal move
    movegen_t movegen(board);
    move_t buf[192];
    int count = movegen.gen_normal(buf);
    for (int i = 0; i < count; i++) {
        if (board.is_legal(buf[i])) return UNKNOWN;
    }

    // No legal moves => we are checkmated or stalemated
    if (board.is_incheck()) return LOSS;
    return DRAW;
}
