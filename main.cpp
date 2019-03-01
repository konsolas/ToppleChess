#include <memory>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <future>

#include "board.h"
#include "movegen.h"
#include "search.h"
#include "eval.h"
#include "endgame.h"

#define TOPPLE_VER "0.3.6"

U64 perft(board_t &, int);

int main(int argc, char *argv[]) {
    // Initialise engine
    init_tables();
    zobrist::init_hashes();
    evaluator_t::eval_init();
    eg_init();

    // Board
    std::unique_ptr<board_t> board = nullptr;

    // Hash
    uint64_t hash_size = 128;
    tt::hash_t *tt;
    tt = new tt::hash_t(hash_size * MB);

    // Search
    std::unique_ptr<search_t> search = nullptr;
    std::atomic_bool search_abort;
    std::future<void> future;

    // Threads
    unsigned int threads = 1;

    // Startup
    std::cout << "Topple v" << TOPPLE_VER << " (c) Vincent Tang 2019" << std::endl;

    while (true) {
        std::string input;
        std::getline(std::cin, input);

        std::istringstream iss(input);

        {
            std::string cmd;
            iss >> cmd;

            if (cmd == "uci") {
                // Print ids
                std::cout << "id name Topple v" << TOPPLE_VER << std::endl;
                std::cout << "id author Vincent Tang" << std::endl;

                // Print options
                std::cout << "option name Hash type spin default 128 min 1 max 1048576" << std::endl;
                std::cout << "option name Threads type spin default 1 min 1 max 128" << std::endl;
                std::cout << "option name Clear Hash type button" << std::endl;
                std::cout << "option name Ponder type check default false" << std::endl;

                std::cout << "uciok" << std::endl;
            } else if (cmd == "setoption") {
                std::string name;

                // Read "name <name>", or just "<name>"
                iss >> name;
                if (name == "name") iss >> name;

                if (name == "Hash") {
                    std::string value;
                    iss >> value; // Skip value
                    iss >> hash_size;

                    // Resize hash
                    delete tt;
                    tt = new tt::hash_t(hash_size * MB);
                } else if (name == "Threads") {
                    std::string value;
                    iss >> value; // Skip value
                    iss >> threads;
                } else if (name == "Clear") {
                    iss >> name;
                    if (name == "Hash") {
                        // Recreate hash
                        delete tt;
                        tt = new tt::hash_t(hash_size * MB);
                    } else {
                        std::cout << "info string unrecognised option " << name << std::endl;
                    }
                } else if(name == "Ponder") {
                    // Do nothing
                } else {
                    std::cout << "info string unrecognised option " << name << std::endl;
                }
            } else if (cmd == "isready") {
                std::cout << "readyok" << std::endl;
            } else if (cmd == "stop") {
                if(search) {
                    // Cancel waiting for ponderhit by enabling the search timer, and then hard-aborting.
                    search->enable_timer();
                    search_abort = true;
                } else {
                    std::cout << "info string stop command received, but no search was in progress" << std::endl;
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
                                        std::cout << "info string illegal move " << move_str << std::endl;
                                        board->unmove();
                                    }
                                } else {
                                    std::cout << "info string invalid move " << move_str << std::endl;
                                }
                            }
                        } else {
                            std::cout << "info string no start position specified" << std::endl;
                        }
                    }
                }
            } else if (cmd == "go") {
                if(search) {
                    std::cout << "info string go command received, but search already in progress" << std::endl;
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
                            if (board->record[board->now].next_move == WHITE) {
                                iss >> time_control.time;
                            }
                        } else if (param == "btime") {
                            time_control.enabled = true;
                            if (board->record[board->now].next_move == BLACK) {
                                iss >> time_control.time;
                            }
                        } else if (param == "winc") {
                            time_control.enabled = true;
                            if (board->record[board->now].next_move == WHITE) {
                                iss >> time_control.inc;
                            }
                        } else if (param == "binc") {
                            time_control.enabled = true;
                            if (board->record[board->now].next_move == BLACK) {
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
                    search_abort = false;

                    search_limits_t limits = time_control.enabled ?
                                             search_limits_t(board->now,
                                                             time_control.time,
                                                             time_control.inc,
                                                             time_control.moves)
                                                                  :
                                             search_limits_t(max_time,
                                                             max_depth,
                                                             max_nodes,
                                                             root_moves);
                    search = std::make_unique<search_t>(*board, tt, threads, limits);

                    if (!ponder) search->enable_timer();

                    // Start search
                    future = std::async(std::launch::async,
                                        [&tt, &search, ponder, &search_abort, limits] {
                                            std::future<search_result_t> bm = std::async(std::launch::async,
                                                                                         &search_t::think,
                                                                                         search.get(),
                                                                                         std::ref(search_abort));
                                            search_result_t result;

                                            // Wait for actual search to start
                                            if (ponder) {
                                                search->wait_for_timer();
                                            }

                                            if (bm.wait_for(std::chrono::milliseconds(limits.hard_time_limit)) ==
                                                std::future_status::ready) {
                                                result = bm.get();
                                            } else {
                                                search_abort = true;
                                                result = bm.get();
                                            }

                                            std::cout << "bestmove " << result.best_move;
                                            if(result.ponder != EMPTY_MOVE) {
                                                std::cout << " ponder " << result.ponder;
                                            }
                                            std::cout << std::endl;

                                            search = nullptr;

                                            // Age the transposition table
                                            tt->age();
                                        }
                    );
                } else {
                    std::cout << "info string search command received, but no position specified" << std::endl;
                }
            } else if (cmd == "ponderhit") {
                if (search) {
                    search->enable_timer();
                } else {
                    std::cout << "info string ponderhit command received, but no search in progress" << std::endl;
                }
            } else if (cmd == "ucinewgame") {
                if(search) {
                    std::cout << "info string ucinewgame command received, but search is in progress" << std::endl;
                } else {
                    delete tt;
                    tt = new tt::hash_t(hash_size * MB);
                }
            } else if (cmd == "eval") {
                if (board) {
                    std::cout << evaluator_t(eval_params_t(), MB).evaluate(*board) << std::endl;
                } else {
                    std::cout << "info string eval command received, but no position specified" << std::endl;
                }
            } else if (cmd == "print") {
                if (board) {
                    std::cout << *board << std::endl;
                } else {
                    std::cout << "nullptr" << std::endl;
                }
            } else if (cmd == "quit") {
                search_abort = true;
                std::cout << "info string exiting" << std::endl;
                break;
            } else if (!cmd.empty()) {
                std::cout << "info string unrecognised command " << cmd << std::endl;
            }
        }
    }

    return 0;
}

