# PulseArena - Esports Tournament Operations & Playoff Engine

PulseArena is a self-contained, real-time esports tournament operations platform and operations dashboard. Designed for tournament organizers and admins, PulseArena manages the full tournament lifecycle: tracking round-robin group stage standings, generating unique fixture schedules, recording match results with live standings recalculations, and advancing qualified teams into a single-elimination playoff bracket with top-seed byes. The application combines an embedded, high-performance C++ backend with a dark-mode web dashboard that polls backend state and hot-reloads on live code edits.

---

## 1. Application Overview

### Core Functionality

- **Live Tournament Overview & Metrics**: Real-time telemetry summarizing total competing teams, completed fixtures versus scheduled matches, available playoff qualification spots, and the overall tournament leader (ranked by points, score differential, and stable team identifier).
- **Multi-Group Qualification Standings**: Manages three distinct qualification groups (Alpha, Beta, Gamma) of three teams each. Displays wins, points, and score differences, clearly designating the top two teams in each group that qualify for the championship playoffs.
- **Automated Round-Robin Fixture Desk**: Generates round-robin fixtures for each group. Provides a group filter to view fixtures by group or across the entire event, along with interactive result submission controls to record wins and test idempotency.
- **Dynamic Playoff Bracket & Bye Advancement**: Generates a six-team single-elimination championship bracket across three rounds (Opening round, Semifinal, and Final). The top two seeds automatically receive opening-round byes, while the remaining four qualifiers battle in opening round matches `P1` and `P2`.
- **State Synchronization & Reset Controls**: Includes live synchronization polling via `/api/state`, server version change detection via `/api/version`, and a one-click demo reset button calling `POST /api/reset` to restore seeded tournament conditions.
- **Embedded Zero-Dependency HTTP Service**: Built on header-only `cpp-httplib` and `nlohmann/json`, binding to `0.0.0.0` on port `8080` (or the environment-configured `PORT`), serving static public assets directly without external runtime dependencies.

### Technology Stack

- **Runtime & Language**: C++20 (`clang++` / `g++` with GNU/POSIX threads).
- **HTTP Server**: `cpp-httplib` (single-header C++ HTTP server library in `include/httplib.h`).
- **Data Serialization**: `nlohmann/json` (single-header JSON serializer in `include/json.hpp`).
- **Frontend Dashboard**: Vanilla HTML5, modern CSS3 (custom properties, CSS grid, dark-mode glassmorphic theme), and vanilla JavaScript ES6+ (Fetch API, non-blocking asynchronous state polling).
- **Data Persistence & Concurrency**: In-memory `Tournament` state protected by `std::mutex`, seeded with initial event teams and match data.
- **Build System**: CMake 3.20+ with three distinct targets:
  - `core_services`: Static library compiling candidate-editable domain services (`src/service/*.cpp`).
  - `server`: Web server executable linking `core_services` and threading libraries.
  - `challenge_tests`: Test runner executable linking `core_services` and running the test suite.
- **Live Recompile & Hot-Reload**: Bash daemon (`start.sh`) running incremental builds (`cmake --build build --target server -j2`) on source file modification.
- **Test Harness**: Zero-dependency native JSON test runner in `tests/test_suite.cpp` invoked via `tests/run_tests.sh`.

### Setup and Execution Commands

#### Configure and Build
```sh
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
```

#### Run the Web Application with Hot-Reload
```sh
./start.sh
```
The server will bind to `0.0.0.0:8080` (or the port defined in `$PORT`). Static assets are served from `public/`.

#### Run the Automated Challenge Test Suite
```sh
./tests/run_tests.sh
```

---

## 2. Debugging Challenge

PulseArena has four reported logic defects distributed across its candidate-editable service files in `src/service/`. These bugs cause incorrect standings rankings, duplicate scheduling, corrupted points on retried result submissions, and broken playoff bracket progression. The objective is to identify each defect, trace the execution flow, and correct the domain logic while ensuring the automated test suite passes.

### Reported Issues & Tasks

#### Issue 1: Group Standings Tiebreak Ignores Score Difference

- **Reproduction**: Call `pulsearena::standings(tournament, "A")` with two teams possessing identical points (e.g., 6 points each) but different score differentials (e.g., +14 vs +18).
- **Failure**: Teams with equal points are sorted alphabetically by team ID (`a.id < b.id`). The team with the higher score difference (`scored - conceded`) fails to take precedence over the team with the lower score difference.
- **Expected**: Teams in each group must first be ranked descending by points; teams tied on points must rank descending by score difference (`scored - conceded`); only when both points and score differences are identical should tiebreaking fall back to alphabetical team ID order.

#### Issue 2: Round Robin Fixture Schedule Opponent Duplication

- **Reproduction**: Call `pulsearena::generateGroupSchedule("A", {"A1", "A2", "A3", "A4"})` to generate a 4-team round-robin schedule.
- **Failure**: The schedule generates 6 matches, but the final fixture duplicates an earlier opponent (scheduling `A3` vs `A1` instead of `A3` vs `A4`). Only 5 unique pairings are created instead of all 6 distinct combinations.
- **Expected**: Every round-robin group schedule must produce all unique opponent pairings with no repeated pairings. For 4 teams, exactly 6 unique match pairings must be generated.

#### Issue 3: Duplicate Standings Points Awarded on Retried Match Result Submission

- **Reproduction**: Call `pulsearena::submitResult(tournament, matchId, winner)` for a match, and then retry or re-submit the same winning result for that identical `matchId`.
- **Failure**: Standings points (+3) and win/loss counters are unconditionally incremented on every submission call, even if the match was already marked as played (`match.played == true`). A retried result adds an extra 3 points to the winner, breaking submission idempotency.
- **Expected**: Result submission must be idempotent. When a result for an already-played match is submitted with the existing winner, the submission should succeed but must not award duplicate points or increment win/loss records a second time.

#### Issue 4: Asymmetric Playoff Bracket Bye Routing

- **Reproduction**: Call `pulsearena::buildPlayoffBracket({"S1", "S2", "S3", "S4", "S5", "S6"})` with six ordered qualifiers. Inspect the generated semifinal matchups.
- **Failure**: Both Semifinal 1 (`S1`) and Semifinal 2 (`S2`) are routed to play the winner of match `P2` (`winner-P2`). The winner of opening-round match `P1` is never routed to any semifinal slot, and seed 1 and seed 2 are incorrectly placed on collision course with the same opening-round winner.
- **Expected**: The bracket routes must be balanced. Semifinal 1 (`S1`) should pit Seed 1 (`q[0]`) against the winner of `P2` (`winner-P2`), and Semifinal 2 (`S2`) should pit Seed 2 (`q[1]`) against the winner of `P1` (`winner-P1`). Both top seeds retain their opening-round byes.

---

## 3. Expected Behavior After Fixing Bugs

1. **Group Standings**: Equal-points teams correctly rank by score difference before falling back to alphabetical team ID order.
2. **Fixture Uniqueness**: All round-robin schedules generate pairwise unique matches with no duplicated opponents.
3. **Result Idempotency**: Re-submitting or retrying a match result does not award points or update win/loss tallies more than once.
4. **Bracket Progression**: Playoff semifinals receive winners from distinct opening-round matchups (`winner-P2` for `S1`, `winner-P1` for `S2`), ensuring a balanced championship bracket.

To validate your fixes against the automated test suite, run:

```sh
./tests/run_tests.sh
```

> **Test Execution Status**: The test suite was executed against the initial challenge repository. All 4 tests failed as expected (`"Failed": 4, "Passed": 0`), confirming the presence of the four intentional bugs waiting to be solved.
#   p u l s e a r e n a _ c -  
 