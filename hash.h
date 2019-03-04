//
// Created by Vincent on 23/09/2017.
//

#ifndef TOPPLE_HASH_H
#define TOPPLE_HASH_H

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
        U64 hash; // 8 bytes
        packed_move_t move; // 2 bytes
        int16_t static_eval; // 2 bytes
        int16_t internal_value; // 2 bytes
        uint16_t about; // 2 bytes [GGGGGGGDDDDDDDBB] // 7 bits generation, 7 bits depth, 2 bits bound

        int value(int ply) {
            if (internal_value >= MINCHECKMATE) {
                return internal_value - ply;
            } else if (internal_value <= -MINCHECKMATE) {
                return internal_value + ply;
            } else {
                return internal_value;
            }
        }

        inline Bound bound() {
            return Bound(about & EXACT);
        }

        inline int depth() {
            return (about >> 2) & 127;
        }

        inline int generation() {
            return about >> 9;
        }

        void update(Bound bound, int depth, int generation, move_t best_move, int eval, int score) {
            about = bound | (depth << 2) | (generation << 9);
            move = compress(best_move);
            static_eval = static_cast<int16_t>(eval);
            internal_value = static_cast<int16_t>(score);
        }

        void refresh(int gen) {
            about = uint16_t((about & 511) | (gen << 9));
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
        entry_t *table;
        uint8_t generation = 1;
    };
}

#endif //TOPPLE_HASH_H
