/*
  Pikafish WASM API Wrapper
  Provides JavaScript bindings for Pikafish engine compiled to WebAssembly

  Copyright (C) 2024 Pikafish developers
  Licensed under GPL-3.0
*/

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <string>
#include <sstream>
#include <memory>
#include <functional>

#include "engine.h"
#include "search.h"
#include "types.h"

using namespace emscripten;
using namespace Stockfish;

class PikafishWASM {
private:
    std::unique_ptr<Engine> engine;
    std::string lastBestMove;
    std::string lastPonder;
    bool searching;

    // Callbacks for JavaScript
    val onUpdateCallback;
    val onBestMoveCallback;

public:
    PikafishWASM() : searching(false),
                     onUpdateCallback(val::null()),
                     onBestMoveCallback(val::null()) {
        // Engine constructor needs optional path
        engine = std::make_unique<Engine>(std::nullopt);

        // Setup callbacks
        engine->set_on_bestmove([this](std::string_view best, std::string_view ponder) {
            this->lastBestMove = std::string(best);
            this->lastPonder = std::string(ponder);
            this->searching = false;

            if (!this->onBestMoveCallback.isNull()) {
                this->onBestMoveCallback(this->lastBestMove, this->lastPonder);
            }
        });

        engine->set_on_update_full([this](const Engine::InfoFull& info) {
            if (!this->onUpdateCallback.isNull()) {
                val updateInfo = val::object();
                updateInfo.set("depth", info.depth);
                updateInfo.set("seldepth", info.selDepth);
                updateInfo.set("time", info.timeMs);
                updateInfo.set("nodes", static_cast<double>(info.nodes));
                updateInfo.set("score", info.score);
                updateInfo.set("hashfull", info.hashfull);
                updateInfo.set("nps", static_cast<double>(info.nps));
                updateInfo.set("tbhits", static_cast<double>(info.tbHits));

                this->onUpdateCallback(updateInfo);
            }
        });
    }

    ~PikafishWASM() {
        if (searching) {
            engine->stop();
            engine->wait_for_search_finished();
        }
    }

    // Initialize the engine and load networks
    void init() {
        try {
            engine->load_networks();
            engine->verify_networks();
        } catch (const std::exception& e) {
            // Network will be loaded from embedded file
        }
    }

    // Set position from FEN string
    void setPosition(const std::string& fen) {
        std::vector<std::string> moves; // empty moves vector
        engine->set_position(fen, moves);
    }

    // Set position from FEN with move list
    void setPositionWithMoves(const std::string& fen, const std::vector<std::string>& moves) {
        engine->set_position(fen, moves);
    }

    // Start search with depth limit
    void goDepth(int depth) {
        Search::LimitsType limits;
        limits.depth = depth;
        searching = true;
        engine->go(limits);
    }

    // Start search with time limit (milliseconds)
    void goTime(int timeMs) {
        Search::LimitsType limits;
        limits.time[WHITE] = timeMs;
        limits.time[BLACK] = timeMs;
        searching = true;
        engine->go(limits);
    }

    // Start search with nodes limit
    void goNodes(uint64_t nodes) {
        Search::LimitsType limits;
        limits.nodes = nodes;
        searching = true;
        engine->go(limits);
    }

    // Start infinite search
    void goInfinite() {
        Search::LimitsType limits;
        limits.infinite = true;
        searching = true;
        engine->go(limits);
    }

    // Stop current search
    void stop() {
        if (searching) {
            engine->stop();
        }
    }

    // Wait for search to finish
    void waitForSearchFinished() {
        engine->wait_for_search_finished();
    }

    // Get the last best move found
    std::string getBestMove() const {
        return lastBestMove;
    }

    // Get the last ponder move
    std::string getPonderMove() const {
        return lastPonder;
    }

    // Check if currently searching
    bool isSearching() const {
        return searching;
    }

    // Get current position as FEN
    std::string getFen() const {
        return engine->fen();
    }

    // Visualize current position (for debugging)
    std::string visualize() const {
        return engine->visualize();
    }

    // Set option value
    void setOption(const std::string& name, const std::string& value) {
        auto& options = engine->get_options();
        if (options.count(name)) {
            options[name] = value;
        }
    }

    // Get hash usage percentage
    int getHashFull() const {
        return engine->get_hashfull();
    }

    // Clear hash table
    void clearHash() {
        engine->search_clear();
    }

    // Set hash size in MB
    void setHashSize(size_t mb) {
        engine->set_tt_size(mb);
    }

    // Set number of threads
    void setThreads(int numThreads) {
        setOption("Threads", std::to_string(numThreads));
        engine->resize_threads();
    }

    // JavaScript callback setters
    void setOnUpdate(val callback) {
        onUpdateCallback = callback;
    }

    void setOnBestMove(val callback) {
        onBestMoveCallback = callback;
    }

    // Perft for testing
    uint64_t perft(const std::string& fen, int depth) {
        return engine->perft(fen, depth);
    }
};

EMSCRIPTEN_BINDINGS(pikafish_module) {
    // Register vector<string> for move lists
    register_vector<std::string>("VectorString");

    // Main API class
    class_<PikafishWASM>("PikafishWASM")
        .constructor<>()
        .function("init", &PikafishWASM::init)
        .function("setPosition", &PikafishWASM::setPosition)
        .function("setPositionWithMoves", &PikafishWASM::setPositionWithMoves)
        .function("goDepth", &PikafishWASM::goDepth)
        .function("goTime", &PikafishWASM::goTime)
        .function("goNodes", &PikafishWASM::goNodes)
        .function("goInfinite", &PikafishWASM::goInfinite)
        .function("stop", &PikafishWASM::stop)
        .function("waitForSearchFinished", &PikafishWASM::waitForSearchFinished)
        .function("getBestMove", &PikafishWASM::getBestMove)
        .function("getPonderMove", &PikafishWASM::getPonderMove)
        .function("isSearching", &PikafishWASM::isSearching)
        .function("getFen", &PikafishWASM::getFen)
        .function("visualize", &PikafishWASM::visualize)
        .function("setOption", &PikafishWASM::setOption)
        .function("getHashFull", &PikafishWASM::getHashFull)
        .function("clearHash", &PikafishWASM::clearHash)
        .function("setHashSize", &PikafishWASM::setHashSize)
        .function("setThreads", &PikafishWASM::setThreads)
        .function("setOnUpdate", &PikafishWASM::setOnUpdate)
        .function("setOnBestMove", &PikafishWASM::setOnBestMove)
        .function("perft", &PikafishWASM::perft);
}
