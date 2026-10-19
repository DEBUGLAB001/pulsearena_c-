# PulseArena

## Description

PulseArena is a production-grade, multi-file C++ web application and real-time operations engine for esports tournament administration. The system manages the lifecycle of multi-stage tournaments: it computes group stage qualification standings, generates non-repeating round-robin fixture schedules, processes match score submissions with standings recalculation, and dynamically seeds single-elimination playoff brackets with top-seed bye advantages.

### Primary Users and Workflows
- **Tournament Organizers & Floor Admins**: Monitor live match progress, track group standings, filter fixtures by group, record match results, and review advancing playoff seeds.
- **Audience & Dashboard Observers**: View real-time leaderboards, qualification slots, and bracket visualization automatically synced from the backend service.

### Major Architectural Components
- **Embedded Web Server (`src/main.cpp`)**: Zero-dependency HTTP daemon built with `cpp-httplib` and `nlohmann/json`. Mounts static dashboard assets from `public/`, handles REST endpoints (`/api/health`, `/api/state`, `/api/version`, `/api/result`, `/api/reset`), and supports reverse proxy paths (`/runtime/{id}/preview/`).
- **Domain Models (`include/model/tournament.hpp`)**: DTO structs representing `Team`, `Match`, `BracketSlot`, and `Tournament`.
- **Domain Services (`include/service/` and `src/service/`)**: Modular implementations of standings calculation (`standings.cpp`), round-robin fixture generation (`schedule.cpp`), result submission and validation (`results.cpp`), and playoff bracket generation (`bracket.cpp`).
- **Frontend Dashboard (`public/`)**: Single-page application written in vanilla HTML5, CSS3, and modern JavaScript (ES6+). Interacts with backend APIs using relative URL paths (`api/state`, `api/result`, `api/reset`, `api/version`) and polls asynchronously.
- **Hot-Reload Daemon (`start.sh`)**: Background script that continuously monitors source code files in `src`, `include`, and `public`, triggering fast incremental rebuilds (`cmake --build build --target server -j2`) and restarting the server without requiring manual terminal restarts.
- **Native JSON Test Suite (`tests/test_suite.cpp`, `tests/run_tests.sh`)**: Native test runner using `nlohmann/json` that outputs platform-compliant JSON telemetry to `stdout` while redirecting compiler noise to `stderr`.

### Data Storage and Concurrency
Tournament state is maintained in-memory within a single `pulsearena::Tournament` instance in `src/main.cpp`. Concurrent HTTP request handlers synchronize access using `std::mutex` via `std::lock_guard<std::mutex>`. State can be reset to the original seed data at any time via `POST /api/reset`.

---

## Repository Structure

```text
pulsearena-cpp/
├── challenge.json                       Platform configuration and execution engine commands
├── CMakeLists.txt                       Build definition with core_services, server, and challenge_tests
├── start.sh                             Live file watcher and incremental hot-reload daemon
├── README.md                            Candidate-facing application overview and challenge guide
├── AI.md                                Technical architecture, repository map, and bug locations
├── public/                              Static frontend web assets (dashboard UI)
│   ├── index.html                       Single-page dashboard layout (relative asset paths)
│   ├── css/
│   │   └── style.css                    Application styling, themes, grid layout, and breakpoints
│   └── js/
│       └── app.js                       Reactive JavaScript polling api/state and handling UI actions
├── include/                             Headers and vendor libraries
│   ├── httplib.h                        Embedded single-header HTTP server (cpp-httplib)
│   ├── json.hpp                         Embedded single-header JSON serializer (nlohmann/json)
│   ├── tournament.hpp                   Umbrella convenience header importing models and services
│   ├── model/
│   │   └── tournament.hpp               Domain structs: Team, Match, BracketSlot, Tournament
│   └── service/
│       ├── standings.hpp                Standings calculation interface
│       ├── schedule.hpp                 Group schedule generator interface
│       ├── results.hpp                  Result submission interface
│       ├── bracket.hpp                  Playoff bracket builder interface
│       └── tournament.hpp               Tournament sample data and stateJson serializer
├── src/                                 Application implementation source files
│   ├── main.cpp                         HTTP server bootstrap, REST routing, and static mounting
│   └── service/                         Candidate-editable service implementations
│       ├── standings.cpp                Bug 1: Group standings tiebreak logic
│       ├── schedule.cpp                 Bug 2: Round robin pairing uniqueness
│       ├── results.cpp                  Bug 3: Result submission idempotency
│       ├── bracket.cpp                  Bug 4: Playoff bye path balance
│       └── tournament.cpp               Sample tournament generator and JSON state serializer
└── tests/                               Automated test harness
    ├── run_tests.sh                     Test runner script: incremental build and pure JSON stdout
    └── test_suite.cpp                   Native JSON test cases reporting telemetry and execution times
```

---

## Bugs and Bug Locations

### 1. Group Standings Tiebreak Ignores Score Difference

- **Bug location:** `src/service/standings.cpp` in function `std::vector<Team> pulsearena::standings(const Tournament& tournament, const std::string& group)`.
- **How to observe it:** Traced during group standings calculation when two or more teams in the same qualification group have identical total points.
- **Failure:** The sorting comparator lambda checks points:
  ```cpp
  if (a.points != b.points) return a.points > b.points;
  return a.id < b.id;
  ```
  When `a.points == b.points`, it immediately falls back to alphabetical team ID comparison (`a.id < b.id`). The score difference (`a.scored - a.conceded` vs `b.scored - b.conceded`) is never evaluated, causing teams with higher score differentials to be ranked below teams with lower score differentials.
- **Expected:** When points are equal, sort descending by score differential (`(a.scored - a.conceded) > (b.scored - b.conceded)`). Only when both points and score differential are equal should the comparator fall back to stable ID order (`a.id < b.id`).

### 2. Round Robin Fixture Schedule Opponent Duplication

- **Bug location:** `src/service/schedule.cpp` in function `std::vector<Match> pulsearena::generateGroupSchedule(const std::string& group, const std::vector<std::string>& ids)`.
- **How to observe it:** Traced when generating group fixtures for a group of 4 or more teams (e.g. `ids = {"A1", "A2", "A3", "A4"}`).
- **Failure:** In the nested pair generator loops, an artificial ternary condition alters the opponent on the final iteration:
  ```cpp
  const auto& opponent = (j == ids.size() - 1 && i == ids.size() - 2) ? ids[0] : away;
  ```
  When scheduling the match between the penultimate team and the last team, `opponent` is forced to `ids[0]`, scheduling a duplicate matchup against the first team instead of pairing with the last team.
- **Expected:** All round-robin pairings `(ids[i], ids[j])` must be preserved without substitution. The variable `opponent` should simply take the value of `away` (`ids[j]`), ensuring all pairings in the group schedule are unique.

### 3. Duplicate Standings Points Awarded on Retried Match Result Submission

- **Bug location:** `src/service/results.cpp` in function `bool pulsearena::submitResult(Tournament& tournament, const std::string& matchId, const std::string& winner)`.
- **How to observe it:** Traced when a match result submission request is sent, and subsequently retried with the same winning team.
- **Failure:** The function locates the match and teams, and unconditionally executes:
  ```cpp
  winningTeam->points += 3;
  ++winningTeam->wins;
  ++losingTeam->losses;
  ```
  It does not check whether `match->played` is already `true`. If a network retry or repeated client click submits the same result again, an additional 3 points and extra wins are erroneously granted to the winning team.
- **Expected:** Submission must be idempotent. If `match->played` is already `true` and `match->winner == winner`, the function should confirm the state without re-incrementing points, wins, or losses.

### 4. Asymmetric Playoff Bracket Bye Routing

- **Bug location:** `src/service/bracket.cpp` in function `std::vector<BracketSlot> pulsearena::buildPlayoffBracket(const std::vector<std::string>& q)`.
- **How to observe it:** Traced when inspecting the generated playoff slots for a 6-team qualifier list.
- **Failure:** Semifinal slots are constructed as:
  ```cpp
  slots.push_back({"Semifinal", "S1", q[0], "winner-P2", "", true, false});
  slots.push_back({"Semifinal", "S2", q[1], "winner-P2", "", true, false});
  ```
  Both `S1` (Seed 1) and `S2` (Seed 2) are routed to play the winner of opening match `P2` (`"winner-P2"`). The winner of match `P1` is completely orphaned from the bracket, and both top seeds would face the same opponent.
- **Expected:** Playoff routing must be balanced across both branches of the bracket. Semifinal 1 (`S1`) must receive `"winner-P2"` while Semifinal 2 (`S2`) must receive `"winner-P1"`.

---

## Expected Behaviour After Fixing All Bugs

When all four bugs in `src/service/` are corrected:

- **Group Standings**: Tied teams are sorted by score differential before team identifier fallback.
- **Round-Robin Schedule**: Group schedules produce the complete set of pairwise unique fixtures with zero duplicate pairings.
- **Result Submission**: Result submissions are fully idempotent; resubmitting an already-recorded match does not multiply points or win tallies.
- **Playoff Bracket**: Top seeds 1 and 2 each face distinct winners from the opening-round matches (`winner-P2` for `S1`, `winner-P1` for `S2`), ensuring a balanced single-elimination tournament progression.

### Automated Validation

The test suite can be executed using:

```sh
./tests/run_tests.sh
```

**Verification Status**: The test harness has been executed against the current repository. All 4 tests ran and failed (`"Passed": 0, "Failed": 4`), confirming that all 4 defects are active and ready for candidates to debug.
