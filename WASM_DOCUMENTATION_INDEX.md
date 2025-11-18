# Pikafish WebAssembly/Emscripten Exploration - Documentation Index

## Overview

This directory contains a comprehensive analysis of the Pikafish xiangqi engine codebase for WebAssembly (WASM) compilation and JavaScript integration. Four documents provide different levels of detail for different audiences.

---

## Documents

### 1. WASM_EXECUTIVE_SUMMARY.md (START HERE)
**Audience:** Project managers, architects, decision makers

**Contents:**
- High-level findings and verdict
- Key technical decisions needed
- Timeline and effort estimates
- Risk assessment and mitigation
- Success criteria

**Length:** 8.2 KB (10 min read)

**Key Takeaway:** Pikafish is production-ready for WASM compilation with 10-14 days of development effort.

---

### 2. WASM_EXPLORATION_REPORT.md (DETAILED TECHNICAL)
**Audience:** C++ developers, architects, engineers

**Contents:**
- Detailed codebase structure (48 files, 14,433 LOC)
- UCI protocol analysis and implementation
- Build system breakdown (Makefile, architectures)
- Position/Move representation (xiangqi-specific)
- NNUE neural network details
- File-by-file location reference

**Length:** 18 KB (30 min read)

**Key Takeaway:** Complete technical inventory of the codebase with exact file locations and implementation details.

---

### 3. WASM_ARCHITECTURE.md (IMPLEMENTATION GUIDE)
**Audience:** Web developers, integration engineers

**Contents:**
- High-level architecture diagrams
- Data flow from JavaScript to best move
- Search callback infrastructure
- JavaScript API design patterns
- Build process (target state)
- Memory considerations
- Threading strategies (Web Workers)
- Code examples for Emscripten bindings

**Length:** 11 KB (20 min read)

**Key Takeaway:** How to wire JavaScript to the C++ engine, with code examples and design patterns.

---

### 4. WASM_QUICK_REFERENCE.md (DEVELOPER CHEAT SHEET)
**Audience:** Developers actively implementing WASM integration

**Contents:**
- Xiangqi rules (9x10 board, 7 piece types)
- Move encoding (16-bit format)
- Critical file locations
- Search callback structures
- Emscripten compilation flags
- Common issues and solutions
- Success criteria checklist
- Implementation roadmap (4 phases)

**Length:** 9.7 KB (15 min read)

**Key Takeaway:** Rapid lookup guide with code snippets and troubleshooting tips.

---

## Recommended Reading Order

### For Project Planning
1. WASM_EXECUTIVE_SUMMARY.md (10 min)
2. Timeline section of WASM_QUICK_REFERENCE.md (5 min)

### For Architecture & Design
1. WASM_EXECUTIVE_SUMMARY.md (10 min)
2. WASM_ARCHITECTURE.md (20 min)
3. Key Integration Points section of WASM_QUICK_REFERENCE.md (5 min)

### For Development
1. WASM_QUICK_REFERENCE.md (15 min)
2. Critical Files section of WASM_EXPLORATION_REPORT.md (5 min)
3. Reference sections as needed during implementation

### For Complete Understanding
1. WASM_EXECUTIVE_SUMMARY.md (10 min) - Context
2. WASM_EXPLORATION_REPORT.md (30 min) - Details
3. WASM_ARCHITECTURE.md (20 min) - Integration approach
4. WASM_QUICK_REFERENCE.md (15 min) - Quick lookup

---

## Key Facts Summary

### Codebase
- **Language:** C++17/20
- **Size:** 14,433 lines of code
- **Files:** 48 source files
- **Purpose:** UCI xiangqi engine
- **Platform Support:** Linux, Windows, macOS, Android (proven portability)

### WASM Status
- **Current WASM Config:** None exists
- **Blockers:** Zero identified
- **SIMD Compatibility:** Yes (zstd library references)
- **Threading:** Can use Web Workers

### Architecture
- **Core Engine:** No platform assumptions
- **Callbacks:** Already implemented (perfect for web)
- **FEN Interface:** Ready for programmatic use
- **NNUE Weights:** ~15-20 MB neural network files

### Implementation
- **Files to Create:** 2 new files (wasm_api.h/cpp)
- **Files to Modify:** 1 file (Makefile)
- **Effort:** 10-14 days for production-ready extension
- **Key Skills Needed:** C++, Emscripten, JavaScript, Web APIs

---

## Key Findings

### What You Get
- Complete xiangqi analysis engine
- Real-time search with progress callbacks
- NNUE-based evaluation (strong play)
- Multi-threaded search (with Web Workers)
- Full UCI command support

### What You Need to Build
- Emscripten wrapper (wasm_api.cpp)
- JavaScript binding layer
- Web extension integration
- NNUE weights management

### What Doesn't Need to Change
- Core search algorithm
- Move generation
- Position representation
- Evaluation logic
- Any platform-independent code

---

## Critical File Locations

```
Engine Interface:
  /src/engine.h              - Main class (has all callbacks)
  /src/engine.cpp            - Implementation

Position & Moves:
  /src/position.h            - Board representation
  /src/types.h               - Move encoding, piece types
  
Search:
  /src/search.h              - Search structures (InfoFull, etc.)
  /src/search.cpp            - Search algorithm

NNUE:
  /src/nnue/network.h        - Neural network interface
  /src/nnue/nnue_*.h         - Network layers and features

UCI (for reference):
  /src/uci.cpp/h             - Command parsing

Build:
  /src/Makefile              - Build configuration
```

---

## Questions Answered by Documents

### Executive Summary Answers:
- Is this codebase suitable for WASM?
- How much effort will it take?
- What are the risks?
- What's the timeline?

### Exploration Report Answers:
- How does the UCI protocol work?
- What does the codebase structure look like?
- How are positions and moves represented?
- What's the build system?

### Architecture Guide Answers:
- How do I wire JavaScript to C++?
- What are the callbacks?
- How should I structure the WASM module?
- What about threading?

### Quick Reference Answers:
- How do I compile with Emscripten?
- What are common issues?
- Where are the critical files?
- What's my implementation roadmap?

---

## Next Steps

1. **Review:** Read WASM_EXECUTIVE_SUMMARY.md
2. **Decide:** Clarify design decisions (threading, weights, etc.)
3. **Plan:** Use timeline estimates for project planning
4. **Build:** Create wasm_api.cpp using WASM_ARCHITECTURE.md as guide
5. **Reference:** Use WASM_QUICK_REFERENCE.md during development
6. **Debug:** Troubleshoot using Common Issues section

---

## Important Notes

### About Xiangqi
This is **NOT** a standard chess engine. Xiangqi (Chinese chess) has:
- Different board size (9x10 vs 8x8)
- Different pieces and rules
- Different notation (a0a1 vs e2e4)
- Different evaluation metrics

**Make sure your web extension UI correctly reflects xiangqi rules, not chess!**

### About NNUE Weights
The engine requires neural network weights to play well:
- File size: ~15-20 MB
- Must be loaded at startup
- Strategy: Embed, load from CDN, or cache locally
- Decision needed before implementation

### About Threading
In web context, threading options:
- **Single-threaded:** Simpler, slower
- **Web Workers:** Faster, more complex
- **Hybrid:** Web Worker pool with queue
- Decision impacts architecture

---

## Document Statistics

| Document | Size | Words | Read Time |
|----------|------|-------|-----------|
| Executive Summary | 8.2 KB | 2,100 | 10 min |
| Exploration Report | 18 KB | 5,200 | 30 min |
| Architecture Guide | 11 KB | 3,500 | 20 min |
| Quick Reference | 9.7 KB | 2,800 | 15 min |
| **Total** | **46.9 KB** | **13,600** | **75 min** |

---

## Questions?

Each document is self-contained. If you need specific information:

- **"Will this work in the browser?"** → Executive Summary
- **"How does move generation work?"** → Exploration Report section 5
- **"How do I create callbacks?"** → Architecture Guide section 1
- **"What's the Emscripten syntax?"** → Quick Reference

---

## Version History

- **v1.0** (Nov 18, 2025): Initial comprehensive exploration
  - Full codebase analysis
  - Architecture assessment
  - Implementation guidance
  - Quick reference guide

---

**Date:** November 18, 2025
**Status:** Complete - Ready for implementation
**Next Review:** After WASM wrapper implementation (Phase 1)

