# Pikafish Codebase Exploration Report

## Executive Summary

Pikafish is a **free and strong UCI xiangqi engine** derived from Stockfish (not chess). It's a complete chess-like game engine for xiangqi (Chinese chess) written in modern C++. The codebase consists of approximately **14,433 lines** of C++ code with a Makefile-based build system supporting multiple platforms and architectures.

### Key Findings:
1. **No existing WebAssembly/Emscripten configuration** - The codebase only contains references to WASM in external zstd library
2. **Well-structured UCI protocol implementation** - Full command-line interface via UCI standard
3. **Modern C++17/20 codebase** with strong architecture
4. **Supports multiple platforms** - Linux, Windows, macOS, Android via NDK
5. **Ready for WASM compilation** - Clean separation of concerns, minimal platform-specific code

---

## 1. WebAssembly/Emscripten Build Configuration

### Current Status: **NOT IMPLEMENTED**

The codebase contains **no existing WASM or Emscripten configuration**. The only WASM-related references are in the external `zstd` library:

**File:** `/Users/nathan/Developments/Pikafish/src/external/common/xxhash.h`

```cpp
#if defined(__clang__) && defined(__ARM_ARCH) && !defined(__wasm__)
#if (defined(__SSE4_1__) || defined(__aarch64__) || defined(__wasm_simd128__))
// ... conditional SIMD support for WASM
```

These are **library-level** WASM compatibility checks, not engine-specific.

### What's Missing for WASM:
- No Emscripten build targets in Makefile
- No JavaScript bindings or wrapper code
- No specialized WASM entry points
- No threading/web worker configuration

---

## 2. Main Entry Points and UCI Protocol Implementation

### Main Entry Point: `/Users/nathan/Developments/Pikafish/src/main.cpp`

```cpp
int main(int argc, char* argv[]) {
    std::cout << engine_info() << std::endl;
    
    Bitboards::init();
    Position::init();
    
    auto uci = std::make_unique<UCIEngine>(argc, argv);
    Tune::init(uci->engine_options());
    
    uci->loop();  // Enter UCI command loop
    return 0;
}
```

**Initialization sequence:**
1. Initialize bitboards lookup tables
2. Initialize position representation
3. Create UCI engine with command-line arguments
4. Initialize tuning parameters
5. Enter main UCI command loop

### UCI Protocol Implementation: `/Users/nathan/Developments/Pikafish/src/uci.cpp` and `uci.h`

**UCI Engine Class Structure:**

```cpp
class UCIEngine {
public:
    UCIEngine(int argc, char** argv);
    void loop();  // Main command processing loop
    
    // Utility methods
    static int to_cp(Value v, const Position& pos);
    static std::string format_score(const Score& s);
    static std::string square(Square s);
    static std::string move(Move m);
    static Move to_move(const Position& pos, std::string str);
    static Search::LimitsType parse_limits(std::istream& is);
    
    auto& engine_options() { return engine.get_options(); }

private:
    Engine engine;
    CommandLine cli;
};
```

**UCI Commands Implemented:**

| Command | Function | Purpose |
|---------|----------|---------|
| `uci` | Identify engine | Returns engine name, version, author |
| `setoption` | Set engine option | Configure hash table, threads, etc. |
| `position` | Set board position | Parse FEN or move sequence |
| `go` | Start search | Begin analysis with time/depth limits |
| `stop` | Stop search | Halt ongoing analysis |
| `ponderhit` | Ponder hit | User played expected move |
| `quit` | Exit engine | Graceful shutdown |
| `isready` | Connection check | Verify engine responsiveness |
| `ucinewgame` | Clear state | Reset for new game |
| `eval` | Evaluate position | Get static evaluation |
| `d` | Display board | ASCII visualization |
| `bench` | Benchmark | Performance testing |
| `flip` | Flip board | Reverse perspective |

**Parse Limits Implementation (Line 179-216):**

```cpp
Search::LimitsType UCIEngine::parse_limits(std::istream& is) {
    Search::LimitsType limits;
    limits.startTime = now();
    
    while (is >> token)
        if (token == "searchmoves") ...
        else if (token == "wtime") is >> limits.time[WHITE];
        else if (token == "btime") is >> limits.time[BLACK];
        else if (token == "winc") is >> limits.inc[WHITE];
        else if (token == "binc") is >> limits.inc[BLACK];
        else if (token == "movestogo") is >> limits.movestogo;
        else if (token == "depth") is >> limits.depth;
        else if (token == "nodes") is >> limits.nodes;
        else if (token == "movetime") is >> limits.movetime;
        else if (token == "mate") is >> limits.mate;
        else if (token == "perft") is >> limits.perft;
        else if (token == "infinite") limits.infinite = 1;
        else if (token == "ponder") limits.ponderMode = true;
    
    return limits;
}
```

### Search Callbacks for JavaScript Integration

The Engine class (in `engine.h`) has callback registration methods perfect for WASM:

```cpp
void set_on_update_no_moves(std::function<void(const InfoShort&)>&&);
void set_on_update_full(std::function<void(const InfoFull&)>&&);
void set_on_iter(std::function<void(const InfoIter&)>&&);
void set_on_bestmove(std::function<void(std::string_view, std::string_view)>&&);
void set_on_verify_networks(std::function<void(std::string_view)>&&);
```

This allows **asynchronous updates** during search - perfect for web callbacks.

---

## 3. Project Structure and Build System

### Directory Structure:

```
/Users/nathan/Developments/Pikafish/
├── src/                          # Source code
│   ├── Makefile                  # Main build configuration
│   ├── main.cpp                  # Entry point
│   ├── engine.{cpp,h}            # Engine core
│   ├── uci.{cpp,h}               # UCI protocol
│   ├── position.{cpp,h}          # Board representation
│   ├── search.{cpp,h}            # Search algorithm
│   ├── evaluate.{cpp,h}          # Evaluation function
│   ├── types.h                   # Type definitions
│   ├── bitboard.{cpp,h}          # Bit manipulation
│   ├── movegen.{cpp,h}           # Move generation
│   ├── thread.{cpp,h}            # Threading support
│   ├── nnue/                      # Neural network evaluation
│   │   ├── network.{cpp,h}
│   │   ├── features/
│   │   └── layers/
│   ├── external/                 # zstd compression library
│   └── [other supporting files]
├── scripts/                       # Build helper scripts
│   ├── get_native_properties.sh
│   └── net.sh
├── tests/                         # Test suite
├── .github/workflows/
│   └── pikafish.yml              # CI/CD configuration
└── README.md, Makefile, etc.
```

### Build System: Makefile-Based

**Build File:** `/Users/nathan/Developments/Pikafish/src/Makefile`

**Configuration Variables:**
- `ARCH`: Target architecture (native, x86-64-avx512, armv8, apple-silicon, etc.)
- `COMP`: Compiler (clang, gcc, mingw, ndk)
- `optimize`: Enable optimization (-O3)
- `debug`: Debug mode (NDEBUG)
- `sanitize`: Address/undefined behavior checks
- `SIMD`: SSE2, SSE4.1, AVX2, AVX512, NEON, etc.

**Build Targets:**
```bash
make help              # Show available targets
make -j build         # Compile
make strip            # Strip symbols
make bench            # Run benchmark
make profile-build    # Profile-guided optimization
```

**Example Builds from CI/CD (`.github/workflows/pikafish.yml`):**

```bash
# Linux - multiple SIMD variants
make -j build EXE=pikafish-avx512icl ARCH=x86-64-avx512icl

# Windows (MinGW)
make -j build EXE=pikafish.exe

# macOS (Apple Silicon)
make -j build EXE=pikafish-apple-silicon ARCH=apple-silicon

# Android (NDK)
make -j build EXE=pikafish-armv8 COMP=ndk ARCH=armv8-dotprod
```

### Compiler Support:
- **GCC** 9.3+
- **Clang** 10.0+
- **MSVC** (Visual Studio)
- **Android NDK** (cross-compilation)

### Total Code Metrics:
- **Total Lines:** ~14,433 (C++ source and headers)
- **Main Components:** 48 source files in `src/`
- **Architecture:** Modular, well-separated concerns

---

## 4. Existing JavaScript Bindings and Web-Related Code

### Current Status: **NONE**

There is **no existing JavaScript binding code** in the repository. However, the architecture is highly amenable to such bindings.

### Search Information Structures (for JavaScript Integration)

**From `search.h`:**

These structures contain all search progress information that can be sent to JavaScript:

```cpp
// Short info update (partial results)
struct InfoShort {
    int      depth;
    int      seldepth;
    int      multipv;
    Value    score;
    bool     upperbound;
    bool     lowerbound;
};

// Full info update (comprehensive results)
struct InfoFull: InfoShort {
    uint64_t nodes;
    uint64_t time;
    std::vector<Move> pv;  // Principal variation (best line)
    int      hashfull;
};

// Iteration info
struct InfoIteration {
    int      depth;
    uint64_t nodes;
};
```

### Engine Interface for Callbacks

**From `engine.h`:**

```cpp
// Non-blocking search
void go(Search::LimitsType&);
void stop();
void wait_for_search_finished();

// Position management
void set_position(const std::string& fen, const std::vector<std::string>& moves);
std::string fen() const;

// Search callbacks
void set_on_update_no_moves(std::function<void(const InfoShort&)>&&);
void set_on_update_full(std::function<void(const InfoFull&)>&&);
void set_on_iter(std::function<void(const InfoIter&)>&&);
void set_on_bestmove(std::function<void(std::string_view, std::string_view)>&&);
```

### Network/Weights Management:

```cpp
void verify_networks() const;
void load_networks();
void load_big_network(const std::string& file);
void save_network(const std::pair<std::optional<std::string>, std::string> files);
```

**Note:** NNUE weights are stored in `.nnue` files (mentioned in README). The engine loads neural network weights at startup.

---

## 5. How the Engine Receives Positions and Returns Moves

### Position Representation: `/Users/nathan/Developments/Pikafish/src/position.h`

**Xiangqi Board:**
- **9 files** (columns A-I)
- **10 ranks** (rows 0-9)
- **90 squares total** (SQ_A0 to SQ_I9)

```cpp
enum Square : int8_t {
    SQ_A0, SQ_B0, ..., SQ_I0,    // Rank 0
    SQ_A1, SQ_B1, ..., SQ_I9,    // Ranks 1-9
    SQ_NONE,
    SQUARE_ZERO = 0,
    SQUARE_NB = 90
};
```

### Move Representation: `/Users/nathan/Developments/Pikafish/src/types.h`

**Move Encoding (16-bit):**
```cpp
class Move {
protected:
    std::uint16_t data;  // 16-bit move encoding
    
public:
    constexpr Move(Square from, Square to) : data((from << 7) + to) {}
    constexpr Square from_sq() const { return Square((data >> 7) & 0x7F); }
    constexpr Square to_sq() const { return Square(data & 0x7F); }
    
    static constexpr Move null() { return Move(129); }
    static constexpr Move none() { return Move(0); }
};
```

**Bit Layout:**
- **Bits 0-6:** Destination square (0-89)
- **Bits 7-13:** Origin square (0-89)

### Piece Types for Xiangqi:

```cpp
enum PieceType : std::int8_t {
    NO_PIECE_TYPE,
    ROOK,           // 车
    ADVISOR,        // 士
    CANNON,         // 炮
    PAWN,           // 兵
    KNIGHT,         // 马
    BISHOP,         // 象
    KING,           // 将/帅
    KNIGHT_TO, PAWN_TO,
    PIECE_TYPE_NB = 8
};

enum Piece : std::int8_t {
    NO_PIECE,
    W_ROOK, W_ADVISOR, W_CANNON, W_PAWN, W_KNIGHT, W_BISHOP, W_KING,
    B_ROOK, B_ADVISOR, B_CANNON, B_PAWN, B_KNIGHT, B_BISHOP, B_KING,
    PIECE_NB
};
```

### Piece Values:
```cpp
constexpr Value RookValue    = 1305;
constexpr Value AdvisorValue = 219;
constexpr Value CannonValue  = 773;
constexpr Value PawnValue    = 144;
constexpr Value KnightValue  = 720;
constexpr Value BishopValue  = 187;
```

### FEN Format:

**Starting Position (Xiangqi):**
```
"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w"
```

**Position Setup via UCI:**

```cpp
// From uci.cpp
void UCIEngine::position(std::istringstream& is) {
    std::string token, fen;
    std::vector<std::string> moves;
    
    is >> token;  // "fen" or "startpos"
    
    if (token == "fen")
        while (is >> token && token != "moves")
            fen += token + " ";
    else
        fen = StartFEN;  // Default xiangqi starting position
    
    // Read move sequence (in UCI notation: "e3e5", etc.)
    if (is >> token && token == "moves")
        while (is >> token)
            moves.push_back(token);
    
    engine.set_position(fen, moves);
}
```

### Engine Move Search:

**From `engine.h`:**

```cpp
// Non-blocking call to start searching
void go(Search::LimitsType&);

// Blocking call to wait for search to finish
void wait_for_search_finished();
```

**Search Limits:**

```cpp
struct LimitsType {
    int    depth;           // Search depth
    int    mate;            // Mate in N moves
    int    nodes;           // Max nodes to search
    int    movetime;        // Time per move (ms)
    int    time[COLOR_NB];  // Remaining time for each side
    int    inc[COLOR_NB];   // Time increment per move
    int    movestogo;       // Moves to next time control
    int    perft;           // Perft depth (not search)
    bool   infinite;        // Search until stop
    bool   ponderMode;      // Ponder mode (search opponent's reply)
    std::vector<std::string> searchmoves;  // Restrict to these moves
};
```

### Getting Results:

**Best Move Output (UCI):**
```
bestmove e2e4 ponder c7c5
```

**Search Progress (UCI):**
```
info depth 20 seldepth 23 multipv 1 score cp 45 nodes 1234567 nps 2345678 time 524 pv e2e4 c7c5 Nf3 d6
```

---

## 6. Platform-Specific Code Analysis

### Threading: `/Users/nathan/Developments/Pikafish/src/thread.{cpp,h}`

- **POSIX threads** for Linux/macOS
- **Windows threads** for Windows
- **Fallback:** `thread_win32_osx.h` for macOS/Windows compatibility
- **NUMA support** for multi-socket systems

### Shared Memory: 

- **Linux:** `shm_linux.h` - Linux-specific shared memory
- **Generic:** `shm.h` - Platform-independent approach

### External Dependencies:

**zstd (Zstandard) Compression Library** - for network file compression
- Located in `/src/external/`
- Already has WASM-compatible code paths

---

## 7. NNUE (Neural Network Evaluation)

The engine uses NNUE evaluation for position assessment.

**Directory:** `/Users/nathan/Developments/Pikafish/src/nnue/`

**Components:**
- **Network architecture:** Half-KA v2 HM (from features/half_ka_v2_hm.{cpp,h})
- **Layers:** Affine transforms, ReLU activations, sparse input handling
- **Accumulator:** Incremental NNUE state updates
- **SIMD optimization:** Multi-platform SIMD support

**Important:** NNUE weights are in binary `.nnue` format files. These will need to be:
1. Embedded or preloaded in the WASM binary
2. Loaded from server or pre-downloaded
3. Cached in browser storage

---

## Recommendations for WASM/Web Implementation

### Architecture Considerations:

1. **Create Emscripten wrapper class** - Wraps the Engine for JavaScript

2. **Async Search Implementation** - Use Emscripten's async callbacks:
   ```cpp
   void set_on_bestmove_js(std::function<void(std::string, std::string)>&&);
   void set_on_update_js(std::function<void(const std::string&)>&&);
   ```

3. **Threading in WASM** - Use Web Workers:
   - Emscripten's pthread support maps OS threads to Web Workers
   - Alternative: Run search in main thread with periodic yields

4. **NNUE Weights** - Package strategy:
   - Embed in WASM binary (increases size ~15-20MB)
   - Load from CDN/server
   - Use IndexedDB for caching

5. **FEN String Interface** - Already perfect for web:
   ```cpp
   // From engine.h
   std::string fen() const;
   void set_position(const std::string& fen, const std::vector<std::string>& moves);
   ```

### File Summary for WASM Binding:

| File | Purpose | Modifications Needed |
|------|---------|---------------------|
| `main.cpp` | Entry point | Create new `wasm_main.cpp` |
| `uci.cpp/h` | UCI protocol | Use indirectly via Engine |
| `engine.cpp/h` | Core engine | **Minimal changes** - already has callbacks |
| `position.cpp/h` | Board state | No changes needed |
| `search.cpp/h` | Search algorithm | No changes needed |
| `types.h` | Data types | No changes needed |
| `Makefile` | Build config | Add Emscripten target |

---

## File Locations Reference

```
/Users/nathan/Developments/Pikafish/src/main.cpp                    # Main entry point
/Users/nathan/Developments/Pikafish/src/uci.cpp                     # UCI command loop
/Users/nathan/Developments/Pikafish/src/uci.h                       # UCI interface
/Users/nathan/Developments/Pikafish/src/engine.cpp                  # Engine implementation
/Users/nathan/Developments/Pikafish/src/engine.h                    # Engine interface
/Users/nathan/Developments/Pikafish/src/position.cpp                # Board representation
/Users/nathan/Developments/Pikafish/src/position.h                  # Position interface
/Users/nathan/Developments/Pikafish/src/types.h                     # Type definitions & Move class
/Users/nathan/Developments/Pikafish/src/search.cpp                  # Search algorithm
/Users/nathan/Developments/Pikafish/src/search.h                    # Search structures
/Users/nathan/Developments/Pikafish/src/Makefile                    # Build configuration
/Users/nathan/Developments/Pikafish/.github/workflows/pikafish.yml  # CI/CD config
```

---

## Summary

Pikafish is a **mature, well-architected C++ chess engine** for xiangqi that:

1. ✓ Has complete UCI protocol support
2. ✓ Uses modern C++17/20
3. ✓ Has callback-based architecture suitable for async operations
4. ✓ Supports multiple platforms (Linux, Windows, macOS, Android)
5. ✓ Contains no existing WASM implementation (clean slate for web)
6. ✓ Has clear separation between engine logic and I/O
7. ✓ Already has position/move APIs suitable for programmatic use

**For WebAssembly compilation:** The codebase is ideal for Emscripten compilation with minimal modifications. The Engine class already provides all necessary APIs for a JavaScript wrapper.

