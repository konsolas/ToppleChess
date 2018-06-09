#include <iostream>
#include <sstream>
#include <iomanip>
#include <future>

#include "board.h"
#include "movegen.h"
#include "search.h"
#include "eval.h"

#define TOPPLE_VER "0.0.2"

const unsigned int MB = 1048576;

U64 perft(board_t &, int);

int main(int argc, char *argv[]) {
    // Initialise engine
    init_tables();
    zobrist::init_hashes();
    eval_init();

    // Setup input
    std::ios_base::sync_with_stdio(false);

    // Board
    board_t *board = nullptr;

    // Hash
    uint64_t hash_size = 128;
    tt::hash_t *tt;
    tt = new tt::hash_t(hash_size * MB);

    // Search
    search_t *search = nullptr;
    std::atomic_bool search_abort;
    std::future<void> future;

    // Threads
    unsigned int threads = 1;

    // Startup
    std::cout << "Topple v" << TOPPLE_VER << " (c) Vincent Tang 2018" << std::endl;

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

                std::cout << "uciok" << std::endl;
            } else if(cmd == "setoption") {
                std::string name;

                // Read "name <name>", or just "<name>"
                iss >> name; if(name == "name") iss >> name;

                if(name == "Hash") {
                    std::string value; iss >> value; if(value == "value");
                    iss >> hash_size;

                    // Resize hash
                    delete tt;
                    tt = new tt::hash_t(hash_size * MB);
                } else if(name == "Threads") {
                    std::string value; iss >> value; if(value == "value");
                    iss >> threads;
                } else if(name == "Clear") {
                    iss >> name;
                    if(name == "Hash") {
                        // Recreate hash
                        delete tt;
                        tt = new tt::hash_t(hash_size * MB);
                    } else {
                        std::cout << "unrecognised option" << std::endl;
                    }
                } else {
                    std::cout << "unrecognised option" << std::endl;
                }
            } else if (cmd == "isready") {
                std::cout << "readyok" << std::endl;
            } else if (cmd == "stop") {
                search_abort = true;
            } else if (cmd == "position") {
                std::string type;
                while (iss >> type) {
                    if (type == "startpos") {
                        delete board;
                        board = new board_t("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                    } else if (type == "fen") {
                        std::string fen;
                        for (int i = 0; i < 6; i++) {
                            std::string component;
                            iss >> component;
                            fen += component + " ";
                        }

                        delete board;
                        board = new board_t(fen);
                    } else if (type == "moves") {
                        if (board != nullptr) {
                            // Read moves
                            std::string move_str;
                            while (iss >> move_str) {
                                move_t move = board->parse_move(move_str);
                                if (board->is_pseudo_legal(move)) {
                                    board->move(move);
                                    if (board->is_illegal()) {
                                        std::cout << "illegal move: " << move << std::endl;
                                        board->unmove();
                                    }
                                } else {
                                    std::cout << "invalid move: " << move << std::endl;
                                }
                            }
                        } else {
                            std::cout << "board=nullptr" << std::endl;
                        }
                    }
                }
            } else if (cmd == "go") {
                if (board != nullptr) {
                    // Parse parameters
                    int max_time = INT_MAX;
                    int max_depth = MAX_PLY;
                    U64 max_nodes = UINT64_MAX;
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
                        }
                    }

                    // Setup
                    delete search;
                    search_abort = false;

                    if (time_control.enabled) {
                        search = new search_t(*board, tt, threads, search_limits_t(board->now,
                                                                             time_control.time,
                                                                             time_control.inc,
                                                                             time_control.moves));
                    } else {
                        search = new search_t(*board, tt, threads, search_limits_t(max_time,
                                                                             max_depth,
                                                                             max_nodes,
                                                                             root_moves));
                    }

                    // Start search
                    future = std::async(std::launch::async,
                                        [search, &search_abort, max_time, max_depth] {
                                            std::future<move_t> bm = std::async(std::launch::async, &search_t::think,
                                                                                search, std::ref(search_abort));

                                            move_t best_move{};
                                            if (bm.wait_for(std::chrono::milliseconds(max_time)) ==
                                                std::future_status::ready) {
                                                best_move = bm.get();
                                            } else {
                                                search_abort = true;
                                                best_move = bm.get();
                                            }

                                            std::cout << "bestmove " << best_move << std::endl;
                                        }
                    );
                } else {
                    std::cout << "board=nullptr" << std::endl;
                }
            } else if (cmd == "print") {
                if (board == nullptr) {
                    std::cout << "nullptr" << std::endl;
                } else {
                    std::cout << *board << std::endl;
                }
            } else if (cmd == "quit") {
                std::cout << "exiting" << std::endl;
                break;
            }
        }
    }

    return 0;
}

