# Pikafish Architecture Summary for WASM Integration

## High-Level Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     JavaScript/Browser                       │
│              (Web Extension or Web Application)              │
└────────────────────────┬────────────────────────────────────┘
                         │
        ┌────────────────▼────────────────┐
        │   Emscripten Binding Layer      │
        │  (To be created - wrapper API)  │
        └────────────────┬────────────────┘
                         │
    ┌────────────────────▼────────────────────────┐
    │    Pikafish Engine (C++ compiled to WASM)  │
    │                                            │
    │  ┌──────────────────────────────────────┐ │
    │  │  Engine Class                        │ │
    │  │  - go(limits)                        │ │
    │  │  - set_position(fen, moves)          │ │
    │  │  - set_on_bestmove(callback)         │ │
    │  │  - set_on_update_full(callback)      │ │
    │  └──────────────────────────────────────┘ │
    │                  │                        │
    │  ┌───────┬───────┴────────┬────────┐    │
    │  │       │                │        │    │
    │  ▼       ▼                ▼        ▼    │
    │ Search Position Move   Evaluate  NNUE   │
    │  Algo   Logic    Gen   (NNUE)  Network  │
    │                                        │
    │  ┌──────────────────────────────────────┐ │
    │  │  Supporting Systems                  │ │
    │  │  - Threading (Web Workers)           │ │
    │  │  - Transposition Table               │ │
    │  │  - Bitboard Lookups                  │ │
    │  │  - History Heuristics                │ │
    │  └──────────────────────────────────────┘ │
    │                                            │
    └────────────────────────────────────────────┘
```

## Data Flow: Position to Best Move

```
JavaScript
    │
    ├─ Input: FEN string + move list
    │         (e.g., "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w")
    │
    ▼
Engine.set_position(fen, moves)
    │
    ├─ Parse FEN
    ├─ Initialize Position object
    ├─ Apply move sequence
    │
    ▼
Engine.go(limits)
    │
    ├─ Initialize search
    ├─ Call search algorithm with callbacks
    │
    ▼
Search Algorithm
    │
    ├─ Generate legal moves (MovGen)
    ├─ Evaluate positions (NNUE neural network)
    ├─ Alpha-beta pruning
    ├─ Transposition table lookup
    │
    ├─ [Invoke callback for each iteration]
    │     InfoIteration { depth, nodes }
    │
    ├─ [Invoke callback for position updates]
    │     InfoFull { depth, score, pv, nodes, time }
    │
    │ (Repeat until depth limit or time expires)
    │
    ▼
Engine.on_bestmove() callback
    │
    ├─ Best move: string format (e.g., "e2e4")
    ├─ Ponder move: string format (optional)
    │
    ▼
JavaScript receives results
```

## Key Integration Points for WASM

### 1. Search Callback Flow

```cpp
// In C++:
engine.set_on_update_full([this](const auto& info) {
    // Called multiple times during search
    // info contains: depth, score, nodes, time, pv (best line)
    // Invoked asynchronously as search progresses
});

engine.set_on_bestmove([this](std::string_view move, std::string_view ponder) {
    // Called once at end of search
    // move: best move (e.g., "e2e4")
    // ponder: expected reply (optional)
});
```

### 2. Position Representation

**FEN String Format (Xiangqi):**
```
rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w
```

- First 9 rows: Board layout (/) separated
- Last part: Side to move (w = white/red, b = black)
- Pieces: R=Rook, A=Advisor, B=Bishop, N=Knight, C=Cannon, P=Pawn, K=King

**Move Notation (UCI):**
```
e2e4  - Move from e2 (square 76) to e4 (square 58)
a0a1  - Move from a0 (square 0) to a1 (square 9)
```

### 3. Search Limits (from JavaScript)

```cpp
struct LimitsType {
    int depth;           // e.g., 20 for 20 ply deep search
    int movetime;        // e.g., 5000 for 5 seconds per move
    int nodes;           // e.g., 1000000 for max 1M nodes
    bool infinite;       // Search until stop() called
};
```

**JavaScript to C++ Example:**
```javascript
// JavaScript side
engine.go({
    movetime: 3000,  // 3 seconds
    depth: 20        // or 20 plies deep
});
```

## Critical Files for WASM Wrapper

### Must Modify/Create:

1. **New File: `src/wasm_api.h`** (Create Emscripten binding)
   ```cpp
   #ifndef WASM_API_H_INCLUDED
   #define WASM_API_H_INCLUDED
   
   #include <emscripten/bind.h>
   #include "engine.h"
   
   class WasmEngine {
       Engine engine;
   public:
       WasmEngine();
       void set_position(std::string fen, std::vector<std::string> moves);
       void go(int depth, int movetime, int nodes, bool infinite);
       void stop();
       std::string get_fen() const;
       // ... JavaScript callback registration
   };
   
   EMSCRIPTEN_BINDINGS(pikafish) {
       emscripten::class_<WasmEngine>("PikafishEngine")
           .constructor<>()
           .function("setPosition", &WasmEngine::set_position)
           .function("go", &WasmEngine::go)
           .function("stop", &WasmEngine::stop)
           .function("getFen", &WasmEngine::get_fen);
   }
   
   #endif
   ```

2. **New File: `Makefile.emscripten`** (Build configuration)
   ```makefile
   EMSCRIPTEN_FLAGS = -s WASM=1 -s ALLOW_MEMORY_GROWTH=1
   EMSCRIPTEN_FLAGS += -s EXPORTED_FUNCTIONS="['_malloc','_free']"
   EMSCRIPTEN_FLAGS += --bind
   
   .PHONY: wasm-build
   wasm-build:
       emcc $(EMSCRIPTEN_FLAGS) src/*.cpp src/nnue/*.cpp \
            -o pikafish.js
   ```

3. **Modify: `src/Makefile`**
   - Add emscripten target
   - Conditional compilation for WASM

### No Changes Needed:

- `engine.cpp/h` - Already has callback infrastructure
- `position.cpp/h` - FEN interface already works
- `search.cpp/h` - No platform assumptions
- `types.h` - Pure C++
- `nnue/` - Pure computation, no I/O

## JavaScript API Example (Target)

```javascript
// Import compiled WASM module
import Module from './pikafish.js';

class PikafishJS {
    constructor() {
        this.engine = new Module.PikafishEngine();
        this.onUpdate = null;
        this.onBestMove = null;
    }
    
    async analyze(fen, movelist, options = {}) {
        const { depth = 20, movetime = 3000 } = options;
        
        // Set position
        this.engine.setPosition(fen, movelist);
        
        // Register callbacks
        this.engine.onUpdate = (info) => {
            if (this.onUpdate) {
                this.onUpdate({
                    depth: info.depth,
                    score: info.score,
                    nodes: info.nodes,
                    pv: info.pv
                });
            }
        };
        
        this.engine.onBestMove = (move, ponder) => {
            if (this.onBestMove) {
                this.onBestMove({ move, ponder });
            }
        };
        
        // Start search
        return new Promise((resolve) => {
            const originalCallback = this.onBestMove;
            this.onBestMove = (result) => {
                this.onBestMove = originalCallback;
                resolve(result);
            };
            
            this.engine.go(depth, movetime);
        });
    }
    
    stop() {
        this.engine.stop();
    }
}

// Usage
const analyzer = new PikafishJS();
analyzer.onUpdate = (info) => {
    console.log(`Depth ${info.depth}: ${info.score}cp, ${info.nodes} nodes`);
};

const result = await analyzer.analyze(
    "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w",
    [],
    { depth: 20, movetime: 5000 }
);
console.log(`Best move: ${result.move}`);
```

## Build Process (Target)

```bash
# Compile Pikafish to WebAssembly
emcripten-build:
    cd src
    emcc -o ../pikafish.js wasm_api.cpp \
        $(shell find . -name '*.cpp' -not -path './external/*') \
        $(shell find ./external/decompress -name '*.cpp') \
        $(shell find ./external/common -name '*.cpp') \
        -s WASM=1 \
        -s ALLOW_MEMORY_GROWTH=1 \
        -O3 \
        --bind \
        -std=c++20

# Create web extension
npm run build
```

## Memory Considerations

| Component | Typical Size |
|-----------|--------------|
| Engine binary (WASM) | 3-5 MB |
| NNUE weights (.nnue) | 15-20 MB |
| Transposition table (Hash) | Configurable (16-1024 MB) |
| Total baseline | ~20-30 MB |

**Strategy:**
- Include small engine + NNUE in WASM bundle
- Make hash table configurable (default 16-64 MB)
- Use shared memory for multi-threaded scenarios
- Consider splitting large NNUE weights into separate files

## Threading in WASM

**Option 1: Web Workers (Recommended)**
```javascript
// main.js
const worker = new Worker('pikafish-worker.js');
worker.postMessage({ cmd: 'analyze', fen, moves, depth: 20 });
worker.onmessage = (e) => {
    console.log(e.data);  // { move, score, depth, ... }
};
```

**Option 2: Main Thread (Simpler)**
```javascript
// Single-threaded, blocks UI but simpler
const result = await engine.analyze(fen, moves, { depth: 20 });
```

---

## Summary Table: Current vs. Target

| Aspect | Current | Target for WASM |
|--------|---------|-----------------|
| **Entry Point** | `main.cpp` CLI | `wasm_api.cpp` C++/JS binding |
| **Input** | stdin (UCI commands) | FEN string + move array |
| **Output** | stdout (UCI output) | Callback functions |
| **Threading** | POSIX/Windows threads | Web Workers (optional) |
| **Binary Format** | Native executable | WebAssembly (.wasm) |
| **JavaScript** | None | Emscripten-generated wrapper |
| **Build System** | Makefile | Makefile + Emscripten compiler |
| **Network files** | Disk I/O (.nnue files) | Embedded or pre-downloaded |

---

## Next Steps for Implementation

1. Create `src/wasm_api.h` with Emscripten bindings
2. Create `src/wasm_api.cpp` with callback implementations
3. Add Emscripten build target to Makefile
4. Implement JavaScript wrapper class
5. Create web extension integration
6. Handle NNUE weight loading/caching
7. Test with browser compatibility

The codebase is well-suited for this integration with minimal modifications needed!
