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

    class entry_t { // 8 bytes
    private:
        uint16_t hash2;
        int16_t value2;
        uint16_t meta2;
        packed_move_t move2;
    public:
        entry_t() = default;
        entry_t(Bound bound, U64 hash, int depth, int ply, int score, move_t move, unsigned gen) :
                hash2(hash >> 48u), value2(score >= MINCHECKMATE ? score + ply : (score <= -MINCHECKMATE ? score - ply : score)),
                meta2(uint16_t(bound) | (uint16_t(depth) << 2u) | (uint16_t(gen) << 10u)), move2(compress(move)) {}

        uint16_t hash16() {
            return hash2;
        }

        int value(int ply) const {
            if (value2 >= MINCHECKMATE) {
                return value2 - ply;
            } else if (value2 <= -MINCHECKMATE) {
                return value2 + ply;
            } else {
                return value2;
            }
        }

        Bound bound() const {
            return Bound(meta2 & 3u);
        }

        int depth() const {
            return (int) ((meta2 >> 2u) & 255u);
        }

        unsigned generation() const {
            return meta2 >> 10u;
        }

        unsigned utility() const {
            return meta2;
        }

        packed_move_t move() const {
            return move2;
        }

        void reset_gen(unsigned gen = 0) {
            meta2 &= 0x3FFu;
            if (gen) meta2 |= gen >> 10u;
        }
    };

    static_assert(sizeof(entry_t) == 8);

    class hash_t {
        static constexpr size_t BUCKET_SIZE = 8;
        static constexpr size_t BUCKET_MEMORY = BUCKET_SIZE * sizeof(tt::entry_t);
    public:
        explicit hash_t(size_t size);
        ~hash_t();

        inline void prefetch(U64 hash) {
            const size_t index = (hash & num_buckets) * BUCKET_SIZE;
            tt::entry_t *bucket = table + index;

#if defined(__GNUC__)
            __builtin_prefetch(bucket);
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
            _mm_prefetch((char*) bucket, _MM_HINT_T0);
#endif
        }

        bool probe(U64 hash, entry_t &entry);
        void save(Bound bound, U64 hash, int depth, int ply, int score, move_t move);
        void age();
        size_t hash_full();
    private:
        size_t num_buckets;
        unsigned generation = 1;
        entry_t *table;
    };
}

#endif //TOPPLE_HASH_H
