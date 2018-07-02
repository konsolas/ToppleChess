# Topple

Chess engine with a mostly complete implementation of the UCI interface.
Topple v0.1.1 is rated 2338 in CCRL 40/4 and 2360 in CCRL 40/40.

## Techniques used
 - Alpha-beta Principal Variation Search
 - Iterative deepening
 - Basic time management and search limits
 - Aspiration windows
 - Quiescence search
 - SEE move ordering
 - SEE pruning
 - Delta pruning
 - Basic transposition table (Zobrist hashing)
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
 - Tapered evaluation
 - Specialised endgame evaluation
 - Piece-square tables
 - Pawn structure evaluation
 - Mobility evaluation
 - King safety evaluation
 - Unit testing with the CATCH framework
 - Multithreading with Lazy SMP (unfinished)
 - Extensions to UCI:
     - "eval" command returns a static evaluation of the position
     - "print" displays a textual representation of the board and previous moves
     - "position moves ..." is stateful and can be used to continue an existing position