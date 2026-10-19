#include "json.hpp"
#include "service/standings.hpp"
#include "service/schedule.hpp"
#include "service/results.hpp"
#include "service/bracket.hpp"
#include "service/tournament.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using json = nlohmann::json;

struct TestCase {
  std::string name;
  std::function<void()> test_fn;
};

int main() {
  std::vector<TestCase> test_cases;

  // --- TEST 1: Group Standings Tiebreak ---
  test_cases.push_back({"test_group_standings_tiebreak", []() {
    pulsearena::Tournament t;
    t.teams = {{"ALFA", "Alpha", "A", 6, 2, 0, 24, 10}, {"BRAV", "Bravo", "A", 6, 2, 0, 28, 10}};
    auto rows = pulsearena::standings(t, "A");
    if (rows.front().id != "BRAV") {
      throw std::runtime_error("Equal-points teams must rank by score difference before stable ID order");
    }
  }});

  // --- TEST 2: Round Robin Fixture Uniqueness ---
  test_cases.push_back({"test_round_robin_fixture_uniqueness", []() {
    auto matches = pulsearena::generateGroupSchedule("A", {"A1", "A2", "A3", "A4"});
    std::set<std::pair<std::string, std::string>> pairs;
    for (const auto& m : matches) {
      auto pair = std::minmax(m.home, m.away);
      if (!pairs.emplace(pair.first, pair.second).second) {
        throw std::runtime_error("Group schedule contains a repeated opponent pairing");
      }
    }
    if (matches.size() != 6 || pairs.size() != 6) {
      throw std::runtime_error("Four-team group must schedule each of its six unique pairings");
    }
  }});

  // --- TEST 3: Result Submission Idempotency ---
  test_cases.push_back({"test_result_submission_is_idempotent", []() {
    auto t = pulsearena::sampleTournament();
    auto it = std::find_if(t.matches.begin(), t.matches.end(), [](const pulsearena::Match& m) { return m.group == "Alpha"; });
    if (it == t.matches.end()) {
      throw std::runtime_error("Expected an Alpha fixture");
    }
    const int before = std::find_if(t.teams.begin(), t.teams.end(), [&](const pulsearena::Team& team) { return team.id == it->home; })->points;
    if (!pulsearena::submitResult(t, it->id, it->home) || !pulsearena::submitResult(t, it->id, it->home)) {
      throw std::runtime_error("Expected both original and retry submissions to be accepted");
    }
    const auto after = std::find_if(t.teams.begin(), t.teams.end(), [&](const pulsearena::Team& team) { return team.id == it->home; })->points;
    if (after != before + 3) {
      throw std::runtime_error("A retried result must not award standings points twice");
    }
  }});

  // --- TEST 4: Playoff Bye Paths Are Balanced ---
  test_cases.push_back({"test_playoff_bye_paths_are_balanced", []() {
    auto bracket = pulsearena::buildPlayoffBracket({"S1", "S2", "S3", "S4", "S5", "S6"});
    auto semifinalOne = std::find_if(bracket.begin(), bracket.end(), [](const auto& slot) { return slot.matchId == "S1"; });
    auto semifinalTwo = std::find_if(bracket.begin(), bracket.end(), [](const auto& slot) { return slot.matchId == "S2"; });
    if (semifinalOne == bracket.end() || semifinalTwo == bracket.end()) {
      throw std::runtime_error("Expected two semifinal slots");
    }
    if (semifinalOne->teamB == semifinalTwo->teamB) {
      throw std::runtime_error("Each semifinal must receive the winner from a different opening match");
    }
    if (!semifinalOne->byeA || !semifinalTwo->byeA) {
      throw std::runtime_error("Top two seeds must receive their opening-round byes");
    }
  }});

  // --- RUN & AGGREGATE RESULTS ---
  json report;
  int passed = 0;
  int failed = 0;
  auto suite_start = std::chrono::steady_clock::now();

  for (const auto& tc : test_cases) {
    auto t_start = std::chrono::steady_clock::now();
    try {
      tc.test_fn();
      auto t_end = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
      report[tc.name] = {
        {"Status", "passed"},
        {"Execution time", std::to_string(elapsed) + "ms"}
      };
      passed++;
    } catch (const std::exception& ex) {
      auto t_end = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
      report[tc.name] = {
        {"Status", "failed"},
        {"Execution time", std::to_string(elapsed) + "ms"},
        {"Error", ex.what()}
      };
      failed++;
    }
  }

  auto suite_end = std::chrono::steady_clock::now();
  auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(suite_end - suite_start).count();

  report["Passed"] = passed;
  report["Failed"] = failed;
  report["Total bugs"] = static_cast<int>(test_cases.size());
  report["total"] = static_cast<int>(test_cases.size());
  report["passed"] = passed;
  report["failed"] = failed;
  report["Total Execution time"] = std::to_string(total_ms) + "ms";

  // Output pure JSON to stdout
  std::cout << report.dump(2) << std::endl;

  // Return non-zero if any test failed
  return (failed == 0) ? 0 : 1;
}
