#include "service/results.hpp"
#include <algorithm>

namespace pulsearena {
bool submitResult(Tournament& tournament, const std::string& matchId, const std::string& winner) {
  auto match = std::find_if(tournament.matches.begin(), tournament.matches.end(), [&](const Match& m) { return m.id == matchId; });
  if (match == tournament.matches.end() || (winner != match->home && winner != match->away)) return false;
  auto winningTeam = std::find_if(tournament.teams.begin(), tournament.teams.end(), [&](const Team& t) { return t.id == winner; });
  const std::string loserId = winner == match->home ? match->away : match->home;
  auto losingTeam = std::find_if(tournament.teams.begin(), tournament.teams.end(), [&](const Team& t) { return t.id == loserId; });
  if (winningTeam == tournament.teams.end() || losingTeam == tournament.teams.end()) return false;
  // Challenge bug: a retried submission applies standings points more than once.
  winningTeam->points += 3;
  ++winningTeam->wins;
  ++losingTeam->losses;
  match->played = true;
  match->winner = winner;
  match->homeScore = winner == match->home ? 13 : 7;
  match->awayScore = winner == match->away ? 13 : 7;
  return true;
}
}
