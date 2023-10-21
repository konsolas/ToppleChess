//
// Created by Vincent on 30/09/2017.
//

#ifndef TOPPLE_EVAL_H
#define TOPPLE_EVAL_H

#include <vector>

#include "types.h"
#include "board.h"
#include "pawns.h"

// ql: 0.0612082
struct eval_params_t {
    // Tapering parameters
    int mat_exch_knight = 19;
    int mat_exch_bishop = 35;
    int mat_exch_rook = 69;
    int mat_exch_queen = 141;
    int pt_blocked_file[4] = {64,82,78,81,};
    int pt_closed_file[4] = {33,33,51,46,};
    int pt_max = 480;

    // Piece-square tables
    // 4x4 tables include the following squares, mirrored horizontally and vertically
    // A4, B4, C4, D4,
    // A3, B3, C3, D3,
    // A2, B2, C2, D2,
    // A1, B1, C1, D1,
    // Pawn tables are mirrored vertically only
    v4si_t n_pst[16] = {{408,398,554,306},{403,430,493,329},{420,419,480,353},{424,418,462,370},{413,386,519,296},{426,402,483,323},{428,420,427,353},{406,436,471,354},{415,356,474,295},{412,375,515,305},{410,410,489,324},{435,410,456,342},{212,294,486,252},{405,367,531,296},{387,361,522,310},{386,377,527,321},};
    v4si_t q_pst[16] = {{1291,1326,1345,1207},{1263,1341,1429,1214},{1301,1301,1320,1256},{1288,1294,1365,1286},{1287,1333,1381,1177},{1320,1328,1296,1218},{1317,1320,1352,1240},{1311,1308,1313,1259},{1321,1314,1314,1226},{1250,1344,1407,1188},{1318,1325,1265,1252},{1332,1327,1272,1254},{1294,1326,1363,1194},{1343,1285,1336,1221},{1326,1312,1272,1230},{1334,1337,1234,1233},};
    v4si_t b_pst[16] = {{361,428,493,346},{413,399,461,369},{401,420,445,380},{426,419,425,386},{422,446,482,340},{423,446,434,363},{421,460,450,370},{420,429,462,381},{400,406,445,346},{425,470,442,354},{430,425,457,359},{426,435,450,374},{401,404,467,345},{424,384,444,355},{421,399,488,358},{406,400,522,353},};
    v4si_t r_pst[16] = {{581,560,781,652},{567,569,772,654},{575,564,777,664},{588,567,774,660},{572,564,790,653},{595,557,789,659},{609,549,769,663},{587,580,772,655},{562,557,778,664},{545,587,817,660},{571,571,795,667},{584,585,772,660},{610,572,748,667},{609,567,759,665},{602,601,752,663},{606,610,719,666},};
    v4si_t p_pst[24] = {{195,186,202,176},{73,212,189,173},{128,165,113,204},{152,155,218,166},{74,189,159,118},{151,196,112,114},{134,165,135,123},{131,165,98,114},{117,108,116,110},{110,114,120,110},{83,123,127,105},{107,115,116,101},{109,109,117,100},{98,98,114,102},{115,97,103,103},{93,125,111,85},{91,98,110,97},{91,72,118,104},{109,93,102,98},{99,81,113,91},{92,100,119,97},{105,107,118,100},{109,111,103,96},{118,92,95,78},};
    v4si_t k_pst[16] = {{-205,-38,59,-7},{-269,-25,95,-4},{-319,-42,94,-3},{-275,-65,103,-11},{3,-33,0,5},{-121,-13,34,7},{-148,-74,43,10},{-305,-84,63,4},{41,51,-42,-3},{-20,-11,-3,17},{-112,-91,23,28},{-136,-122,20,27},{81,84,-102,-23},{48,56,-8,-11},{13,-24,4,13},{-10,-29,-17,7},};

    // Pawn structure
    v4si_t isolated[2] = {{-7,-4,6,5},{-16,-15,5,-2},};
    v4si_t backwards[2] = {{-7,1,-11,3},{-25,0,-48,-11},};
    v4si_t semi_backwards[2] = {{1,3,-4,8},{-25,-8,-15,2},};
    v4si_t paired[2] = {{9,17,6,-2},{2,6,13,-7},};
    v4si_t detached[2] = {{-3,-7,-8,-6},{-6,-1,7,-9},};
    v4si_t doubled[2] = {{5,-5,-22,-24},{9,24,-12,-37},};
    v4si_t chain[5] = {{16,26,25,12},{15,15,27,2},{28,8,29,18},{-57,121,124,-10},{6,497,229,-138},};
    v4si_t passed[6] = {{-26,-2,-111,-54},{13,-11,-72,-33},{29,-44,-22,5},{35,-29,35,39},{-14,-53,79,122},{-121,15,157,126},};
    v4si_t candidate[4] = {{-18,-9,-24,4},{-2,2,-23,1},{25,-3,-1,15},{111,-24,99,54},};

    // Interaction of pieces and pawn structure
    v4si_t king_tropism[2] = {{-5,-12,2,-1},{3,2,9,5},};
    v4si_t passer_tropism[2] = {{10,1,-11,-10},{0,-1,25,18},};
    v4si_t blocked[2] = {{-28,1,-1,-8},{-3,2,-11,-46},};
    v4si_t pos_r_open_file = {31,42,64,-26};
    v4si_t pos_r_own_half_open_file = {25,20,-2,-11};
    v4si_t pos_r_other_half_open_file = {27,-1,69,-19};
    v4si_t outpost[2] = {{-2,14,14,-10},{-3,-9,-11,6},};
    v4si_t outpost_hole[2] = {{14,22,52,14},{5,82,-18,-2},};
    v4si_t outpost_half[2] = {{8,-8,35,-34},{24,13,26,-31},};
    v4si_t ks_pawn_shield[4] = {{55,-30,26,5},{47,-6,1,15},{28,16,-13,-2},{13,45,6,-4},};

    // King safety
    int kat_zero = 13;
    int kat_open_file = 33;
    int kat_own_half_open_file = 26;
    int kat_other_half_open_file = 12;
    int kat_attack_weight[5] = {7,12,9,7,9,};
    int kat_defence_weight[5] = {2,6,5,3,3,};
    int kat_table_scale = 47;
    int kat_table_translate = 77;
    v4si_t kat_table_max = {458,454,60,-198};

    // Dynamic threats
    v4si_t undefended[5] = {{-4,-16,-6,7},{-21,-18,40,-23},{-15,-5,17,-18},{2,-5,28,-19},{-13,-15,42,-19},};
    v4si_t overprotected[5] = {{4,6,-4,-1},{10,4,24,8},{4,4,24,0},{5,4,40,-6},{-8,7,46,-16},};
    v4si_t threat_matrix[4][5] = {
            {{-17,-14,44,-47},{41,26,16,-24},{84,27,32,26},{83,22,14,-62},{39,4,109,-59},},
            {{-5,-17,3,5},{-59,-2,-143,67},{23,13,111,10},{17,83,85,-26},{33,-6,48,-39},},
            {{6,-7,17,10},{17,18,56,13},{36,-179,-115,-108},{10,-19,23,-5},{50,1,41,-60},},
            {{5,-12,31,5},{16,11,12,12},{23,28,13,7},{307,-183,71,-180},{68,58,117,-156},},
    };

    // Other positional factors
    v4si_t pos_bishop_pair {41,48,61,63};
    v4si_t mat_opp_bishop[3] = {{28,-52,-46,122},{108,-66,-32,108},{46,-26,122,66},};
    v4si_t pos_r_trapped = {-39,-62,60,-53};
    v4si_t pos_r_behind_own_passer = {-25,33,-22,10};
    v4si_t pos_r_behind_enemy_passer = {139,-7,67,42};
    v4si_t pos_mob[4] = {{11,8,34,-6},{9,7,24,-4},{-1,6,22,-2},{5,5,23,-3},};
};

struct processed_params_t : public eval_params_t {
    explicit processed_params_t(const eval_params_t &params);

    // Expanded piece-square tables for fast lookup
    v4si_t pst[2][6][64] = {}; // [TEAM][PIECE][SQUARE][MG/EG]

    // Precomputed king danger scores for fast lookup
    v4si_t kat_table[128] = {};
};

class alignas(64) evaluator_t {
    pawns::structure_t *pawn_hash_table;
    size_t pawn_hash_entries;

    const processed_params_t &params;
public:
    /**
     * Construct a new evaluator with a pawn hash table using the given parameters and size in bytes.
     *
     * @param params evaluation parameters
     * @param pawn_hash_size size of the pawn hash table in bytes
     */
    evaluator_t(const processed_params_t &params, size_t pawn_hash_size);

    ~evaluator_t();

    // This object has a (potentially large) pawn hash table - don't copy
    evaluator_t(const evaluator_t &) = delete;

    /**
     * Evaluate a position, giving a result in centipawns.
     *
     * @param board position to evaluate
     * @return evaluation in centipawns.
     */
    int evaluate(const board_t &board);

    /**
     * Prefetch an entry in the pawn hash table
     *
     * @param pawn_hash hash of entry to prefetch
     */
    void prefetch(U64 pawn_hash);

    /**
     * Computes the game phase of the given position, used to taper evaluation.
     * Topple's search uses this value to guide time management.
     * Close to 1.0 in the middlegame, 0.0 in the endgame
     *
     * @param board position to check
     * @return tapering factor between 0 and 1
     */
    [[nodiscard]] float game_phase(const board_t &board) const; // returns tapering factor 0-1
private:
    struct eval_data_t {
        int king_pos[2];
        U64 king_circle[2];
        int king_danger[2];

        U64 team_attacks[2] = {};
        U64 attacks[2][6] = {};
        U64 double_attacks[2] = {};

        void update_attacks(Team team, Piece piece, U64 bb) {
            double_attacks[team] |= team_attacks[team] & bb;
            attacks[team][piece] |= bb;
            team_attacks[team] |= bb;
        }
    };

    v4si_t eval_pawns(const board_t &board, eval_data_t &data, float &taper);

    v4si_t eval_pieces(const board_t &board, eval_data_t &data);

    v4si_t eval_threats(const board_t &board, eval_data_t &data);

    v4si_t eval_positional(const board_t &board, eval_data_t &data);
};

#endif //TOPPLE_EVAL_H
