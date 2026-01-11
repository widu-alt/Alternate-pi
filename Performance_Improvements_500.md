# Alternate-Pi Engine: Optimization Log & Architecture Handoff

**Current Status:** `stable-check` branch
**Performance:** ~515 Games/Second (Simulated)
**Target:** 1,000+ Games/Second

---

## 1. Implemented Optimizations
The following optimizations have been successfully merged and verified.

### A. Algorithmic Improvements
1.  **Graph Tunneling (Constraint Generation):**
    * *Old:* Brute-force iterating A-Z (26 checks) for every empty square.
    * *New:* Using the GADDAG structure to "tunnel" through valid constraints.
    * *Implementation:* `ConstraintGenerator` in `include/fast_constraints.h`.
    
2.  **Zero-Allocation Pipeline:**
    * *Old:* `MoveGenerator` returned a `std::vector<MoveCandidate>`, causing massive heap allocation/deallocation overhead.
    * *New:* Implemented a `Consumer` pattern (Lambda callback). Moves are evaluated on the stack and discarded immediately if they aren't the best.
    * *Implementation:* `MoveGenerator::generate_raw` accepts a generic template callback.

3.  **Templated Scoring (Branch Removal):**
    * *Old:* `calculateTrueScore` used `if(horizontal)` checks inside the hot loop.
    * *New:* Templated functions `calculateTrueScoreFast<bool IS_HORIZONTAL>` generate two distinct assembly paths, removing branching inside the loop.
    * *Implementation:* `include/engine/mechanics.h`.

### B. Low-Level/Memory Optimizations
1.  **Inlined GADDAG Traversal:**
    * *Technique:* Moved Dictionary logic to `include/engine/dictionary.h`.
    * *Detail:* Exposed raw pointers (`nodePtr`, `childPtr`) to bypass `std::vector::operator[]` bounds checking. Added `__builtin_popcount` / `std::countr_zero` for bitmask traversal.

2.  **Direct Heuristic Access:**
    * *Technique:* Replaced function calls in `Heuristics::getTileValue` with direct array lookups (`TILE_VALUES[]`).
    * *Detail:* Removed `toupper` and safety checks in the hot path.

3.  **Build Configuration:**
    * Enabled `-O3` (Release Mode).
    * Enabled `-flto` (Link Time Optimization).
    * Enabled `-march=native` (AVX2 support).

---

## 2. Architectural Boundaries (STRICT)
The codebase enforces a strict separation of concerns. **Future modifications must respect this.**

### The Engine Namespace (`src/engine/`, `include/engine/`)
* **Role:** The "Physics" of the game.
* **Responsibilities:** Board state, tile management, mechanics (placing words), rules enforcement (Referee).
* **Constraint:** *Must not depend on AI logic.*

### The Spectre Namespace (`src/spectre/`, `include/spectre/`)
* **Role:** The "Brain" (AI).
* **Responsibilities:** Move generation (GADDAG), heuristics (Vanguard/Treasurer), opponent modeling (Spy).
* **Constraint:** `Spectre` classes are tools used by the `AIPlayer`. They should not leak into the core `GameState`.

### The Boundary (The Friction Point)
* **Constraint Objects:** `RowConstraint` and `ConstraintGenerator` currently live in `include/fast_constraints.h`.
* **Namespace:** `spectre`.
* **Usage:** Used by `MoveGenerator` (Spectre) but calculated using `LetterBoard` (Engine).

---

## 3. The Current Bottleneck
At **515 games/sec**, the engine is limited by **Stateless Constraint Generation**.

* **Issue:** For every move search, we recalculate constraints for the entire board (15 rows + 15 cols).
* **Cost:** This is an $O(N)$ operation repeated 1000s of times per second.
* **Failed Attempt:** A previous attempt to optimize this using "Shadow Bitboards" (TZCNT loops) inside the generator yielded negligible gains due to setup overhead.

---

## 4. Future Roadmap (The "Think Bigger" Phase)
To reach 1,000 games/sec, the next steps must be architectural, not micro-optimizations.

### A. Incremental Constraint Maintenance (The "Golden Ticket")
* **Concept:** Make constraints persistent. Update only the rows/cols affected by a move.
* **Challenge:** Requires storing `spectre::RowConstraint` objects inside or alongside `GameState` without violating the modular architecture (i.e., Engine should ideally not know about Spectre structs).
* **Proposed Solution:** Create a `ConstraintCache` object in the Player Controller or AI layer that syncs with the `GameState`, rather than embedding it directly in `GameState` if strict purity is required.

### B. Bitwise Parallelism (PDF Method)
* **Concept:** Use `pdep` / `pext` or simple bitmasks to find anchors.
* **Status:** Partially attempted. Requires the Constraint Maintenance system to be in place first to be effective.

### C. Genetic Tuning
* **Concept:** Optimize `Treasurer` weights using genetic algorithms.
* **Status:** Pending engine speed targets.