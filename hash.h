//
// Created by Vincent on 23/09/2017.
//

#ifndef TOPPLE_HASH_H
#define TOPPLE_HASH_H

#include <mutex>

#include "types.h"
#include "move.h"

namespace zobrist {
    extern U64 squares[64][2][6];
    extern U64 side;
    extern U64 ep[64];
    extern U64 castle[2][2];

    /**
     * Initialise the hash arrays
     */
    void init_hashes();
}

union material_data_t {
    struct {
        uint32_t w_pawns : 4,
                 w_knights : 4,
                 w_bishops : 4,
                 w_rooks : 4,
                 w_queens : 4;
        uint32_t b_pawns : 4,
                b_knights : 4,
                b_bishops : 4,
                b_rooks : 4,
                b_queens : 4;
        
        inline unsigned count(Team team, Piece piece) {
            switch (team) {
                case WHITE:
                    switch (piece) {
                        case PAWN: return w_pawns;
                        case KNIGHT: return w_knights;
                        case BISHOP: return w_bishops;
                        case ROOK: return w_rooks;
                        case QUEEN: return w_queens;
                        case KING: return 1;
                    }
                    break;
                case BLACK:
                    switch (piece) {
                        case PAWN: return b_pawns;
                        case KNIGHT: return b_knights;
                        case BISHOP: return b_bishops;
                        case ROOK: return b_rooks;
                        case QUEEN: return b_queens;
                        case KING: return 1;
                    }
                    break;
            }

            return 0;
        }

        inline void inc(Team team, Piece piece) {
            switch (team) {
                case WHITE:
                    switch (piece) {
                        case PAWN: w_pawns++; break;
                        case KNIGHT: w_knights++; break;
                        case BISHOP: w_bishops++; break;
                        case ROOK: w_rooks++; break;
                        case QUEEN: w_queens++; break;
                        case KING: break;
                    }
                    break;
                case BLACK:
                    switch (piece) {
                        case PAWN: b_pawns++; break;
                        case KNIGHT: b_knights++; break;
                        case BISHOP: b_bishops++; break;
                        case ROOK: b_rooks++; break;
                        case QUEEN: b_queens++; break;
                        case KING: break;
                    }
                    break;
            }
        }
        
        inline void dec(Team team, Piece piece) {
            switch (team) {
                case WHITE:
                    switch (piece) {
                        case PAWN: w_pawns--; break;
                        case KNIGHT: w_knights--; break;
                        case BISHOP: w_bishops--; break;
                        case ROOK: w_rooks--; break;
                        case QUEEN: w_queens--; break;
                        case KING: break;
                    }
                    break;
                case BLACK:
                    switch (piece) {
                        case PAWN: b_pawns--; break;
                        case KNIGHT: b_knights--; break;
                        case BISHOP: b_bishops--; break;
                        case ROOK: b_rooks--; break;
                        case QUEEN: b_queens--; break;
                        case KING: break;
                    }
                    break;
            }
        }
    } info;

    uint64_t hash;
};

namespace tt {
    enum Bound : uint8_t {
        NONE=0, UPPER, LOWER, EXACT
    };

    inline size_t lower_power_of_2(size_t size) {
        if (size & (size - 1)) { // Check if size is a power of 2
            for (unsigned int i = 1; i < 64; i++) {
                size |= size >> i; // Fill bits to the right
            }

            size += 1; // Add one (so there is only one bit set)
            size >>= 1; // Shift left one
        }

        return size;
    }

    struct entry_t { // 16 bytes
        U64 coded_hash; // 8 bytes
        union {
            struct {
                packed_move_t move;
                int16_t static_eval;
                int16_t internal_value;
                uint16_t about; // 6G 8D 2B
            } info;

            U64 data;
        };

        int value(int ply) const {
            if (info.internal_value >= MINCHECKMATE) {
                return info.internal_value - ply;
            } else if (info.internal_value <= -MINCHECKMATE) {
                return info.internal_value + ply;
            } else {
                return info.internal_value;
            }
        }

        inline Bound bound() {
            return Bound(info.about & 3u);
        }

        inline int depth() {
            return (info.about >> 2u) & 255u;
        }

        inline unsigned generation() {
            return info.about >> 10u;
        }

        void refresh(unsigned gen) {
            U64 original = coded_hash ^ data;
            info.about = uint16_t((info.about & 1023u) | (gen << 10u));
            coded_hash = original ^ data;
        }
    };

    class hash_t {
        static constexpr size_t bucket_size = 4;
    public:
        explicit hash_t(size_t size);
        ~hash_t();

        bool probe(U64 hash, entry_t &entry);
        void prefetch(U64 hash);
        void save(Bound bound, U64 hash, int depth, int ply, int static_eval, int score, move_t move);
        void age();
        size_t hash_full();
    private:
        size_t num_entries;
        unsigned generation = 1;
        entry_t *table;
    };
}

#endif //TOPPLE_HASH_H
