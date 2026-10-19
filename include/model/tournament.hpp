#pragma once
#include <string>
#include <vector>

namespace pulsearena {

struct Team {
  std::string id, name, group;
  int points = 0, wins = 0, losses = 0, scored = 0, conceded = 0;
};

struct Match {
  std::string id, group, home, away;
  bool played = false;
  std::string winner;
  int homeScore = 0, awayScore = 0;
};

struct BracketSlot {
  std::string round, matchId, teamA, teamB, winner;
  bool byeA = false, byeB = false;
};

struct Tournament {
  std::vector<Team> teams;
  std::vector<Match> matches;
  std::vector<BracketSlot> bracket;
};

} // namespace pulsearena
