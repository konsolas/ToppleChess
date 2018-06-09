# Topple

Chess engine with a mostly complete implementation of the UCI interface.
Estimated rating is around 2100 (CCRL 40/4 conditions).

## Techniques used
 - Alpha-beta Principal Variation Search in an Iterative Deepening framework
 - Basic time management and search limits
 - Aspiration windows
 - Quiescence search
 - SEE ordering and pruning in quiescence search
 - Basic transposition table (Zobrist hashing)
 - Killer move heuristic
 - History heuristic
 - Bitboard representation
 - Check extensions
 - Mate distance pruning
 - Tapered evaluation
 - Specialised endgame evaluation
 - Piece-squares tables
 - Pawn structure evaluation
 - Mobility evaluation
 - Basic king safety evaluation
 - Unit testing with the CATCH framework.
 - Multithreading with Lazy SMP
 - Extensions to UCI:
     - "eval" command returns a static evaluation of the position
     - "print" displays a textual representation of the board and previous moves
     - "position moves ..." is stateful and can be used to continue an existing position