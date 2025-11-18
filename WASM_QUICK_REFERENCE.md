# Pikafish WASM Integration Quick Reference

## Key Findings at a Glance

### Status Checklist
- [x] Codebase explored: 14,433 lines of C++ across 48 files
- [ ] WebAssembly build: NOT YET IMPLEMENTED
- [ ] JavaScript bindings: NOT YET IMPLEMENTED
- [x] UCI Protocol: FULLY IMPLEMENTED
- [x] Architecture: WASM-READY (minimal changes needed)

### What You Need to Know

#### 1. The Engine is Xiangqi, Not Chess
- **Board size:** 9x10 (90 squares)
- **Piece types:** Rook, Advisor, Cannon, Pawn, Knight, Bishop, King
- **Uses FEN notation:** Xiangqi-specific (not standard chess FEN)
- **Example position:** `rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w`

#### 2. Perfect for Web: Callback-Based Architecture
```cpp
// The Engine class ALREADY has these callback methods:
void set_on_update_full(std::function<void(const InfoFull&)>&&);
void set_on_bestmove(std::function<void(std::string_view, std::string_view)>&&);
void set_on_iter(std::function<void(const InfoIter&)>&&);
```

**Translation:** You can receive updates from the search as it progresses - perfect for web progress bars, live score updates, etc.

#### 3. Simple Input/Output Interface
**Input (from JavaScript):**
- FEN string (board position)
- Move list (game history)
- Search limits (depth, time, nodes)

**Output (to JavaScript):**
- Best move (e.g., "e2e4")
- Search statistics (depth, score, nodes, time)
- Principal variation (best line of play)

#### 4. Zero Platform Dependencies (for core logic)
- No file I/O in core search
- No direct platform calls
- Threading can use Web Workers
- Perfect for Emscripten compilation

---

## Critical Files to Know

### Your Starting Points
```
/Users/nathan/Developments/Pikafish/src/
├── engine.h                # The class you'll wrap
├── types.h                 # Move and Position types
├── uci.cpp                 # How UCI protocol works
└── position.h              # Board representation
```

### Size: Main Components
- **Search:** 74,276 bytes (search.cpp)
- **Position:** 38,425 bytes (position.cpp)
- **UCI:** 21,077 bytes (uci.cpp)
- **Bitboard:** 9,729 bytes (bitboard.cpp)
- **NNUE:** Neural network evaluation (nnue/ directory)

---

## Move Encoding (Important!)

**16-bit format:**
```
Bits 0-6:   Destination square (0-89)
Bits 7-13:  Origin square (0-89)

Example: "e2e4"
- e2 (square 76 = 0x4C) -> left-shift 7 -> 0x2600
- e4 (square 58 = 0x3A) -> OR -> 0x263A = 9786
```

**In JavaScript:** Convert to/from "e2e4" string format

---

## Search Callback Structures

### InfoFull (sent multiple times during search)
```cpp
struct InfoFull {
    int      depth;           // Search depth (1-20+)
    int      seldepth;        // Selective depth
    int      multipv;         // Multi-principal variation
    Value    score;           // Evaluation in centipawns
    bool     upperbound;      // Score is upper bound
    bool     lowerbound;      // Score is lower bound
    uint64_t nodes;           // Nodes evaluated
    uint64_t time;            // Time elapsed (ms)
    std::vector<Move> pv;     // Principal variation (best moves)
    int      hashfull;        // Transposition table %
};
```

### Best Move (sent once at end)
```cpp
// Callback receives:
// - move: std::string_view (e.g., "e2e4")
// - ponder: std::string_view (expected reply)
```

---

## NNUE Neural Network Weights

**Important:** The engine uses neural network evaluation. The weights come from `.nnue` files.

**Default files:**
- Main network: Usually embedded or loaded from disk
- File extension: `.nnue` (binary format)
- Size: ~15-20 MB for full network

**Strategy for WASM:**
1. **Option A:** Embed weights in WASM (increases binary size)
2. **Option B:** Load from CDN/server on first run
3. **Option C:** Use IndexedDB to cache weights in browser

---

## Testing the C++ Code First

**Before WASM, verify the engine works:**

```bash
# Navigate to source directory
cd /Users/nathan/Developments/Pikafish/src

# Build the native binary
make -j build

# Run the engine with UCI protocol
./pikafish

# Test UCI commands:
# > uci
# > position fen rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w
# > go depth 15
# > quit
```

---

## Emscripten Compilation Checklist

### Prerequisites
```bash
# Install Emscripten (if not already done)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### Compilation Steps (Target)
1. Create `src/wasm_api.cpp` (wrapper class)
2. Create `src/wasm_api.h` (Emscripten bindings)
3. Update `src/Makefile` with Emscripten target
4. Run: `emcc [flags] [source files] -o pikafish.js`
5. Include `pikafish.js` + `pikafish.wasm` in extension

### Key Emscripten Flags
```bash
-s WASM=1                          # Generate WebAssembly
-s ALLOW_MEMORY_GROWTH=1           # Let memory grow
--bind                              # Enable C++/JS binding
-O3                                # Optimization level
-std=c++20                         # C++ standard
```

---

## Architecture Overview (Simplified)

```
JavaScript (Web Extension)
        ↓
Emscripten Wrapper (wasm_api.cpp)
        ↓
Engine Class (engine.cpp)
        ↓
Search Algorithm (search.cpp)
        ├─ NNUE Evaluation (nnue/)
        ├─ Move Generation (movegen.cpp)
        ├─ Position Logic (position.cpp)
        └─ Transposition Table (tt.cpp)
```

---

## Xiangqi Move Examples

| Notation | From | To | Description |
|----------|------|----|----|
| `a0a1` | a0 (Red rook start) | a1 | Move forward one square |
| `e3e5` | e3 | e5 | Move along file |
| `h0g2` | h0 (Red knight) | g2 | Knight move (L-shaped) |
| `i9i8` | i9 (Black pawn) | i8 | Pawn move forward |

**Key difference from chess:** Xiangqi pawns move differently, kings stay in palace (3x3 area), etc.

---

## Common Integration Issues & Solutions

### Issue: "Move parsing fails"
**Cause:** Using chess notation instead of xiangqi
**Solution:** Learn xiangqi square naming (a-i files, 0-9 ranks)

### Issue: "Callback not firing"
**Cause:** Emscripten function binding issue
**Solution:** Check `EMSCRIPTEN_BINDINGS()` macro syntax

### Issue: "WASM binary too large"
**Cause:** NNUE weights included
**Solution:** Move weights to separate file, load from CDN

### Issue: "Search is too slow"
**Cause:** No SIMD optimization in WASM
**Solution:** Use Emscripten's SIMD support (`-msimd128`) if available

### Issue: "Memory grows unbounded"
**Cause:** Large transposition table
**Solution:** Limit hash table size with engine options

---

## Essential Engine Options

From `engine.h`, these can be configured via JavaScript:

```cpp
Options add:
  - "Hash" (1-33554432 MB) - Transposition table size
  - "Threads" (1-MaxThreads) - Number of search threads
  - "Ponder" (true/false) - Search during opponent's turn
  - "MultiPV" (1-128) - Show N best moves
  - "Move Overhead" (0-5000 ms) - Time buffer
```

**JavaScript equivalent:**
```javascript
engine.setOption("Hash", 256);      // 256 MB hash table
engine.setOption("Threads", 1);     // Single thread (for web)
engine.setOption("MultiPV", 3);     // Show 3 best moves
```

---

## File Structure for Web Extension

### Recommended Layout
```
pikafish-web-extension/
├── src/
│   ├── manifest.json
│   ├── content-script.js      # Injects into xiangqi websites
│   ├── background.js           # Service worker
│   ├── engine-worker.js         # Web Worker for search
│   └── ui.js                    # UI components
├── wasm/
│   ├── pikafish.js             # Emscripten output
│   ├── pikafish.wasm           # Binary module
│   └── networks.nnue           # NNUE weights (optional)
└── dist/
    └── [packaged extension]
```

---

## Next Phase: Implementation Roadmap

1. **Phase 1 - WASM Wrapper (2-3 days)**
   - Create `wasm_api.cpp/h`
   - Emscripten Makefile target
   - Basic JavaScript wrapper

2. **Phase 2 - Web Integration (2-3 days)**
   - Create web extension structure
   - Integrate with xiangqi websites
   - Handle UI updates from callbacks

3. **Phase 3 - Optimization (1-2 days)**
   - Benchmark performance
   - Optimize memory usage
   - Cache NNUE weights

4. **Phase 4 - Polish (1 day)**
   - Error handling
   - Configuration UI
   - Documentation

---

## Resources & Documentation

### Official Pikafish
- GitHub: https://github.com/official-pikafish/Pikafish
- Wiki: https://github.com/official-pikafish/Pikafish/wiki
- UCI Protocol: Fully documented in wiki

### Emscripten
- Docs: https://emscripten.org/docs/
- C++ Binding: https://emscripten.org/docs/porting/connecting_cpp_and_javascript/

### Xiangqi
- Rules: Detailed in /Pikafish.org
- FEN Notation: Check position.h for parsing logic

---

## Questions to Answer Before Starting

1. **Threading:** Should the extension use Web Workers or main thread?
2. **NNUE Weights:** Embed in binary or load from server?
3. **Hash Table:** Default size (16 MB, 64 MB, 256 MB)?
4. **UI Updates:** How frequently should progress be reported?
5. **Browser Support:** Modern browsers only (Chromium 95+) or older?

---

## Success Criteria

- [ ] C++ code compiles to WebAssembly
- [ ] JavaScript can instantiate the engine
- [ ] Can set position via FEN string
- [ ] Can start search with depth/time limits
- [ ] Receives callback updates during search
- [ ] Gets best move with score
- [ ] Reasonable performance (< 5 second delay for depth 15)
- [ ] Works in browser (extension or standalone)

---

## TL;DR

**Pikafish for WASM:** Ready to go with minimal changes. The Engine class already has the callback infrastructure you need. Main work is creating an Emscripten wrapper (wasm_api.cpp) and JavaScript integration. No changes needed to core search/evaluation code. NNUE weights need to be handled separately (embed or load from CDN).

