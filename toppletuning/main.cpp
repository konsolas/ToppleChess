//
// Created by Vincent on 23/05/2020.
//

#include "toppletuner.h"
#include "../syzygy/tbprobe.h"

void print_params(const eval_params_t &current_params);

int main(int argc, char *argv[]) {
    // Initialise engine
    init_tables();
    zobrist::init_hashes();
    evaluator_t::eval_init();

    std::cout << "total parameters: " << sizeof(eval_params_t) / sizeof(int) << std::endl;

    if (argc > 1) {
        init_tablebases(argv[1]);
    }

    std::vector<board_t> openings{board_t{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"}};
    tuner_params_t params{100, 0.5, 0.05, 5, 10};
    search_limits_t limits{1, MAX_PLY, UINT64_MAX, {}};
    limits.syzygy_resolve = 0;

    topple_tuner_t tuner(params, limits, openings);

    tuner.init_population();

    print_params(eval_params_t());

    for(size_t gen = 0; gen < 10000000; gen++) {
        eval_params_t eval_params = tuner.evolve(7);
        std::cout << "generation " << gen << ": " << std::endl;
        print_params(eval_params);
    }
}

void texel_t::print_params() {
    std::cout << "// Tapering parameters" << std::endl;

    std::cout << "int mat_exch_knight = " << current_params.mat_exch_knight << ";" << std::endl;
    std::cout << "int mat_exch_bishop = " << current_params.mat_exch_bishop << ";" << std::endl;
    std::cout << "int mat_exch_rook = " << current_params.mat_exch_rook << ";" << std::endl;
    std::cout << "int mat_exch_queen = " << current_params.mat_exch_queen << ";" << std::endl;

    std::cout << "int pt_blocked_file[4] = {";
    for (const auto &param : current_params.pt_blocked_file) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "int pt_open_file[4] = {";
    for (const auto &param : current_params.pt_open_file) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "int pt_island = " << current_params.pt_island << ";" << std::endl;
    std::cout << "int pt_max = " << current_params.pt_max << ";" << std::endl;

    std::cout << "// Piece-square tables" << std::endl;
    std::cout << "// 4x4 tables include the following squares, mirrored horizontally and vertically" << std::endl;
    std::cout << "// A4, B4, C4, D4," << std::endl;
    std::cout << "// A3, B3, C3, D3," << std::endl;
    std::cout << "// A2, B2, C2, D2," << std::endl;
    std::cout << "// A1, B1, C1, D1," << std::endl;
    std::cout << "// Pawn tables are mirrored vertically only" << std::endl;

    std::cout << "v4si_t n_pst[16] = {";
    for (const auto &param : current_params.n_pst) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t q_pst[16] = {";
    for (const auto &param : current_params.q_pst) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t b_pst[16] = {";
    for (const auto &param : current_params.b_pst) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;


    std::cout << "v4si_t r_pst[16] = {";
    for (const auto &param : current_params.r_pst) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t p_pst[24] = {";
    for (const auto &param : current_params.p_pst) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t k_pst[16] = {";
    for (const auto &param : current_params.k_pst) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "// Pawn structure" << std::endl;

    std::cout << "v4si_t isolated[2] = {";
    for (const auto &param : current_params.isolated) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t backwards[2] = {";
    for (const auto &param : current_params.backwards) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t semi_backwards[2] = {";
    for (const auto &param : current_params.semi_backwards) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t paired[2] = {";
    for (const auto &param : current_params.paired) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t detached[2] = {";
    for (const auto &param : current_params.detached) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t doubled[2] = {";
    for (const auto &param : current_params.doubled) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t chain[5] = {";
    for (const auto &param : current_params.chain) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t passed[6] = {";
    for (const auto &param : current_params.passed) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t candidate[4] = {";
    for (const auto &param : current_params.candidate) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "// Interaction of pieces and pawn structure" << std::endl;

    std::cout << "v4si_t king_tropism[2] = {";
    for (const auto &param : current_params.king_tropism) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t passer_tropism[2] = {";
    for (const auto &param : current_params.passer_tropism) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t blocked[2] = {";
    for (const auto &param : current_params.blocked) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t pos_r_open_file = " << current_params.pos_r_open_file << ";" << std::endl;
    std::cout << "v4si_t pos_r_own_half_open_file = " << current_params.pos_r_own_half_open_file << ";" << std::endl;
    std::cout << "v4si_t pos_r_other_half_open_file = " << current_params.pos_r_other_half_open_file << ";" << std::endl;

    std::cout << "v4si_t outpost[2] = {";
    for (auto param : current_params.outpost) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t outpost_hole[2] = {";
    for (auto param : current_params.outpost_hole) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t outpost_half[2] = {";
    for (auto param : current_params.outpost_half) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t ks_pawn_shield[4] = {";
    for (const auto &param : current_params.ks_pawn_shield) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "// King safety" << std::endl;

    std::cout << "int kat_zero = " << current_params.kat_zero << ";" << std::endl;
    std::cout << "int kat_open_file = " << current_params.kat_open_file << ";" << std::endl;
    std::cout << "int kat_own_half_open_file = " << current_params.kat_own_half_open_file << ";" << std::endl;
    std::cout << "int kat_other_half_open_file = " << current_params.kat_other_half_open_file << ";" << std::endl;

    std::cout << "int kat_attack_weight[5] = {";
    for (const auto &param : current_params.kat_attack_weight) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "int kat_defence_weight[5] = {";
    for (const auto &param : current_params.kat_defence_weight) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "int kat_table_scale = " << current_params.kat_table_scale << ";" << std::endl;
    std::cout << "int kat_table_translate = " << current_params.kat_table_translate << ";" << std::endl;
    std::cout << "v4si_t kat_table_max = " << current_params.kat_table_max << ";" << std::endl;

    std::cout << "// Dynamic threats" << std::endl;

    std::cout << "v4si_t undefended[5] = {";
    for (const auto &param : current_params.undefended) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t overprotected[5] = {";
    for (const auto &param : current_params.overprotected) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t threat_matrix[4][5] = {" << std::endl;
    for(auto &param : current_params.threat_matrix) {
        std::cout << "  {";
        for(auto param2 : param) {
            std::cout << param2 << ",";
        }
        std::cout << "}, " << std::endl;
    }
    std::cout << "};" << std::endl;

    std::cout << "// Other positional factors" << std::endl;

    std::cout << "v4si_t pos_bishop_pair " << current_params.pos_bishop_pair << ";" << std::endl;

    std::cout << "v4si_t mat_opp_bishop[3] = {";
    for (const auto &param : current_params.mat_opp_bishop) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;

    std::cout << "v4si_t pos_r_trapped = " << current_params.pos_r_trapped << ";" << std::endl;
    std::cout << "v4si_t pos_r_behind_own_passer = " << current_params.pos_r_behind_own_passer << ";" << std::endl;
    std::cout << "v4si_t pos_r_behind_enemy_passer = " << current_params.pos_r_behind_enemy_passer << ";" << std::endl;

    std::cout << "v4si_t pos_mob[4] = {";
    for (const auto &param : current_params.pos_mob) {
        std::cout << param << ",";
    }
    std::cout << "};" << std::endl;
}
