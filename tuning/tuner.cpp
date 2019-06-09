//
// Created by Vincent Tang on 2019-01-04.
//

#include <algorithm>
#include <random>
#include <utility>
#include <cmath>
#include <future>

#include "tuner.h"
#include "../movegen.h"
#include "../movesort.h"

tuner_t::tuner_t(unsigned int threads, size_t entries, std::vector<board_t> &positions, std::vector<double> &results)
        : threads(threads), entries(entries), positions(positions), results(results) {
    if (positions.size() != entries || results.size() != entries) throw std::invalid_argument("invalid entry count");

    current_error = mean_evaluation_error();

    std::cout << "starting error: " << current_error << std::endl;

    // Pick scaling constant
    current_error = momentum_optimise(&scaling_constant, current_error, 500, 1);
    std::cout << "scaling constant = " << scaling_constant << std::endl;
}

int tuner_t::quiesce(board_t &board, int alpha, int beta, evaluator_t &evaluator) {
    int stand_pat = evaluator.evaluate(board);

    if (stand_pat >= beta) return beta;
    if (alpha < stand_pat) alpha = stand_pat;

    movegen_t gen(board);
    move_t next{};
    move_t buf[128] = {};
    gen.gen_caps(buf);
    int idx = 0;

    while ((next = buf[idx++]) != EMPTY_MOVE) {
        if (next.info.captured_type == KING) return INF; // Ignore this position in case of a king capture
        if (board.see(next) < 0) {
            continue;
        }

        board.move(next);
        if (board.is_illegal()) {
            board.unmove();
            continue;
        } else {
            int score = -quiesce(board, -beta, -alpha, evaluator);
            board.unmove();

            if (score >= beta) {
                return beta;
            }
            if (score > alpha) {
                alpha = score;
            }
        }
    }

    return alpha;
}

double tuner_t::sigmoid(double score) {
    return 1.0 / (1.0 + exp(-score / scaling_constant));
}

double tuner_t::mean_evaluation_error() {
    evaluator_t evaluator(current_params, 4096);
    const size_t section_size = entries / threads;

    std::vector<std::future<double>> futures(threads);
    for(unsigned int thread = 0; thread < threads; thread++) {
        futures[thread] = std::async(std::launch::async, [this, section_size, thread] () -> double {
            evaluator_t local_evaluator(current_params, 8 * MB);
            size_t start = section_size * thread;
            size_t end = section_size * (thread + 1);

            double total_squared_error = 0;
            for(size_t i = start; i < end; i++) {
                int raw_eval = local_evaluator.evaluate(positions[i]);
                if (positions[i].record[positions[i].now].next_move) raw_eval = -raw_eval;
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
        if (positions[i].record[positions[i].now].next_move) raw_eval = -raw_eval;
        double eval = sigmoid((double) raw_eval);
        double error = eval - results[i];
        total_squared_error += error * error;
    }

    for(unsigned int thread = 0; thread < threads; thread++) {
        total_squared_error += futures[thread].get();
    }

    return total_squared_error / entries;
}

double tuner_t::momentum_optimise(int *parameter, double current_mea, int max_iter, int step) {
    int original = *parameter;

    // Determine direction
    double adjusted_mea;
    *parameter = original + step;
    if ((adjusted_mea = mean_evaluation_error()) < current_mea) {
        std::cout << "optimising parameter (increasing): " << *parameter << std::endl;

        while (adjusted_mea < current_mea && abs(*parameter - original) <= max_iter) {
            current_mea = adjusted_mea;
            *parameter += step;
            adjusted_mea = mean_evaluation_error();

            std::cout << " : parameter: " << *parameter << " -> " << adjusted_mea << std::endl;
        }

        *parameter -= step;
    } else {
        std::cout << "optimising parameter (decreasing): " << *parameter << std::endl;

        *parameter = original - step;
        adjusted_mea = mean_evaluation_error();
        while (adjusted_mea < current_mea && abs(*parameter - original) <= max_iter) {
            current_mea = adjusted_mea;
            *parameter -= step;
            adjusted_mea = mean_evaluation_error();

            std::cout << " : parameter: " << *parameter << " -> " << adjusted_mea << std::endl;
        }

        *parameter += step;
    }

    std::cout << "parameter optimised: " << *parameter << " -> " << current_mea << std::endl;

    return current_mea;
}

void tuner_t::optimise(int *parameter, size_t count, int max_iter) {
    for (size_t i = 0; i < count; i++) {
        std::cout << "parameter " << i << " of " << count << std::endl;
        current_error = momentum_optimise(parameter + i, current_error, max_iter, 5);
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

void tuner_t::random_optimise(int *parameter, size_t count, int max_iter) {
    std::vector<size_t> indices;
    indices.reserve(count);
    for(size_t i = 0; i < count; i++) {
        indices.push_back(i);
    }

    std::shuffle(indices.begin(), indices.end(), std::mt19937(std::random_device()()));

    for(size_t i = 0; i < count; i++) {
        std::cout << "parameter " << i << " of " << count << std::endl;
        current_error = momentum_optimise(parameter + indices[i], current_error, max_iter, 5);
        current_error = momentum_optimise(parameter + indices[i], current_error, max_iter, 1);
    }
}

void tuner_t::anneal(int *parameter, size_t count, double base_temp, double hc_frac, int max_iter) {
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

void tuner_t::print_params() {
    std::cout << "  mat_mg ";
    for (int param : current_params.mat_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  mat_eg ";
    for (int param : current_params.mat_eg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;


    std::cout << "  mat_exch_scale " << current_params.mat_exch_scale << std::endl;
    std::cout << "  mat_exch_pawn " << current_params.mat_exch_pawn << std::endl;
    std::cout << "  mat_exch_minor " << current_params.mat_exch_minor << std::endl;
    std::cout << "  mat_exch_rook " << current_params.mat_exch_rook << std::endl;
    std::cout << "  mat_exch_queen " << current_params.mat_exch_queen << std::endl;

    std::cout << "  mat_opp_bishop ";
    for (int param : current_params.mat_opp_bishop) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  n_pst_mg ";
    for (int param : current_params.n_pst_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  q_pst_mg ";
    for (int param : current_params.q_pst_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  n_pst_eg ";
    for (int param : current_params.n_pst_eg) {
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

    std::cout << "  r_pst_mg ";
    for (int param : current_params.r_pst_mg) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

    std::cout << "  b_pst_eg ";
    for (int param : current_params.b_pst_eg) {
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

    std::cout << "  blocked_mg " << current_params.blocked_mg << std::endl;
    std::cout << "  blocked_eg " << current_params.blocked_eg << std::endl;
    std::cout << "  pos_r_open_file_mg " << current_params.pos_r_open_file_mg << std::endl;
    std::cout << "  pos_r_open_file_eg " << current_params.pos_r_open_file_eg << std::endl;
    std::cout << "  pos_r_own_half_open_file_mg " << current_params.pos_r_own_half_open_file_mg << std::endl;
    std::cout << "  pos_r_own_half_open_file_eg " << current_params.pos_r_own_half_open_file_eg << std::endl;
    std::cout << "  pos_r_other_half_open_file_mg " << current_params.pos_r_other_half_open_file_mg << std::endl;
    std::cout << "  pos_r_other_half_open_file_eg " << current_params.pos_r_other_half_open_file_eg << std::endl;

    std::cout << "  ks_pawn_shield ";
    for (int param : current_params.ks_pawn_shield) {
        std::cout << param << ", ";
    }
    std::cout << std::endl;

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
    std::cout << "  kat_table_offset " << current_params.kat_table_offset << std::endl;

    std::cout << "  pos_bishop_pair_mg " << current_params.pos_bishop_pair_mg << std::endl;
    std::cout << "  pos_bishop_pair_eg " << current_params.pos_bishop_pair_eg << std::endl;

    std::cout << "  pos_r_trapped_mg " << current_params.pos_r_trapped_mg << std::endl;
    std::cout << "  pos_r_behind_own_passer_eg " << current_params.pos_r_behind_own_passer_eg << std::endl;
    std::cout << "  pos_r_behind_enemy_passer_eg " << current_params.pos_r_behind_enemy_passer_eg << std::endl;
    std::cout << "  pos_r_xray_pawn_eg " << current_params.pos_r_xray_pawn_eg << std::endl;

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

