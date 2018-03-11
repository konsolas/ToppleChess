# Topple

Chess engine with a partial implementation of the UCI interface. Works, quite weak play, and currently unfinished.

## Techniques used
 - Alpha-beta Principal Variation Search in an Iterative Deepening framework
 - Quiescence search
 - Bitboard representation
 - Basic transposition table (Zobrist hashing)
 - Check extensions
 - Mate distance pruning
 - MVV/LVA move ordering
 - Tapered evaluation
 - Pawn structure evaluation
 - Mobility evaluation
 - Unit testing with CATCH framework.