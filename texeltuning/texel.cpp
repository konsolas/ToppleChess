//
// Created by Vincent Tang on 2019-01-04.
//

#include <algorithm>
#include <random>
#include <utility>
#include <cmath>
#include <future>

#include "texel.h"
#include "../movegen.h"
#include "../movesort.h"

texel_t::texel_t(unsigned int threads, size_t entries, std::vector<board_t> &positions, std::vector<double> &results)
        : threads(threads), entries(entries), positions(positions), results(results) {
    if (positions.size() != entries || results.size() != entries) throw std::invalid_argument("invalid entry count");

    current_error = mean_evaluation_error();

    std::cout << sizeof(eval_params_t) / sizeof(int) << " parameters" << std::endl;
    std::cout << "starting error: " << current_error << std::endl;

    // Pick scaling constant
    current_error = momentum_optimise(&scaling_constant, current_error, 500, 1);
    std::cout << "scaling constant = " << scaling_constant << std::endl;
}

double texel_t::sigmoid(double score) {
    return 1.0 / (1.0 + exp(-score / scaling_constant));
}

double texel_t::mean_evaluation_error() {
    auto processed = processed_params_t(current_params);
    evaluator_t evaluator(processed, 4096);
    const size_t section_size = entries / threads;

    std::vector<std::future<double>> futures(threads);
    for(unsigned int thread = 0; thread < threads; thread++) {
        futures[thread] = std::async(std::launch::async, [this, processed, section_size, thread] () -> double {
            evaluator_t local_evaluator(processed, 8 * MB);
            size_t start = section_size * thread;
            size_t end = section_size * (thread + 1);

            double total_squared_error = 0;
            for(size_t i = start; i < end; i++) {
                int raw_eval = local_evaluator.evaluate(positions[i]);
                if (positions[i].record.back().next_move) raw_eval = -raw_eval;
                double eval = sigmoid((double) raw_eval);
                double error = eval - results[i];
                total_squared_error += error * error;
            }

            return total_squared_error;
        });
    }

    double total_squared_error = 0;

    // Add on the missing bits (up to 3)
    for(size_t i = section_size * threads; i < entries; i++) {
        int raw_eval = evaluator.evaluate(positions[i]);
        if (positions[i].record.back().next_move) raw_eval = -raw_eval;
        double eval = sigmoid((double) raw_eval);
        double error = eval - results[i];
        total_squared_error += error * error;
    }

    for(unsigned int thread = 0; thread < threads; thread++) {
        total_squared_error += futures[thread].get();
    }

    return total_squared_error / entries;
}

double texel_t::momentum_optimise(int *parameter, double current_mea, int max_iter, int step) {
    int original = *parameter;

    // Determine direction
    double adjusted_mea;
    *parameter = original + step;
    if ((adjusted_mea = mean_evaluation_error()) < current_mea) {
        std::cout << "p+: " << *parameter << " ";

        while (adjusted_mea < current_mea && abs(*parameter - original) <= max_iter) {
            current_mea = adjusted_mea;
            *parameter += step;
            adjusted_mea = mean_evaluation_error();

            std::cout << *parameter << " ";
        }

        *parameter -= step;
    } else {
        std::cout << "p-: " << *parameter << " ";

        *parameter = original - step;
        adjusted_mea = mean_evaluation_error();
        while (adjusted_mea < current_mea && abs(*parameter - original) <= max_iter) {
            current_mea = adjusted_mea;
            *parameter -= step;
            adjusted_mea = mean_evaluation_error();

            std::cout << *parameter << " ";
        }

        *parameter += step;
    }

    std::cout << *parameter << " -> " << current_mea << std::endl;

    return current_mea;
}

void texel_t::optimise(int *parameter, size_t count, int max_iter) {
    for (size_t i = 0; i < count; i++) {
        std::cout << "parameter " << i << " of " << count << ": ";
        current_error = momentum_optimise(parameter + i, current_error, max_iter, 5);
        std::cout << "parameter " << i << " of " << count << ": ";
        current_error = momentum_optimise(parameter + i, current_error, max_iter, 1);
    }

    std::cout << "Final result:";
    for (size_t i = 0; i < count; i++) {
        std::cout << " " << *(parameter + i);
        if (i < count - 1) {
            std::cout << ",";
        }
    }
    std::cout << std::endl;
}

void texel_t::random_optimise(int *parameter, size_t count, int max_iter) {
    std::vector<size_t> indices;
    indices.reserve(count);
    for(size_t i = 0; i < count; i++) {
        indices.push_back(i);
    }

    std::shuffle(indices.begin(), indices.end(), std::mt19937(std::random_device()()));

    for(size_t i = 0; i < count; i++) {
        std::cout << "parameter " << i << " of " << count << ": ";
        current_error = momentum_optimise(parameter + indices[i], current_error, max_iter, 5);
        std::cout << "parameter " << i << " of " << count << ": ";
        current_error = momentum_optimise(parameter + indices[i], current_error, max_iter, 1);
    }
}

void texel_t::anneal(int *parameter, size_t count, double base_temp, double hc_frac, int max_iter) {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<size_t> dist(0, count - 1);
    std::bernoulli_distribution choice;

    auto start = engine_clock::now();

    for(int i = 0; i < max_iter; i++) {
        double temp = std::max(0.0, (max_iter - (int) (i * (1 + hc_frac))) * base_temp / max_iter);

        int *param = parameter + dist(rng);
        int offset = choice(rng) ? (choice(rng) ? -1 : 1) : (choice(rng) ? -10 : 10);

        *param += offset;

        double new_error = mean_evaluation_error();

        std::bernoulli_distribution accept(new_error < current_error ? 1.0 : exp(-(new_error - current_error) / temp));
        if(!accept(rng)) {
            *param -= offset;
        } else {
            current_error = new_error;
        }

        if(i % 1000 == 0) {
            std::cout << "time: " << CHRONO_DIFF(start, engine_clock::now()) / 1000 << "s" << " ";
            std::cout << "epoch: " << i << " error: " << current_error << std::endl;
        }
    }

    std::cout << "finished: final error: " << current_error << std::endl;
    print_params();
}

void texel_t::print_params() {
    std::cout << "int mat_exch_knight = " << current_params.mat_exch_knight << ";" << std::endl;
    std::cout << "int mat_exch_bishop = " << current_params.mat_exch_bishop << ";" << std::endl;
    std::cout << "int mat_exch_rook = " << current_params.mat_exch_rook << ";" << std::endl;
    std::cout << "int mat_exch_queen = " << current_params.mat_exch_queen << ";" << std::endl;

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

    std::cout << "v4si_t undefended[5] = {";
    for (const auto &param : current_params.undefended) {
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

