//
// Created by Vincent on 28/08/2021.
//

#ifndef TOPPLE_FATHOM_H
#define TOPPLE_FATHOM_H

#include <vector>
#include <optional>
#include <string>

#include "../move.h"

class board_t;

enum class WDL {
    LOSS = -2,
    BLESSED_LOSS = -1,
    DRAW = 0,
    CURSED_WIN = 1,
    WIN = 2,
    INVALID
};

/**
 * Get the size of the largest tablebase currently available
 */
unsigned tb_largest();

/**
 * Initialises tablebase support with the given path
 */
void init_tb(const std::string &path);

/**
 * Probe the DTZ table, returning a vector of candidate moves which preserve the WDL value.
 * An empty vector indicates failure. Not thread safe - only call at start of search.
 */
std::vector<move_t> probe_root(const board_t &board);

/**
 * Probe the WDL table, returning a WDL state. Thread safe, designed to be used during search.
 */
std::optional<WDL> probe_wdl(const board_t &board);

#endif //TOPPLE_FATHOM_H
