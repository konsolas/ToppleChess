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

namespace tt {
    enum Bound : uint8_t {
        UPPER, LOWER, EXACT
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
        move_t move; // 4 bytes
        int16_t internal_value; // 2 bytes
        uint8_t depth; // 1 byte
        uint8_t bound; // 1 byte]

        int value(int ply) {
            if (internal_value >= MINCHECKMATE) {
                return internal_value - ply;
            } else if (internal_value <= -MINCHECKMATE) {
                return internal_value + ply;
            } else {
                return internal_value;
            }
        }
    };

    class hash_t {
    public:
        explicit hash_t(size_t size);

        ~hash_t();

        bool probe(U64 hash, entry_t &entry);
        void save(Bound bound, U64 hash, int depth, int ply, int eval, move_t move);
        size_t hash_full();
    private:
        const int BUCKET_SIZE = 4;

        size_t num_entries;
        entry_t *table;
    };
}

#endif //TOPPLE_HASH_H
