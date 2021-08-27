#include <memory>
#include <iostream>
#include <sstream>
#include <future>
#include <climits>

#include "board.h"
#include "search.h"

#include "syzygy/tbprobe.h"

U64 perft(board_t &, int);

int main(int argc, char *argv[]) {
    // Initialise engine
    init_tables();
    evaluator_t::eval_init();

    // Board
    std::unique_ptr<board_t> board = nullptr;

    // Hash
    uint64_t hash_size = 128;
    tt::hash_t *tt;
    std::mutex tt_memory_mtx;
    tt = new tt::hash_t(hash_size * MB);

    // Evaluation
    processed_params_t params = processed_params_t(eval_params_t());

    // Search
    std::unique_ptr<search_t> search = std::make_unique<search_t>(tt, params, 1);
    std::atomic_bool search_abort;
    std::future<void> future;
    bool search_active;

    // Parameters
    size_t threads = 1;
    int move_overhead = 50;
    size_t syzygy_resolve = 512;
    std::string tb_path;

    // Startup
    std::cout << "Topple " << TOPPLE_VER << " (c) Vincent Tang 2020" << std::endl;

    while (true) {
        std::string input;
        std::getline(std::cin, input);

        std::istringstream iss(input);

        {
            std::string cmd;
            iss >> cmd;

            if (cmd == "uci") {
                // Print ids
                std::cout << "id name Topple " << TOPPLE_VER << std::endl;
                std::cout << "id author Vincent Tang" << std::endl;

                // Print options
                std::cout << "option name Hash type spin default 128 min 1 max 131072" << std::endl;
                std::cout << "option name MoveOverhead type spin default 50 min 0 max 10000" << std::endl;
                std::cout << "option name Threads type spin default 1 min 1 max 256" << std::endl;
                std::cout << "option name SyzygyPath type string default <empty>" << std::endl;
                std::cout << "option name SyzygyResolve type spin default 512 min 1 max 1024" << std::endl;
                std::cout << "option name Ponder type check default false" << std::endl;

                std::cout << "uciok" << std::endl;
            } else if (cmd == "setoption") {
                if (search_active) {
                    std::cerr << "warn: setoption command rejected as search is in progress" << std::endl;
                } else {
                    std::string name;

                    // Read "name <name>", or just "<name>"
                    iss >> name;
                    if (name == "name") iss >> name;

                    if (name == "Hash") {
                        std::string value;
                        iss >> value; // Skip value
                        iss >> hash_size;

                        // Resize hash
                        {
                            std::lock_guard<std::mutex> lock(tt_memory_mtx);
                            delete tt;
                            tt = new tt::hash_t(hash_size * MB);
                        }

                        // Recreate search
                        search = std::make_unique<search_t>(tt, params, threads);
                    } else if (name == "MoveOverhead") {
                        std::string value;
                        iss >> value;
                        iss >> move_overhead;
                    } else if (name == "Threads") {
                        std::string value;
                        iss >> value; // Skip value
                        iss >> threads;

                        search = std::make_unique<search_t>(tt, params, threads);
                    } else if (name == "SyzygyPath") {
                        std::string value;
                        iss >> value; // Skip value

                        std::getline(iss, tb_path);

                        tb_path = tb_path.substr(1, tb_path.size() - 1);

                        std::cout << "Looking for tablebases in: " << tb_path << std::endl;

                        init_tablebases(tb_path.c_str());
                    } else if (name == "SyzygyResolve") {
                        std::string value;
                        iss >> value; // Skip value
                        iss >> syzygy_resolve;
                    } else if (name == "Ponder") {
                        // Do nothing
                    } else {
                        std::cerr << "warn: unrecognised option " << name << std::endl;
                    }
                }
            } else if (cmd == "isready") {
                std::cout << "readyok" << std::endl;
            } else if (cmd == "stop") {
                if (search_active) {
                    // Cancel waiting for ponderhit by enabling the search timer, and then hard-aborting.
                    search->enable_timer();
                    search_abort = true;

                    // Wait for the search to finish before accepting any other commands.
                    future.wait();
                } else {
                    std::cerr << "warn: stop command received, but no search was in progress" << std::endl;
                }
            } else if (cmd == "position") {
                std::string type;
                while (iss >> type) {
                    if (type == "startpos") {
                        board = std::make_unique<board_t>("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                    } else if (type == "fen") {
                        std::string fen;
                        for (int i = 0; i < 6; i++) {
                            std::string component;
                            iss >> component;
                            fen += component + " ";
                        }

                        board = std::make_unique<board_t>(fen);
                    } else if (type == "moves") {
                        if (board) {
                            // Read moves
                            std::string move_str;
                            while (iss >> move_str) {
                                move_t move = board->parse_move(move_str);
                                if (board->is_pseudo_legal(move)) {
                                    board->move(move);
                                    if (board->is_illegal()) {
                                        std::cerr << "warn: illegal move " << move_str << std::endl;
                                        board->unmove();
                                    }
                                } else {
                                    std::cerr << "warn: invalid move " << move_str << std::endl;
                                }
                            }
                        } else {
                            std::cerr << "warn: no start position specified" << std::endl;
                        }
                    }
                }
            } else if (cmd == "go") {
                if (search_active) {
                    std::cerr << "warn: go command received, but search already in progress" << std::endl;
                } else if (board) {
                    // Parse parameters
                    int max_time = INT_MAX;
                    int max_depth = MAX_PLY;
                    U64 max_nodes = UINT64_MAX;
                    bool ponder = false;
                    std::vector<move_t> root_moves;

                    struct {
                        bool enabled = false;
                        int time = 0;
                        int inc = 0;
                        int moves = 0;
                    } time_control;

                    std::string param;
                    while (iss >> param) {
                        if (param == "infinite") {
                            max_time = INT_MAX;
                            max_depth = MAX_PLY;
                        } else if (param == "depth") {
                            iss >> max_depth;
                        } else if (param == "movetime") {
                            iss >> max_time;
                        } else if (param == "nodes") {
                            iss >> max_nodes;
                        } else if (param == "searchmoves") {
                            std::string move_str;
                            while (iss >> move_str) {
                                move_t move = board->parse_move(move_str);
                                if (move != EMPTY_MOVE) {
                                    root_moves.push_back(move);
                                } else {
                                    break;
                                }
                            }
                        } else if (param == "wtime") {
                            time_control.enabled = true;
                            if (board->now().next_move == WHITE) {
                                iss >> time_control.time;
                            }
                        } else if (param == "btime") {
                            time_control.enabled = true;
                            if (board->now().next_move == BLACK) {
                                iss >> time_control.time;
                            }
                        } else if (param == "winc") {
                            time_control.enabled = true;
                            if (board->now().next_move == WHITE) {
                                iss >> time_control.inc;
                            }
                        } else if (param == "binc") {
                            time_control.enabled = true;
                            if (board->now().next_move == BLACK) {
                                iss >> time_control.inc;
                            }
                        } else if (param == "movestogo") {
                            time_control.enabled = true;
                            iss >> time_control.moves;
                        } else if (param == "ponder") {
                            ponder = true;
                        }
                    }

                    // Setup
                    search_active = true;
                    search_abort = false;

                    search_limits_t limits = time_control.enabled ?
                                             search_limits_t(time_control.time - move_overhead,
                                                             time_control.inc, time_control.moves) :
                                             search_limits_t(max_time, max_depth, max_nodes, root_moves);

                    limits.syzygy_resolve = syzygy_resolve;

                    if (!ponder) search->enable_timer();

                    // Start search
                    future = std::async(std::launch::async,
                                        [&tt, &search, &board, &search_active, ponder, &search_abort, &tt_memory_mtx, limits] {
                                            search_result_t result = search->think(*board, limits, search_abort);

                                            std::cout << "bestmove " << result.best_move;
                                            if (result.ponder != EMPTY_MOVE) {
                                                std::cout << " ponder " << result.ponder;
                                            }
                                            std::cout << std::endl;

                                            search_active = false;
                                            search->reset_timer();

                                            // Age the transposition table
                                            {
                                                std::lock_guard<std::mutex> lock(tt_memory_mtx);
                                                tt->age();
                                            }
                                        }
                    );
                } else {
                    std::cerr << "warn: search command received, but no position specified" << std::endl;
                }
            } else if (cmd == "ponderhit") {
                if (search_active) {
                    search->enable_timer();
                } else {
                    std::cerr << "warn: ponderhit command received, but no search in progress" << std::endl;
                }
            } else if (cmd == "ucinewgame") {
                if (search_active) {
                    std::cerr << "warn: ucinewgame command received, but search is in progress" << std::endl;
                } else {
                    {
                        // Recreate hash table
                        std::lock_guard<std::mutex> lock(tt_memory_mtx);
                        delete tt;
                        tt = new tt::hash_t(hash_size * MB);
                    }

                    // Recreate search
                    search = std::make_unique<search_t>(tt, params, threads);
                }
            } else if (cmd == "mirror") {
                if (board) {
                    board->mirror();
                } else {
                    std::cerr << "warn: mirror command received, but no position specified" << std::endl;
                }
            } else if (cmd == "tbprobe") {
                std::string type;
                iss >> type;

                if (board) {
                    const std::string cases[5] = {"loss", "blessed_loss", "draw", "cursed_win", "win"};

                    if (type == "dtz") {
                        int success;
                        int dtz = probe_dtz(*board, &success);

                        if (success) {
                            int cnt50 = board->now().halfmove_clock;
                            int wdl = 0;
                            if (dtz > 0)
                                wdl = (dtz + cnt50 <= 100) ? 2 : 1;
                            else if (dtz < 0)
                                wdl = (-dtz + cnt50 <= 100) ? -2 : -1;

                            std::cout << "syzygy " << cases[wdl + 2] << " dtz " << dtz << std::endl;
                        } else {
                            std::cout << "syzygy failed" << std::endl;
                        }
                    } else if (type == "wdl") {
                        int success;
                        int wdl = probe_wdl(*board, &success);

                        if (success) {
                            std::cout << "syzygy " << cases[wdl + 2] << std::endl;
                        } else {
                            std::cout << "syzygy failed" << std::endl;
                        }
                    } else {
                        std::cerr << "warn: unrecognised table type: dtz or wdl?" << std::endl;
                    }
                } else {
                    std::cerr << "warn: tbprobe command received, but no position specified" << std::endl;
                }
            } else if (cmd == "print") {
                if (board) {
                    std::cout << *board << std::endl;
                } else {
                    std::cout << "nullptr" << std::endl;
                }
            } else if (cmd == "quit" || cmd == "exit") {
                search_abort = true;
                break;
            } else if (!cmd.empty()) {
                std::cerr << "warn: unrecognised command " << cmd << std::endl;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(tt_memory_mtx);
        delete tt;
    }

    return 0;
}

