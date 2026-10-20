# PulseArena — Esports Tournament Operations & Playoff Engine

PulseArena is a real-time esports tournament operations dashboard designed for tournament organizers and floor administrators. It manages the complete event flow by tracking group stage leaderboards, scheduling round-robin matches, recording match outcomes, and advancing qualified teams into a single-elimination championship playoff bracket. Organizers can monitor live event metrics, filter fixtures by group, and reset tournament state directly from an interactive web interface.

## 1. Application Overview

### Core Functionality

- **Live Tournament Dashboard**: View key event telemetry including total participating teams, completed match counts, playoff qualification positions, and overall leader rankings.
- **Group Stage Standings**: Track standings across three distinct qualification groups (Alpha, Beta, and Gamma) with real-time updates for wins, points, and score differences.
- **Round-Robin Match Scheduling**: Generate fixture schedules ensuring every team plays against every other group opponent, complete with filtering by group or across the entire event.
- **Match Result Recording**: Submit match winners directly from the fixture desk with automatic score assignment and immediate recalculation of group standings.
- **Dynamic Playoff Bracket**: Automatically advance the top qualifying teams into a six-team single-elimination championship bracket where the top two seeds receive opening-round byes.
- **State Reset Controls**: Restore the initial seeded tournament teams and match state with a single click for testing and demonstration workflows.

## 2. Debugging Challenge

PulseArena contains four operational defects in its tournament management logic that disrupt standings, match scheduling, result recording, and playoff brackets. The challenge is to identify and resolve each defect so that tournament data remains consistent and accurate throughout the competition lifecycle.

### Reported Issues & Tasks:

#### Issue 1: Group Standings Tiebreak Ignores Score Difference

- User Symptom: When two teams in a group finish with the same number of points, the leaderboard ranks them purely alphabetically by team ID instead of rewarding the team with the better score differential.
- Task: Ensure that tied teams are ranked by score difference (scored minus conceded) before falling back to team identifiers.

#### Issue 2: Round Robin Fixture Schedule Opponent Duplication

- User Symptom: In four-team groups, the schedule generates a duplicate match against the first opponent instead of pairing the last two teams, leaving the schedule one unique matchup short.
- Task: Ensure that all round-robin fixtures produce distinct, non-repeating matchups so that every team plays each group opponent exactly once.

#### Issue 3: Duplicate Standings Points Awarded on Retried Match Result Submission

- User Symptom: Submitting the result of a match that was already recorded adds an additional 3 points and extra wins to the winning team, distorting group standings upon repeated clicks or retried requests.
- Task: Ensure that result recording is idempotent, confirming existing results without granting duplicate points or win tallies.

#### Issue 4: Asymmetric Playoff Bracket Bye Routing

- User Symptom: In the championship playoff bracket, both top-seeded semifinalists are routed to face the winner of the second opening match, completely stranding the winner of the first opening match.
- Task: Ensure that semifinal matchups route winners from different opening matches, so Seed 1 and Seed 2 each face a distinct opponent while maintaining their opening-round byes.

## 3. Expected Behavior After Fixing Bugs

1. **Group Standings**: Teams tied on points are ranked by score differential, with team ID order used only when points and score differentials are identical.
2. **Fixture Uniqueness**: Group schedules generate the full set of unique pairwise matchups with zero duplicate opponents.
3. **Result Idempotency**: Recording an already-completed match result confirms the recorded outcome without inflating points or win/loss records.
4. **Bracket Progression**: Semifinal slots correctly route winners from separate opening-round matches to Seed 1 and Seed 2.


