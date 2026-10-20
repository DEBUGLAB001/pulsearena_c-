# PulseArena

## Description

PulseArena is a real-time esports tournament operations platform and administration engine written in C++20. It manages multi-stage tournament lifecycles: calculating group stage qualification standings, generating round-robin fixture schedules, recording match score submissions with standings updates, and seeding single-elimination playoff brackets with top-seed byes. The entry point is `src/main.cpp`, which initializes an embedded HTTP server via `cpp-httplib` and `nlohmann/json`, exposes REST endpoints (`/api/state`, `/api/result`, `/api/reset`, `/api/version`, `/api/health`), and mounts static dashboard assets from `public/`. Core business logic resides in modular service files under `src/service/` with declarations in `include/service/` and domain models in `include/model/tournament.hpp`. Tournament state is held in-memory via a thread-safe `pulsearena::Tournament` instance protected by `std::mutex` in `src/main.cpp`.

## Repository Structure

```
.
├── CMakeLists.txt              # CMake build configuration for services and server
├── challenge.json              # Platform challenge specification and runtime config
├── start.sh                    # Live file-watcher daemon for incremental rebuilds
├── include/                    # Header files and third-party single-header libraries
│   ├── httplib.h               # Embedded HTTP server library (cpp-httplib)
│   ├── json.hpp                # JSON serialization library (nlohmann/json)
│   ├── tournament.hpp          # Aggregate include header
│   ├── model/
│   │   └── tournament.hpp      # Tournament domain data structures (Team, Match, BracketSlot)
│   └── service/
│       ├── bracket.hpp         # Playoff bracket generation interface
│       ├── results.hpp         # Result submission interface
│       ├── schedule.hpp        # Fixture schedule generator interface
│       ├── standings.hpp       # Group standings calculation interface
│       └── tournament.hpp      # Sample tournament and state serialization interface
├── public/                     # Static web frontend dashboard
│   ├── css/
│   │   └── style.css           # Dashboard layout, themes, and responsive styling
│   ├── js/
│   │   └── app.js              # Client state polling and interaction handlers
│   └── index.html              # Single-page dashboard interface
└── src/                        # C++ implementation source files
    ├── main.cpp                # HTTP server setup, REST routing, and mutex synchronization
    └── service/                # Core domain service implementations
        ├── bracket.cpp         # Playoff bracket construction
        ├── results.cpp         # Match result recording and validation
        ├── schedule.cpp        # Round-robin match schedule generation
        ├── standings.cpp       # Group standings sorting and tiebreaking
        └── tournament.cpp      # Sample data generation and JSON state serializer
```

## Bugs and Bug Locations

1. Group Standings Tiebreak Ignores Score Difference

- Bug location: "src/service/standings.cpp", within `std::vector<Team> pulsearena::standings(const Tournament& tournament, const std::string& group)` (lines 8–12).
- How to observe it: Compute standings for a group containing two teams with identical points (e.g., 6 points each) but differing score differentials (e.g., +18 vs +14).
- Failure: The sorting comparator evaluates points (`a.points != b.points`) and immediately falls back to alphabetical team ID sorting (`a.id < b.id`). Score difference (`a.scored - a.conceded`) is never checked, ranking teams with lower differentials above teams with higher differentials.
- Expected: Teams tied on points must be sorted descending by score difference (`(a.scored - a.conceded) > (b.scored - b.conceded)`). Only when both points and score differentials are identical should ranking fall back to ascending team ID (`a.id < b.id`).

2. Round Robin Fixture Schedule Opponent Duplication

- Bug location: "src/service/schedule.cpp", within `std::vector<Match> pulsearena::generateGroupSchedule(const std::string& group, const std::vector<std::string>& ids)` (lines 8–15).
- How to observe it: Generate a fixture schedule for a group with 4 or more teams (e.g., `ids = {"A1", "A2", "A3", "A4"}`).
- Failure: A conditional ternary on line 12 (`(j == ids.size() - 1 && i == ids.size() - 2) ? ids[0] : away`) forces the opponent on the final pairing iteration to `ids[0]`. This schedules a duplicate match against the first team instead of pairing with `ids[j]`, leaving only 5 unique pairings instead of 6.
- Expected: All pairings must use `away` (`ids[j]`) without substitution, producing the complete set of unique, non-repeating round-robin fixtures.

3. Duplicate Standings Points Awarded on Retried Match Result Submission

- Bug location: "src/service/results.cpp", within `bool pulsearena::submitResult(Tournament& tournament, const std::string& matchId, const std::string& winner)` (lines 5–21).
- How to observe it: Submit a valid match result via `pulsearena::submitResult` and then invoke the function a second time with the identical `matchId` and `winner`.
- Failure: Lines 13–15 increment `winningTeam->points += 3` and increment win/loss tallies without verifying whether `match->played` is already `true`. Resubmission grants an extra 3 points and redundant win/loss counts on each duplicate call.
- Expected: Submission must be idempotent. If `match->played` is `true` and `match->winner == winner`, the function must confirm success without re-incrementing points, wins, or losses.

4. Asymmetric Playoff Bracket Bye Routing

- Bug location: "src/service/bracket.cpp", within `std::vector<BracketSlot> pulsearena::buildPlayoffBracket(const std::vector<std::string>& q)` (lines 7–12).
- How to observe it: Generate a 6-team playoff bracket with qualifiers `{"S1", "S2", "S3", "S4", "S5", "S6"}` and inspect semifinal matchup assignments.
- Failure: Both semifinal slots `S1` and `S2` assign `"winner-P2"` as `teamB` (lines 9 and 11). The winner of opening-round match `P1` is orphaned from the bracket, routing both top seeds against the same opening-round winner.
- Expected: Semifinal bracket slots must route distinct opening-round winners: Semifinal `S1` must face `"winner-P2"` while Semifinal `S2` must face `"winner-P1"`, maintaining balanced tournament progression for both bye seeds.
