//
// Created by Vincent on 21/10/2017.
//
#include <string>

#include "move.h"
#include "bb.h"

bool operator==(const move_t& lhs, const move_t& rhs)
{
    return lhs.move_bytes == rhs.move_bytes;
}

bool operator!=(const move_t& lhs, const move_t& rhs)
{
    return lhs.move_bytes != rhs.move_bytes;
}

std::ostream& operator<<(std::ostream& stream, const move_t &move) {
    stream << from_sq(move.info.from) << from_sq(move.info.to);

    if(move.info.is_promotion) {
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

    return stream;
}
