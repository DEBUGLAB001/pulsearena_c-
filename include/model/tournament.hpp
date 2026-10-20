#pragma once

#include <string>
#include <vector>

namespace pulsearena {

struct Team {
  std::string id;
  std::string name;
  std::string group;
  int points = 0;
  int wins = 0;
  int losses = 0;
  int scored = 0;
  int conceded = 0;
};

struct Match {
  std::string id;
  std::string group;
  std::string home;
  std::string away;
  bool played = false;
  std::string winner;
  int homeScore = 0;
  int awayScore = 0;
};

struct BracketSlot {
  std::string round;
  std::string matchId;
  std::string teamA;
  std::string teamB;
  std::string winner;
  bool byeA = false;
  bool byeB = false;
};

struct Tournament {
  std::vector<Team> teams;
  std::vector<Match> matches;
  std::vector<BracketSlot> bracket;
};

} // namespace pulsearena
