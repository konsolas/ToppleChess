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


void print_params(const eval_params_t &current_params) {
    std::cout << "  mat_exch_knight " << current_params.mat_exch_knight << std::endl;
    std::cout << "  mat_exch_bishop " << current_params.mat_exch_bishop << std::endl;
    std::cout << "  mat_exch_rook " << current_params.mat_exch_rook << std::endl;
    std::cout << "  mat_exch_queen " << current_params.mat_exch_queen << std::endl;

    std::cout << "  n_pst_mg ";
    for (int param : current_params.n_pst_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  n_pst_eg ";
    for (int param : current_params.n_pst_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  q_pst_mg ";
    for (int param : current_params.q_pst_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  q_pst_eg ";
    for (int param : current_params.q_pst_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  b_pst_mg ";
    for (int param : current_params.b_pst_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  b_pst_eg ";
    for (int param : current_params.b_pst_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  r_pst_mg ";
    for (int param : current_params.r_pst_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  r_pst_eg ";
    for (int param : current_params.r_pst_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  p_pst_mg ";
    for (int param : current_params.p_pst_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  p_pst_eg ";
    for (int param : current_params.p_pst_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  k_pst_mg ";
    for (int param : current_params.k_pst_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  k_pst_eg ";
    for (int param : current_params.k_pst_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  isolated_mg ";
    for (int param : current_params.isolated_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  isolated_eg ";
    for (int param : current_params.isolated_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  backwards_mg ";
    for (int param : current_params.backwards_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  backward_eg ";
    for (int param : current_params.backwards_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  semi_backwards_mg ";
    for (int param : current_params.semi_backwards_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  semi_backward_eg ";
    for (int param : current_params.semi_backwards_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  paired_mg ";
    for (int param : current_params.paired_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  paired_eg ";
    for (int param : current_params.paired_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  detached_mg ";
    for (int param : current_params.detached_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  detached_eg ";
    for (int param : current_params.detached_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  doubled_mg ";
    for (int param : current_params.doubled_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  doubled_eg ";
    for (int param : current_params.doubled_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  chain_mg ";
    for (int param : current_params.chain_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  chain_eg ";
    for (int param : current_params.chain_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  passed_mg ";
    for (int param : current_params.passed_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  passed_eg ";
    for (int param : current_params.passed_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  candidate_mg ";
    for (int param : current_params.candidate_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  candidate_eg ";
    for (int param : current_params.candidate_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  king_tropism_eg ";
    for (int param : current_params.king_tropism_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  passer_tropism_eg ";
    for (int param : current_params.passer_tropism_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  blocked_mg ";
    for (int param : current_params.blocked_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  blocked_eg ";
    for (int param : current_params.blocked_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  pos_r_open_file_mg " << current_params.pos_r_open_file_mg << std::endl;
    std::cout << "  pos_r_open_file_eg " << current_params.pos_r_open_file_eg << std::endl;
    std::cout << "  pos_r_own_half_open_file_mg " << current_params.pos_r_own_half_open_file_mg << std::endl;
    std::cout << "  pos_r_own_half_open_file_eg " << current_params.pos_r_own_half_open_file_eg << std::endl;
    std::cout << "  pos_r_other_half_open_file_mg " << current_params.pos_r_other_half_open_file_mg << std::endl;
    std::cout << "  pos_r_other_half_open_file_eg " << current_params.pos_r_other_half_open_file_eg << std::endl;

    std::cout << "  outpost_mg ";
    for (auto param : current_params.outpost_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  outpost_eg ";
    for (auto param : current_params.outpost_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  outpost_hole_mg ";
    for (auto param : current_params.outpost_hole_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  outpost_hole_eg ";
    for (auto param : current_params.outpost_hole_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  outpost_half_mg ";
    for (auto param : current_params.outpost_half_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  outpost_half_eg ";
    for (auto param : current_params.outpost_half_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  ks_pawn_shield ";
    for (int param : current_params.ks_pawn_shield) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  kat_zero " << current_params.kat_zero << std::endl;
    std::cout << "  kat_open_file " << current_params.kat_open_file << std::endl;
    std::cout << "  kat_own_half_open_file " << current_params.kat_own_half_open_file << std::endl;
    std::cout << "  kat_other_half_open_file " << current_params.kat_other_half_open_file << std::endl;

    std::cout << "  kat_attack_weight ";
    for (int param : current_params.kat_attack_weight) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  kat_defence_weight ";
    for (int param : current_params.kat_defence_weight) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  kat_table_scale " << current_params.kat_table_scale << std::endl;
    std::cout << "  kat_table_translate " << current_params.kat_table_translate << std::endl;
    std::cout << "  kat_table_max " << current_params.kat_table_max << std::endl;

    std::cout << "  undefended_mg ";
    for (int param : current_params.undefended_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  undefended_eg ";
    for (int param : current_params.undefended_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << " threat_matrix_mg" << std::endl;
    for(auto &param : current_params.threat_matrix_mg) {
        std::cout << "  {";
        for(auto param2 : param) {
            std::cout << param2 << ", ";
        }
        std::cout << "}, " << std::endl;
    }
    std::cout << " threat_matrix_eg" << std::endl;
    for(auto &param : current_params.threat_matrix_eg) {
        std::cout << "  {";
        for(auto param2 : param) {
            std::cout << param2 << ", ";
        }
        std::cout << "}, " << std::endl;
    }

    std::cout << "  pos_bishop_pair_mg " << current_params.pos_bishop_pair_mg << std::endl;
    std::cout << "  pos_bishop_pair_eg " << current_params.pos_bishop_pair_eg << std::endl;

    std::cout << "  mat_opp_bishop ";
    for (int param : current_params.mat_opp_bishop) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  pos_r_trapped_mg " << current_params.pos_r_trapped_mg << std::endl;
    std::cout << "  pos_r_behind_own_passer_eg " << current_params.pos_r_behind_own_passer_eg << std::endl;
    std::cout << "  pos_r_behind_enemy_passer_eg " << current_params.pos_r_behind_enemy_passer_eg << std::endl;

    std::cout << "  pos_mob_mg ";
    for (int param : current_params.pos_mob_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  pos_mob_eg ";
    for (int param : current_params.pos_mob_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;
}
