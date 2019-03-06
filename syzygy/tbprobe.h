//
// Created by Vincent on 01/03/2019.
//

#ifndef TOPPLE_TBPROBE_H
#define TOPPLE_TBPROBE_H

#include "../types.h"
#include "../board.h"

// Topple: always compile for 64-bit
#define DECOMP64

extern int TBlargest; // 5 if 5-piece tables, 6 if 6-piece tables were found.

void init_tablebases(const char *path);
int probe_wdl(board_t& pos, int *success);
int probe_dtz(board_t& pos, int *success);
std::vector<move_t> root_probe(board_t& pos, int &wdl);
std::vector<move_t> root_probe_wdl(board_t& pos, int &wdl);

#endif //TOPPLE_TBPROBE_H
