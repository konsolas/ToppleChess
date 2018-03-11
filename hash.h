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
    enum Bound : uint8_t {UPPER, LOWER, EXACT};

    struct entry_t { // 16 bytes
        U64 hash; // 8 bytes
        move_t move; // 4 bytes
        int16_t value; // 2 bytes
        uint8_t depth; // 1 byte
        uint8_t bound;
    };

    class hash_t {
    public:
        explicit hash_t(unsigned int size);

        ~hash_t();

        bool probe(U64 hash, entry_t &entry);
        void save(Bound bound, U64 hash, int depth, int eval, move_t move);
        Bound bound(int value, int beta);
    private:
        const int BUCKET_SIZE = 4;

        int num_entries;
        entry_t *table;
    };
}

#endif //TOPPLE_HASH_H
