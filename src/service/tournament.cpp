#include "service/tournament.hpp"
#include "service/standings.hpp"
#include "service/schedule.hpp"
#include "service/bracket.hpp"

#include <algorithm>
#include <sstream>

namespace pulsearena {

Tournament sampleTournament() {
  Tournament tournament;

  tournament.teams = {
    {"NOVA", "Nova Ravens", "Alpha", 6, 2, 0, 31, 18},
    {"VOLT", "Volt Union", "Alpha", 6, 2, 0, 28, 16},
    {"ECHO", "Echo Five", "Alpha", 3, 1, 1, 24, 25},
    {"MINT", "Mint Condition", "Beta", 6, 2, 0, 30, 17},
    {"ORBT", "Orbit Club", "Beta", 6, 2, 0, 27, 17},
    {"KITE", "Kite Theory", "Beta", 3, 1, 1, 22, 25},
    {"RIFT", "Rift Walkers", "Gamma", 6, 2, 0, 29, 16},
    {"LUMA", "Luma Core", "Gamma", 6, 2, 0, 26, 15},
    {"BYTE", "Byte Me", "Gamma", 3, 1, 1, 23, 27}
  };

  for (const std::string& group : {"Alpha", "Beta", "Gamma"}) {
    std::vector<std::string> ids;
    for (const auto& team : tournament.teams) {
      if (team.group == group) {
        ids.push_back(team.id);
      }
    }
    auto schedule = generateGroupSchedule(group, ids);
    tournament.matches.insert(tournament.matches.end(), schedule.begin(), schedule.end());
  }

  tournament.bracket = buildPlayoffBracket({"NOVA", "MINT", "RIFT", "VOLT", "ORBT", "LUMA"});
  return tournament;
}

namespace {

std::string escapeJson(const std::string& str) {
  std::string out;
  for (char c : str) {
    if (c == '"' || c == '\\') {
      out.push_back('\\');
    }
    if (c == '\n') {
      out += "\\n";
      continue;
    }
    out.push_back(c);
  }
  return out;
}

void appendTeamJson(std::ostringstream& oss, const Team& team) {
  oss << "{\"id\":\"" << escapeJson(team.id)
      << "\",\"name\":\"" << escapeJson(team.name)
      << "\",\"group\":\"" << escapeJson(team.group)
      << "\",\"points\":" << team.points
      << ",\"wins\":" << team.wins
      << ",\"losses\":" << team.losses
      << ",\"scored\":" << team.scored
      << ",\"conceded\":" << team.conceded
      << ",\"diff\":" << (team.scored - team.conceded)
      << "}";
}

} // namespace

std::string stateJson(const Tournament& tournament) {
  std::ostringstream out;
  const std::vector<std::string> groups{"Alpha", "Beta", "Gamma"};

  std::vector<Team> allTeams = tournament.teams;
  std::sort(allTeams.begin(), allTeams.end(), [](const Team& a, const Team& b) {
    if (a.points != b.points) {
      return a.points > b.points;
    }
    const int diffA = a.scored - a.conceded;
    const int diffB = b.scored - b.conceded;
    if (diffA != diffB) {
      return diffA > diffB;
    }
    return a.id < b.id;
  });

  out << "{\"event\":\"PulseArena Spring Invitational\",\"status\":\"GROUP STAGE LIVE\",\"leader\":";
  if (!allTeams.empty()) {
    const auto& leader = allTeams.front();
    out << "{\"id\":\"" << escapeJson(leader.id)
        << "\",\"points\":" << leader.points
        << ",\"diff\":" << (leader.scored - leader.conceded) << '}';
  } else {
    out << "null";
  }

  out << ",\"qualifierCount\":" << std::min<size_t>(6, tournament.teams.size())
      << ",\"matchCount\":" << tournament.matches.size()
      << ",\"groups\":[";

  for (size_t g = 0; g < groups.size(); ++g) {
    if (g > 0) out << ',';
    out << "{\"name\":\"" << groups[g] << "\",\"teams\":[";
    auto rows = standings(tournament, groups[g]);
    for (size_t i = 0; i < rows.size(); ++i) {
      if (i > 0) out << ',';
      appendTeamJson(out, rows[i]);
    }
    out << "]}";
  }
  out << "],\"matches\":[";

  for (size_t i = 0; i < tournament.matches.size(); ++i) {
    if (i > 0) out << ',';
    const auto& m = tournament.matches[i];
    out << "{\"id\":\"" << escapeJson(m.id)
        << "\",\"group\":\"" << escapeJson(m.group)
        << "\",\"home\":\"" << escapeJson(m.home)
        << "\",\"away\":\"" << escapeJson(m.away)
        << "\",\"played\":" << (m.played ? "true" : "false")
        << ",\"winner\":\"" << escapeJson(m.winner)
        << "\",\"homeScore\":" << m.homeScore
        << ",\"awayScore\":" << m.awayScore << '}';
  }
  out << "],\"bracket\":[";

  for (size_t i = 0; i < tournament.bracket.size(); ++i) {
    if (i > 0) out << ',';
    const auto& b = tournament.bracket[i];
    out << "{\"round\":\"" << escapeJson(b.round)
        << "\",\"matchId\":\"" << escapeJson(b.matchId)
        << "\",\"teamA\":\"" << escapeJson(b.teamA)
        << "\",\"teamB\":\"" << escapeJson(b.teamB)
        << "\",\"winner\":\"" << escapeJson(b.winner)
        << "\",\"byeA\":" << (b.byeA ? "true" : "false")
        << ",\"byeB\":" << (b.byeB ? "true" : "false") << '}';
  }
  out << "]}";

  return out.str();
}

} // namespace pulsearena

