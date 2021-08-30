# Topple

Topple is a UCI-compatible chess engine. 
Topple v0.7.5 is rated 3063 on [CCRL 40/15](http://www.computerchess.org.uk/ccrl/4040/index.html) on 4 threads.

## Download

Windows users should go to the Releases page, which hosts pre-built binaries.
The `modern` binary should be preferred, but `popcnt` can be used for older systems.
The `legacy` build does not use the popcount instruction and is slowest.

Compilation instructions for other platforms are available below.

## Usage
Topple requires a GUI that supports the UCI protocol to be used comfortably, although it can be used from the command line.

A list of UCI-compatible chess interfaces is available on the [Chess Programming Wiki](https://www.chessprogramming.org/UCI#GUIs).

Topple implements some extensions to the UCI protocol to make it easier to use from the command line.
- `eval` returns a static evaluation of the position
- `print` displays a textual representation of the board and previous moves
- `mirror` flips the colours in the current position
- `position moves ...` is stateful and can be used to continue an existing position
  - The UCI protocol normally requires each position command to specify all moves from a start position, 
    e.g. `position startpos moves e2e4 e7e5 g1f3 ...`

### Configuration

Five configuration options are made available: `Hash`, `MoveOverhead`, `Threads`, `SyzygyPath` and `Ponder`.

The `Hash` option sets the size of the main transposition table in MiB. If the size given is not a power of two, Topple will round it down to next lowest power of 2 to maximise probing efficiency. For example, if a value of 1000 is specified, Topple will only use a 512 MiB hash table. `Hash` does not control the value of the other tables in Topple, such as those used for move generation, evaluation and other data structures.

The `MoveOverhead` option sets the (network or GUI) delay that should be accounted for in time management. This can be used to prevent losses on time.

The `Threads` option sets the number of search threads that Topple will use. Topple may use additional threads for keeping track of inputs (such as the UCI `stop` command). Topple utilises additional threads by using Lazy SMP, so the `Hash` value should be increased to improve scaling with additional threads. 

The `SyzygyPath` option sets the location in which Topple should search for Syzygy tablebases. These can be used to somewhat improve playing strength in the endgame. Multiple paths should be delimited by a semicolon on Windows and a colon on other operating systems.

The `Ponder` option has no effect, but is used to indicate that Topple has the ability to think during its opponent's time.

# Compilation

Topple uses GCC extensions (e.g. `__builtin_prefetch`) and will need a GCC-compatible toolchain.
Beyond that, the code is platform independent and has been tested on both x86 and ARM.

Install a toolchain and CMake (example commands given for Ubuntu):
```shell
sudo apt install build-essential cmake
```

Clone and build Topple:

```shell
git clone https://github.com/konsolas/ToppleChess
mkdir build
cd build
cmake ../ToppleChess
make Topple
```

Run unit tests: (optional)

```shell
make ToppleTest
ctest
```

Building a release: (optional)

```shell
cmake -DCMAKE_BUILD_TYPE=Release ../ToppleChess
make Release
```

The current Windows binaries are built with LLVM/clang.
