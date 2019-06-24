/*
  Copyright (c) 2013-2015 Ronald de Man
  This file may be redistributed and/or modified without restrictions.

  tbprobe.cpp contains the Stockfish-specific routines of the
  tablebase probing code. It should be relatively easy to adapt
  this code to other chess engines.
*/

// The probing code currently expects a little-endian architecture (e.g. x86).

// Define DECOMP64 when compiling for a 64-bit platform.
// 32-bit is only supported for 5-piece tables, because tables are mmap()ed
// into memory.
#ifdef IS_64BIT
#define DECOMP64
#endif

#include <vector>

#include "../bb.h"
#include "../hash.h"
#include "../movegen.h"

#include "tbprobe.h"
#include "tbcore.h"

int TBlargest = 0;

#include "tbcore.cpp"
#include "../move.h"

// Given a position with 6 or fewer pieces, produce a text string
// of the form KQPvKRP, where "KQP" represents the white pieces if
// mirror == 0 and the black pieces if mirror == 1.
// No need to make this very efficient.
static void prt_str(board_t& pos, char *str, int mirror)
{
    Team color;
    int pt;
    int i;

    color = !mirror ? WHITE : BLACK;
    for (pt = KING; pt >= PAWN; pt--)
        for (i = pos.record[pos.now].material.info.count(color, Piece(pt)); i > 0; i--)
            *str++ = pchr[5 - pt];
    *str++ = 'v';
    color = Team(!color);
    for (pt = KING; pt >= PAWN; pt--)
        for (i = pos.record[pos.now].material.info.count(color, Piece(pt)); i > 0; i--)
            *str++ = pchr[5 - pt];
    *str++ = 0;
}

// Given a position, produce a 64-bit material signature key.
// If the engine supports such a key, it should equal the engine's key.
// Again no need to make this very efficient.
static uint64 calc_key(board_t& pos, int mirror)
{
    Team color;
    int pt;
    int i;
    uint64 key = 0;

    color = !mirror ? WHITE : BLACK;
    for (pt = PAWN; pt <= KING; pt++)
        for (i = pos.record[pos.now].material.info.count(color, Piece(pt)); i > 0; i--)
            key ^= zobrist::squares[i - 1][WHITE][pt];
    color = Team(!color);
    for (pt = PAWN; pt <= KING; pt++)
        for (i = pos.record[pos.now].material.info.count(color, Piece(pt)); i > 0; i--)
            key ^= zobrist::squares[i - 1][BLACK][pt];

    return key;
}

// Produce a 64-bit material key corresponding to the material combination
// defined by pcs[16], where pcs[1], ..., pcs[6] is the number of white
// pawns, ..., kings and pcs[9], ..., pcs[14] is the number of black
// pawns, ..., kings.
// Again no need to be efficient here.
static uint64 calc_key_from_pcs(int *pcs, int mirror)
{
    int color;
    int pt;
    int i;
    uint64 key = 0;

    color = !mirror ? 0 : 8;
    for (pt = PAWN; pt <= KING; pt++)
        for (i = 0; i < pcs[color + pt + 1]; i++)
            key ^= zobrist::squares[i][WHITE][pt];
    color ^= 8;
    for (pt = PAWN; pt <= KING; pt++)
        for (i = 0; i < pcs[color + pt + 1]; i++)
            key ^= zobrist::squares[i][BLACK][pt];

    return key;
}

// probe_wdl_table and probe_dtz_table require similar adaptations.
static int probe_wdl_table(board_t& pos, int *success)
{
    struct TBEntry *ptr;
    struct TBHashEntry *ptr2;
    uint64 idx;
    uint64 key;
    int i;
    ubyte res;
    int p[TBPIECES];

    // Obtain the position's material signature key.
    //key = pos.material_key();
    key = calc_key(pos, false);

    // Test for KvK.
    if (key == (zobrist::squares[0][WHITE][KING] ^ zobrist::squares[0][BLACK][KING]))
        return 0;

    ptr2 = TB_hash[key >> (64 - TBHASHBITS)];
    for (i = 0; i < HSHMAX; i++)
        if (ptr2[i].key == key) break;
    if (i == HSHMAX) {
        *success = 0;
        return 0;
    }

    ptr = ptr2[i].ptr;
    if (!ptr->ready) {
        LOCK(TB_mutex);
        if (!ptr->ready) {
            char str[16];
            prt_str(pos, str, ptr->key != key);

            if (!init_table_wdl(ptr, str)) {
                ptr2[i].key = 0ULL;
                *success = 0;
                UNLOCK(TB_mutex);
                return 0;
            }
            // Memory barrier to ensure ptr->ready = 1 is not reordered.
            __asm__ __volatile__ ("" ::: "memory");
            ptr->ready = 1;
        }
        UNLOCK(TB_mutex);
    }

    int bside, mirror, cmirror;
    if (!ptr->symmetric) {
        if (key != ptr->key) {
            cmirror = 8;
            mirror = 0x38;
            bside = (pos.record[pos.now].next_move == WHITE);
        } else {
            cmirror = mirror = 0;
            bside = !(pos.record[pos.now].next_move == WHITE);
        }
    } else {
        cmirror = pos.record[pos.now].next_move == WHITE ? 0 : 8;
        mirror = pos.record[pos.now].next_move == WHITE ? 0 : 0x38;
        bside = 0;
    }

    // p[i] is to contain the square 0-63 (A1-H8) for a piece of type
    // pc[i] ^ cmirror, where 1 = white pawn, ..., 14 = black king.
    // Pieces of the same type are guaranteed to be consecutive.
    if (!ptr->has_pawns) {
        struct TBEntry_piece *entry = (struct TBEntry_piece *)ptr;
        ubyte *pc = entry->pieces[bside];
        for (i = 0; i < entry->num;) {
            U64 bb = pos.bb_pieces[(Team)((pc[i] ^ cmirror) >> 3)]
                                     [(Piece)(pc[i] & 0x07) - 1];
            do {
                p[i++] = pop_bit(bb);
            } while (bb);
        }
        idx = encode_piece(entry, entry->norm[bside], p, entry->factor[bside]);
        res = decompress_pairs(entry->precomp[bside], idx);
    } else {
        struct TBEntry_pawn *entry = (struct TBEntry_pawn *)ptr;
        int k = entry->file[0].pieces[0][0] ^ cmirror;
        U64 bb = pos.bb_pieces[(Team)(k >> 3)][(Piece)(k & 0x07) - 1];
        i = 0;
        do {
            p[i++] = pop_bit(bb) ^ mirror;
        } while (bb);
        int f = pawn_file(entry, p);
        ubyte *pc = entry->file[f].pieces[bside];
        for (; i < entry->num;) {
            bb = pos.bb_pieces[(Team)((pc[i] ^ cmirror) >> 3)]
                            [(Piece)(pc[i] & 0x07) - 1];
            do {
                p[i++] = pop_bit(bb) ^ mirror;
            } while (bb);
        }
        idx = encode_pawn(entry, entry->file[f].norm[bside], p, entry->file[f].factor[bside]);
        res = decompress_pairs(entry->file[f].precomp[bside], idx);
    }

    return ((int)res) - 2;
}

// The value of wdl MUST correspond to the WDL value of the position without
// en passant rights.
static int probe_dtz_table(board_t& pos, int wdl, int *success)
{
    struct TBEntry *ptr;
    uint64 idx;
    int i, res;
    int p[TBPIECES];

    // Obtain the position's material signature key.
    //uint64 key = pos.material_key();
    uint64 key = calc_key(pos, false);

    if (DTZ_table[0].key1 != key && DTZ_table[0].key2 != key) {
        for (i = 1; i < DTZ_ENTRIES; i++)
            if (DTZ_table[i].key1 == key || DTZ_table[i].key2 == key) break;
        if (i < DTZ_ENTRIES) {
            struct DTZTableEntry table_entry = DTZ_table[i];
            for (; i > 0; i--)
                DTZ_table[i] = DTZ_table[i - 1];
            DTZ_table[0] = table_entry;
        } else {
            struct TBHashEntry *ptr2 = TB_hash[key >> (64 - TBHASHBITS)];
            for (i = 0; i < HSHMAX; i++)
                if (ptr2[i].key == key) break;
            if (i == HSHMAX) {
                *success = 0;
                return 0;
            }
            ptr = ptr2[i].ptr;
            char str[16];
            int mirror = (ptr->key != key);
            prt_str(pos, str, mirror);
            if (DTZ_table[DTZ_ENTRIES - 1].entry)
                free_dtz_entry(DTZ_table[DTZ_ENTRIES-1].entry);
            for (i = DTZ_ENTRIES - 1; i > 0; i--)
                DTZ_table[i] = DTZ_table[i - 1];
            load_dtz_table(str, calc_key(pos, mirror), calc_key(pos, !mirror));
        }
    }

    ptr = DTZ_table[0].entry;
    if (!ptr) {
        *success = 0;
        return 0;
    }

    int bside, mirror, cmirror;
    if (!ptr->symmetric) {
        if (key != ptr->key) {
            cmirror = 8;
            mirror = 0x38;
            bside = (pos.record[pos.now].next_move == WHITE);
        } else {
            cmirror = mirror = 0;
            bside = !(pos.record[pos.now].next_move == WHITE);
        }
    } else {
        cmirror = pos.record[pos.now].next_move == WHITE ? 0 : 8;
        mirror = pos.record[pos.now].next_move == WHITE ? 0 : 0x38;
        bside = 0;
    }

    if (!ptr->has_pawns) {
        struct DTZEntry_piece *entry = (struct DTZEntry_piece *)ptr;
        if ((entry->flags & 1) != bside && !entry->symmetric) {
            *success = -1;
            return 0;
        }
        ubyte *pc = entry->pieces;
        for (i = 0; i < entry->num;) {
            U64 bb = pos.bb_pieces[(Team)((pc[i] ^ cmirror) >> 3)]
                                     [(Piece)(pc[i] & 0x07) - 1];
            do {
                p[i++] = pop_bit(bb);
            } while (bb);
        }
        idx = encode_piece((struct TBEntry_piece *)entry, entry->norm, p, entry->factor);
        res = decompress_pairs(entry->precomp, idx);

        if (entry->flags & 2)
            res = entry->map[entry->map_idx[wdl_to_map[wdl + 2]] + res];

        if (!(entry->flags & pa_flags[wdl + 2]) || (wdl & 1))
            res *= 2;
    } else {
        struct DTZEntry_pawn *entry = (struct DTZEntry_pawn *)ptr;
        int k = entry->file[0].pieces[0] ^ cmirror;
        U64 bb = pos.bb_pieces[(Team)(k >> 3)][(Piece)(k & 0x07) - 1];
        i = 0;
        do {
            p[i++] = pop_bit(bb) ^ mirror;
        } while (bb);
        int f = pawn_file((struct TBEntry_pawn *)entry, p);
        if ((entry->flags[f] & 1) != bside) {
            *success = -1;
            return 0;
        }
        ubyte *pc = entry->file[f].pieces;
        for (; i < entry->num;) {
            bb = pos.bb_pieces[(Team)((pc[i] ^ cmirror) >> 3)]
                            [(Piece)(pc[i] & 0x07) - 1];
            do {
                p[i++] = pop_bit(bb) ^ mirror;
            } while (bb);
        }
        idx = encode_pawn((struct TBEntry_pawn *)entry, entry->file[f].norm, p, entry->file[f].factor);
        res = decompress_pairs(entry->file[f].precomp, idx);

        if (entry->flags[f] & 2)
            res = entry->map[entry->map_idx[f][wdl_to_map[wdl + 2]] + res];

        if (!(entry->flags[f] & pa_flags[wdl + 2]) || (wdl & 1))
            res *= 2;
    }

    return res;
}

// Add underpromotion captures to list of captures.
/*
static ExtMove *add_underprom_caps(board_t& pos, MoveStack *stack, MoveStack *end)
{
    ExtMove *moves, *extra = end;

    for (moves = stack; moves < end; moves++) {
        Move move = moves->move;
        if (type_of(move) == PROMOTION && !pos.empty(to_sq(move))) {
            (*extra++).move = (Move)(move - (1 << 12));
            (*extra++).move = (Move)(move - (2 << 12));
            (*extra++).move = (Move)(move - (3 << 12));
        }
    }

    return extra;
}
*/

static int probe_ab(board_t& pos, int alpha, int beta, int *success)
{
    int v;
    move_t stack[64];
    move_t *moves, *end;

    // Generate (at least) all legal captures including (under)promotions.
    // It is OK to generate more, as long as they are filtered out below.
    movegen_t gen(pos);
    end = stack + gen.gen_noisy(stack);

    for (moves = stack; moves < end; moves++) {
        move_t capture = *moves;
        if (!capture.info.is_capture || !pos.is_legal(capture))
            continue;
        pos.move(capture);
        v = -probe_ab(pos, -beta, -alpha, success);
        pos.unmove();
        if (*success == 0) return 0;
        if (v > alpha) {
            if (v >= beta)
                return v;
            alpha = v;
        }
    }

    v = probe_wdl_table(pos, success);

    return alpha >= v ? alpha : v;
}

// Probe the WDL table for a particular position.
//
// If *success != 0, the probe was successful.
//
// If *success == 2, the position has a winning capture, or the position
// is a cursed win and has a cursed winning capture, or the position
// has an ep capture as only best move.
// This is used in probe_dtz().
//
// The return value is from the point of view of the side to move:
// -2 : loss
// -1 : loss, but draw under 50-move rule
//  0 : draw
//  1 : win, but draw under 50-move rule
//  2 : win
int probe_wdl(board_t& pos, int *success)
{
    *success = 1;

    // Generate (at least) all legal en passant captures.
    move_t stack[192];
    move_t *moves, *end;

    // Generate (at least) all legal captures including (under)promotions.
    movegen_t gen(pos);
    end = stack + gen.gen_noisy(stack);

    int best_cap = -3, best_ep = -3;

    // We do capture resolution, letting best_cap keep track of the best
    // capture without ep rights and letting best_ep keep track of still
    // better ep captures if they exist.

    for (moves = stack; moves < end; moves++) {
        move_t capture = *moves;
        if (!capture.info.is_capture || !pos.is_legal(capture))
            continue;
        pos.move(capture);
        int v = -probe_ab(pos, -2, -best_cap, success);
        pos.unmove();
        if (*success == 0) return 0;
        if (v > best_cap) {
            if (v == 2) {
                *success = 2;
                return 2;
            }
            if (!capture.info.is_ep)
                best_cap = v;
            else if (v > best_ep)
                best_ep = v;
        }
    }

    int v = probe_wdl_table(pos, success);
    if (*success == 0) return 0;

    // Now max(v, best_cap) is the WDL value of the position without ep rights.
    // If the position without ep rights is not stalemate or no ep captures
    // exist, then the value of the position is max(v, best_cap, best_ep).
    // If the position without ep rights is stalemate and best_ep > -3,
    // then the value of the position is best_ep (and we will have v == 0).

    if (best_ep > best_cap) {
        if (best_ep > v) { // ep capture (possibly cursed losing) is best.
            *success = 2;
            return best_ep;
        }
        best_cap = best_ep;
    }

    // Now max(v, best_cap) is the WDL value of the position unless
    // the position without ep rights is stalemate and best_ep > -3.

    if (best_cap >= v) {
        // No need to test for the stalemate case here: either there are
        // non-ep captures, or best_cap == best_ep >= v anyway.
        *success = 1 + (best_cap > 0);
        return best_cap;
    }

    // Now handle the stalemate case.
    if (best_ep > -3 && v == 0) {
        // Check for stalemate in the position with ep captures.
        for (moves = stack; moves < end; moves++) {
            move_t move = *moves;
            if (move.info.is_ep) continue;
            if (pos.is_legal(move)) break;
        }
        if (moves == end && !pos.is_incheck()) {
            end = end + gen.gen_quiets(end);
            for (; moves < end; moves++) {
                move_t move = *moves;
                if (pos.is_legal(move))
                    break;
            }
        }
        if (moves == end) { // Stalemate detected.
            *success = 2;
            return best_ep;
        }
    }

    // Stalemate / en passant not an issue, so v is the correct value.

    return v;
}

static int wdl_to_dtz[] = {
        -1, -101, 0, 101, 1
};

// Probe the DTZ table for a particular position.
// If *success != 0, the probe was successful.
// The return value is from the point of view of the side to move:
//         n < -100 : loss, but draw under 50-move rule
// -100 <= n < -1   : loss in n ply (assuming 50-move counter == 0)
//         0	    : draw
//     1 < n <= 100 : win in n ply (assuming 50-move counter == 0)
//   100 < n        : win, but draw under 50-move rule
//
// If the position is mate, -1 is returned instead of 0.
//
// The return value n can be off by 1: a return value -n can mean a loss
// in n+1 ply and a return value +n can mean a win in n+1 ply. This
// cannot happen for tables with positions exactly on the "edge" of
// the 50-move rule.
//
// This means that if dtz > 0 is returned, the position is certainly
// a win if dtz + 50-move-counter <= 99. Care must be taken that the engine
// picks moves that preserve dtz + 50-move-counter <= 99.
//
// If n = 100 immediately after a capture or pawn move, then the position
// is also certainly a win, and during the whole phase until the next
// capture or pawn move, the inequality to be preserved is
// dtz + 50-movecounter <= 100.
//
// In short, if a move is available resulting in dtz + 50-move-counter <= 99,
// then do not accept moves leading to dtz + 50-move-counter == 100.
//
int probe_dtz(board_t& pos, int *success)
{
    int wdl = probe_wdl(pos, success);
    if (*success == 0) return 0;

    // If draw, then dtz = 0.
    if (wdl == 0) return 0;

    // Check for winning (cursed) capture or ep capture as only best move.
    if (*success == 2)
        return wdl_to_dtz[wdl + 2];

    move_t stack[192];
    move_t *moves, *end = nullptr;

    // If winning, check for a winning pawn move.
    if (wdl > 0) {
        // Generate at least all legal non-capturing pawn moves
        // including non-capturing promotions.
        // (The call to generate<>() in fact generates all moves.)
        movegen_t gen(pos);
        end = stack + gen.gen_quiets(stack);

        for (moves = stack; moves < end; moves++) {
            move_t move = *moves;
            if (move.info.piece != PAWN || move.info.is_capture
                || !pos.is_legal(move))
                continue;
            pos.move(move);
            int v = -probe_wdl(pos, success);
            pos.unmove();
            if (*success == 0) return 0;
            if (v == wdl)
                return wdl_to_dtz[wdl + 2];
        }
    }

    // If we are here, we know that the best move is not an ep capture.
    // In other words, the value of wdl corresponds to the WDL value of
    // the position without ep rights. It is therefore safe to probe the
    // DTZ table with the current value of wdl.

    int dtz = probe_dtz_table(pos, wdl, success);
    if (*success >= 0)
        return wdl_to_dtz[wdl + 2] + ((wdl > 0) ? dtz : -dtz);

    // *success < 0 means we need to probe DTZ for the other side to move.
    int best;
    if (wdl > 0) {
        best = INT32_MAX;
        // If wdl > 0, we already generated all moves.
    } else {
        // If (cursed) loss, the worst case is a losing capture or pawn move
        // as the "best" move, leading to dtz of -1 or -101.
        // In case of mate, this will cause -1 to be returned.
        best = wdl_to_dtz[wdl + 2];
        movegen_t gen(pos);
        end = stack + gen.gen_quiets(stack);
    }

    for (moves = stack; moves < end; moves++) {
        move_t move = *moves;
        // We can skip pawn moves and captures.
        // If wdl > 0, we already caught them. If wdl < 0, the initial value
        // of best already takes account of them.
        if (move.info.is_capture || move.info.piece == PAWN
            || !pos.is_legal(move))
            continue;
        pos.move(move);
        int v = -probe_dtz(pos, success);
        pos.unmove();
        if (*success == 0) return 0;
        if (wdl > 0) {
            if (v > 0 && v + 1 < best)
                best = v + 1;
        } else {
            if (v -1 < best)
                best = v - 1;
        }
    }
    return best;
}

// Check whether there has been at least one repetition of positions
// since the last capture or pawn move.
/*
static int has_repeated(StateInfo *st)
{
    while (1) {
        int i = 4, e = std::min(st->rule50, st->pliesFromNull);
        if (e < i)
            return 0;
        StateInfo *stp = st->previous->previous;
        do {
            stp = stp->previous->previous;
            if (stp->key == st->key)
                return 1;
            i += 2;
        } while (i <= e);
        st = st->previous;
    }
}
*/

static int wdl_to_value[5] = {
        -INF + MAX_PLY + 1,
        0 - 2,
        0,
        0 + 2,
        INF - MAX_PLY - 1
};

// Use the DTZ tables to filter out moves that don't preserve the win or draw.
// If the position is lost, but DTZ is fairly high, only keep moves that
// maximise DTZ.
//
// Returns a vector of root moves, or an empty vector if no moves were filtered out.
std::vector<move_t> root_probe(board_t& pos, int &wdl)
{
    move_t buf[256];
    movegen_t gen(pos);

    std::vector<move_t> root_moves(buf, buf + gen.gen_normal(buf));
    std::vector<int> root_scores(root_moves.size(), 0);

    int success;

    int dtz = probe_dtz(pos, &success);
    if (!success) return {};

    // Probe each move.
    for (size_t i = 0; i < root_moves.size(); i++) {
        move_t move = root_moves[i];
        if(!pos.is_legal(move)) continue;
        pos.move(move);
        int v = 0;
        if (pos.is_incheck() && dtz > 0) {
            move_t s[192];
            movegen_t checkmate_gen(pos);
            int n_generated = checkmate_gen.gen_normal(s);
            bool found_legal = false;
            for(int j = 0; j < n_generated; j++) {
                if(pos.is_legal(s[j])){
                    found_legal = true;
                    break;
                }
            }
            if (!found_legal)
                v = 1;
        }
        if (!v) {
            if (pos.record[pos.now].halfmove_clock != 0) {
                v = -probe_dtz(pos, &success);
                if (v > 0) v++;
                else if (v < 0) v--;
            } else {
                v = -probe_wdl(pos, &success);
                v = wdl_to_dtz[v + 2];
            }
        }
        pos.unmove();
        if (!success) return {};
        root_scores[i] = v;
    }

    // Obtain 50-move counter for the root position.
    int cnt50 = pos.record[pos.now].halfmove_clock;

    // Use 50-move counter to determine whether the root position is
    // won, lost or drawn.
    wdl = 0;
    if (dtz > 0)
        wdl = (dtz + cnt50 <= 100) ? 2 : 1;
    else if (dtz < 0)
        wdl = (-dtz + cnt50 <= 100) ? -2 : -1;

    // Now be a bit smart about filtering out moves.
    size_t j = 0;
    if (dtz > 0) { // winning (or 50-move rule draw)
        int best = 0xffff;
        for (size_t i = 0; i < root_moves.size(); i++) {
            int v = root_scores[i];
            if (v > 0 && v < best)
                best = v;
        }
        int max = best;
        // If the current phase has not seen repetitions, then try all moves
        // that stay safely within the 50-move budget, if there are any.
        if (!pos.is_repetition_draw(100) && best + cnt50 <= 99)
            max = 99 - cnt50;
        for (size_t i = 0; i < root_moves.size(); i++) {
            int v = root_scores[i];
            if (v > 0 && v <= max) {
                root_moves[j] = root_moves[i];
                root_scores[j] = root_scores[i];
                j++;
            }
        }
    } else if (dtz < 0) {
        int best = 0;
        for (size_t i = 0; i < root_moves.size(); i++) {
            int v = root_scores[i];
            if (v < best)
                best = v;
        }
        // Try all moves, unless we approach or have a 50-move rule draw.
        if (-best * 2 + cnt50 < 100)
            return root_moves;
        for (size_t i = 0; i < root_moves.size(); i++) {
            if (root_scores[i] == best) {
                root_moves[j] = root_moves[i];
                root_scores[j] = root_scores[i];
                j++;
            }
        }
    } else { // drawing
        // Try all moves that preserve the draw.
        for (size_t i = 0; i < root_moves.size(); i++) {
            if (root_scores[i] == 0) {
                root_moves[j] = root_moves[i];
                root_scores[j] = root_scores[i];
                j++;
            }
        }
    }

    root_moves.resize(j);

    return root_moves;
}

// Use the WDL tables to filter out moves that don't preserve the win or draw.
// This is a fallback for the case that some or all DTZ tables are missing.
//
// A return value of 0 indicates that not all probes were successful and that
// no moves were filtered out.
std::vector<move_t> root_probe_wdl(board_t& pos, int& wdl)
{
    int success;

    move_t buf[256];
    movegen_t gen(pos);

    std::vector<move_t> root_moves(buf, buf + gen.gen_normal(buf));
    std::vector<int> root_scores(root_moves.size(), 0);

    wdl = probe_wdl(pos, &success);
    if (!success) return {};

    int best = -2;

    // Probe each move.
    for (size_t i = 0; i < root_moves.size(); i++) {
        move_t move = root_moves[i];
        if(!pos.is_legal(move)) continue;
        pos.move(move);
        int v = -probe_wdl(pos, &success);
        pos.unmove();
        if (!success) return {};
        root_scores[i] = v;
        if (v > best)
            best = v;
    }

    size_t j = 0;
    for (size_t i = 0; i < root_moves.size(); i++) {
        if (root_scores[i] == best) {
            root_moves[j] = root_moves[i];
            root_scores[j] = root_scores[i];
            j++;
        }
    }

    root_moves.resize(j);

    return root_moves;
}
