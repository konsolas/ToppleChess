//
// Created by Vincent on 24/09/2017.
//

#ifndef TOPPLE_MOVE_H
#define TOPPLE_MOVE_H

#include "types.h"
#include "bb.h"

#include <iostream>

/**
 * Represents a move in the game
 */
#pragma pack(push, 1)
union move_t {
    struct {
        uint8_t from;
        uint8_t to;

        uint16_t team : 1,
                piece: 3,
                is_capture: 1,
                captured_type: 3,
                is_promotion: 1,
                promotion_type: 3,
                castle: 1,
                castle_side: 1,
                is_ep: 1,
                : 1; // 16 bits
    } info;

    uint32_t move_bytes;
};
#pragma pack(pop)

static const move_t EMPTY_MOVE = {0};

inline bool operator==(const move_t &lhs, const move_t &rhs) {
    return lhs.move_bytes == rhs.move_bytes;
}

inline bool operator!=(const move_t &lhs, const move_t &rhs) {
    return lhs.move_bytes != rhs.move_bytes;
}

inline std::ostream &operator<<(std::ostream &stream, const move_t &move) {
    if (move == EMPTY_MOVE) {
        stream << "0000";
    } else {
        stream << from_sq(move.info.from) << from_sq(move.info.to);

        if (move.info.is_promotion) {
            switch (move.info.promotion_type) {
                case KNIGHT:
                    stream << "n";
                    break;
                case BISHOP:
                    stream << "b";
                    break;
                case ROOK:
                    stream << "r";
                    break;
                case QUEEN:
                    stream << "q";
                    break;
                default:
                    throw std::runtime_error("invalid promotion type");
            }
        }
    }

    return stream;
}

inline move_t reverse(move_t &move) {
    if(!move.info.is_ep && !move.info.is_capture && !move.info.is_promotion && !move.info.castle) {
        move_t reversed = move;
        reversed.info.from = move.info.to;
        reversed.info.to = move.info.from;

        return reversed;
    }

    return EMPTY_MOVE;
}

#endif //TOPPLE_MOVE_H
