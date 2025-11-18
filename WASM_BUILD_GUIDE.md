# Pikafish WebAssembly Build Guide

## Overview

This guide explains how to build Pikafish as a WebAssembly module for use in web browsers and browser extensions.

## What Was Added

### 1. GitHub Workflow (`.github/workflows/pikafish-wasm.yml`)

A complete CI/CD workflow that:
- Downloads the NNUE network file from GitHub releases
- Compiles Pikafish to WebAssembly using Emscripten
- Creates both SIMD and basic (compatible) versions
- Generates JavaScript wrapper and TypeScript definitions
- Packages everything as a ready-to-use distribution
- Automatically uploads artifacts on every push

### 2. WASM API Wrapper (`src/wasm_api.cpp`)

A C++ wrapper that exposes Pikafish functionality to JavaScript:
- **Position management**: Set positions via FEN, handle move lists
- **Search control**: Search by depth, time, nodes, or infinite
- **Real-time callbacks**: Get search progress and results
- **Engine configuration**: Set hash size, threads, options
- **Utility functions**: Visualize positions, run perft tests

## How to Use the Workflow

### Trigger Build

The workflow runs automatically on:
- Every push to `master` branch
- Every pull request to `master` branch
- Manual trigger via GitHub Actions UI

### Download Built Artifacts

1. Go to your repository's "Actions" tab
2. Click on the latest "Pikafish-WASM" workflow run
3. Download the "Pikafish-WASM" artifact (contains all files)

### Files in Distribution

```
Pikafish-WASM/
├── pikafish-simd.js         # SIMD-optimized JS glue code
├── pikafish-simd.wasm       # SIMD-optimized WASM binary
├── pikafish-basic.js        # Basic JS glue code
├── pikafish-basic.wasm      # Basic WASM binary (wider compatibility)
├── pikafish-wrapper.js      # High-level JavaScript API
├── pikafish-wrapper.d.ts    # TypeScript definitions
├── example.html             # Working demo
└── WASM-README.md           # Usage instructions
```

## Using in Your Web Extension

### Option 1: Basic Integration

```javascript
// In your extension's background script or content script
import { PikafishEngine } from './pikafish-wrapper.js';

const engine = new PikafishEngine();
await engine.init('./pikafish-simd.js');

// Analyze a position
const fen = 'rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1';
const bestMove = await engine.analyze(fen, 15);
console.log('Best move:', bestMove);
```

### Option 2: Advanced Usage with Callbacks

```javascript
import createPikafishModule from './pikafish-simd.js';

// Initialize module
const Module = await createPikafishModule();
const engine = new Module.PikafishWASM();
engine.init();

// Set up callbacks for real-time updates
engine.setOnUpdate((info) => {
  console.log(`Depth: ${info.depth}, Nodes: ${info.nodes}, Score: ${info.score}`);
});

engine.setOnBestMove((bestMove, ponder) => {
  console.log(`Best move: ${bestMove}, Ponder: ${ponder}`);
});

// Set position and start search
engine.setPosition('rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1');
engine.goDepth(20);

// Stop search anytime
// engine.stop();
```

### Option 3: Web Worker for Better Performance

```javascript
// worker.js
importScripts('./pikafish-simd.js');

let engine;

createPikafishModule().then(Module => {
  engine = new Module.PikafishWASM();
  engine.init();

  engine.setOnBestMove((bestMove, ponder) => {
    postMessage({ type: 'bestmove', bestMove, ponder });
  });

  postMessage({ type: 'ready' });
});

onmessage = (e) => {
  switch (e.data.type) {
    case 'analyze':
      engine.setPosition(e.data.fen);
      engine.goDepth(e.data.depth);
      break;
    case 'stop':
      engine.stop();
      break;
  }
};
```

```javascript
// main.js
const worker = new Worker('worker.js');

worker.onmessage = (e) => {
  if (e.data.type === 'ready') {
    // Engine is ready
    worker.postMessage({
      type: 'analyze',
      fen: 'rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1',
      depth: 20
    });
  } else if (e.data.type === 'bestmove') {
    console.log('Best move:', e.data.bestMove);
  }
};
```

## Web Extension manifest.json Setup

### For Manifest V3 (Chrome/Edge)

```json
{
  "manifest_version": 3,
  "name": "Xiangqi Assistant",
  "version": "1.0.0",
  "description": "Xiangqi analysis powered by Pikafish",

  "background": {
    "service_worker": "background.js",
    "type": "module"
  },

  "content_scripts": [{
    "matches": ["<all_urls>"],
    "js": ["content.js"]
  }],

  "web_accessible_resources": [{
    "resources": [
      "pikafish-simd.js",
      "pikafish-simd.wasm",
      "pikafish-basic.js",
      "pikafish-basic.wasm"
    ],
    "matches": ["<all_urls>"]
  }],

  "permissions": [
    "storage"
  ]
}
```

### For Manifest V2 (Firefox)

```json
{
  "manifest_version": 2,
  "name": "Xiangqi Assistant",
  "version": "1.0.0",
  "description": "Xiangqi analysis powered by Pikafish",

  "background": {
    "scripts": ["background.js"]
  },

  "content_scripts": [{
    "matches": ["<all_urls>"],
    "js": ["content.js"]
  }],

  "web_accessible_resources": [
    "pikafish-simd.js",
    "pikafish-simd.wasm",
    "pikafish-basic.js",
    "pikafish-basic.wasm"
  ],

  "permissions": [
    "storage"
  ]
}
```

## Complete API Reference

### PikafishWASM Class Methods

#### Initialization
- `init()` - Initialize engine and load networks

#### Position Management
- `setPosition(fen: string)` - Set position from FEN
- `setPositionWithMoves(fen: string, moves: string[])` - Set position with move history
- `getFen(): string` - Get current position as FEN
- `visualize(): string` - Get ASCII board visualization

#### Search Control
- `goDepth(depth: number)` - Search to specified depth
- `goTime(timeMs: number)` - Search for specified time in milliseconds
- `goNodes(nodes: number)` - Search specified number of nodes
- `goInfinite()` - Start infinite search (call `stop()` to end)
- `stop()` - Stop current search
- `waitForSearchFinished()` - Block until search completes

#### Results
- `getBestMove(): string` - Get best move from last search
- `getPonderMove(): string` - Get ponder move from last search
- `isSearching(): boolean` - Check if currently searching

#### Configuration
- `setOption(name: string, value: string)` - Set engine option
- `setHashSize(mb: number)` - Set hash table size in megabytes
- `setThreads(count: number)` - Set number of search threads
- `clearHash()` - Clear transposition table
- `getHashFull(): number` - Get hash table usage percentage

#### Callbacks
- `setOnUpdate(callback: function)` - Set callback for search updates
  - Receives object: `{depth, seldepth, time, nodes, score, hashfull, nps, tbhits}`
- `setOnBestMove(callback: function)` - Set callback for best move
  - Receives: `(bestMove: string, ponder: string)`

#### Testing
- `perft(fen: string, depth: number): number` - Run perft test

## Performance Considerations

### SIMD vs Basic Version
- **SIMD version**: 2-3x faster, requires Chrome 91+, Firefox 89+, Safari 16.4+
- **Basic version**: Slower but works on any browser with WASM support
- Auto-detect browser capabilities and load appropriate version

### Memory Settings
- Default hash: 16MB (configurable via `setHashSize()`)
- Increase for stronger analysis: `engine.setHashSize(256)` for 256MB
- Browser memory limit typically ~2GB

### Threading
- Web Workers: Recommended for UI responsiveness
- WASM threads: Available but requires SharedArrayBuffer (security headers needed)
- Single-threaded: Simpler, works everywhere

### NNUE File Handling
- Embedded in WASM (~30-35MB total size)
- No separate file loading needed
- First load may be slow, subsequent loads use browser cache

## Building Locally

If you want to build manually instead of using GitHub Actions:

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Get NNUE file
cd /path/to/Pikafish/src
wget https://github.com/official-pikafish/Networks/releases/download/master-net/pikafish.nnue

# Build WASM
SOURCES=$(find . -name "*.cpp" -not -name "main.cpp" | tr '\n' ' ')

em++ -std=c++20 \
  -O3 \
  -msimd128 \
  -msse -msse2 -msse3 -mssse3 -msse4.1 \
  -DUSE_SSE41 -DUSE_SSSE3 -DUSE_SSE2 \
  -DNDEBUG \
  --bind \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=256MB \
  -s MAXIMUM_MEMORY=2GB \
  -s EXPORT_ES6=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="createPikafishModule" \
  -s ENVIRONMENT=web,worker \
  --embed-file pikafish.nnue@/pikafish.nnue \
  -o pikafish.js \
  $SOURCES
```

## Troubleshooting

### "Module not found" errors
- Ensure WASM files are in `web_accessible_resources`
- Use correct relative paths
- Check Content-Security-Policy headers

### Search never completes
- Make sure to set up `setOnBestMove()` callback
- Check if search was stopped prematurely
- Verify position FEN is valid

### Out of memory errors
- Reduce hash size: `engine.setHashSize(16)`
- Use basic version instead of SIMD
- Limit search depth

### Slow performance
- Use SIMD version if browser supports it
- Run engine in Web Worker
- Increase hash size for longer analyses

## License

Pikafish is licensed under GPL-3.0. Your web extension must also be GPL-3.0 compatible.

## Support

For issues with:
- **Pikafish engine**: https://github.com/official-pikafish/Pikafish/issues
- **WASM build**: Create issue with `[WASM]` prefix
- **Your extension**: Your own repository

## Next Steps

1. Download the WASM distribution from GitHub Actions
2. Test with the included `example.html`
3. Integrate into your web extension
4. Consider using Web Worker for better UX
5. Test on different browsers for compatibility
