# Topple

Topple is a UCI-compatible chess engine.
Topple v0.2.1 is rated 2457 in CCRL 40/40 and 2472 in CCRL 40/40.

## Usage
Topple requires a GUI that supports the UCI protocol to be used comfortably, although it can be used from the command line.
Three configuration options are made available: `Hash`, `Threads` and `Clear Hash`.

The `Hash` option sets the size of the main transposition table in MiB. If the size given is not a power of two, Topple will round it down to next lowest power of 2 to maximise probing efficiency. For example, if a value of 1000 is specified, Topple will only use a 512 MiB hash table. `Hash` does not control the value of the other tables in Topple, such as those used for move generation, evaluation and other data structures.

The `Threads` option sets the number of search threads that Topple will use. Topple may use additional threads for keeping track of inputs (such as the UCI `stop` command). Topple utilises additional threads by using Lazy SMP, so the `Hash` value should be increased to improve scaling with additional threads. 

The `Clear Hash` option deletes and reallocates Topple's main transposition table.

## Techniques used
 - Alpha-beta Principal Variation Search
 - Iterative deepening
 - Basic time management and search limits
 - Aspiration windows
 - Quiescence search
 - SEE move ordering
 - SEE pruning
 - Delta pruning
 - 4-bucket transposition table with Zobrist hashing
 - Killer move heuristic
 - History heuristic
 - Bitboard representation
 - Check extensions
 - Internal iterative deepening
 - Restricted singular extension search
 - Adaptive null move pruning
 - Mate distance pruning
 - Late move reductions
 - History pruning
 - Futility pruning
 - Pawn evaluation hash table
 - Tapered evaluation
 - Specialised endgame evaluation
 - Piece-square tables
 - Pawn structure evaluation
 - King safety evaluation
 - Parameter tuning with an adapted version of Texel's tuning method
 - Unit testing with the CATCH framework
 - Multithreading with Lazy SMP
 - Extensions to UCI:
     - "eval" command returns a static evaluation of the position
     - "print" displays a textual representation of the board and previous moves
     - "position moves ..." is stateful and can be used to continue an existing position