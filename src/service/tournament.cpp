#include "service/tournament.hpp"
#include "service/standings.hpp"
#include "service/schedule.hpp"
#include "service/bracket.hpp"

#include <algorithm>
#include <sstream>

namespace pulsearena {

Tournament sampleTournament() {
  Tournament t;
  t.teams = {
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
  for (const std::string group : {"Alpha", "Beta", "Gamma"}) {
    std::vector<std::string> ids;
    for (const auto& team : t.teams) if (team.group == group) ids.push_back(team.id);
    auto schedule = generateGroupSchedule(group, ids);
    t.matches.insert(t.matches.end(), schedule.begin(), schedule.end());
  }
  t.bracket = buildPlayoffBracket({"NOVA", "MINT", "RIFT", "VOLT", "ORBT", "LUMA"});
  return t;
}

namespace {
std::string esc(const std::string& s) {
  std::string out;
  for (char c : s) {
    if (c == '"' || c == '\\') out.push_back('\\');
    if (c == '\n') { out += "\\n"; continue; }
    out.push_back(c);
  }
  return out;
}
void teamJson(std::ostringstream& o, const Team& t) {
  o << "{\"id\":\"" << esc(t.id) << "\",\"name\":\"" << esc(t.name)
    << "\",\"group\":\"" << esc(t.group) << "\",\"points\":" << t.points
    << ",\"wins\":" << t.wins << ",\"losses\":" << t.losses
    << ",\"scored\":" << t.scored << ",\"conceded\":" << t.conceded
    << ",\"diff\":" << (t.scored - t.conceded) << "}";
}
}

std::string stateJson(const Tournament& t) {
  std::ostringstream o;
  const std::vector<std::string> groups{"Alpha", "Beta", "Gamma"};
  std::vector<Team> allTeams = t.teams;
  std::sort(allTeams.begin(), allTeams.end(), [](const Team& a, const Team& b) {
    if (a.points != b.points) return a.points > b.points;
    if (a.scored - a.conceded != b.scored - b.conceded) return a.scored - a.conceded > b.scored - b.conceded;
    return a.id < b.id;
  });
  o << "{\"event\":\"PulseArena Spring Invitational\",\"status\":\"GROUP STAGE LIVE\",\"leader\":";
  if (!allTeams.empty()) {
    const auto& leader = allTeams.front();
    o << "{\"id\":\"" << esc(leader.id) << "\",\"points\":" << leader.points << ",\"diff\":" << (leader.scored - leader.conceded) << '}';
  } else o << "null";
  o << ",\"qualifierCount\":" << std::min<size_t>(6, t.teams.size()) << ",\"matchCount\":" << t.matches.size() << ",\"groups\":[";
  for (size_t g = 0; g < groups.size(); ++g) {
    if (g) o << ',';
    o << "{\"name\":\"" << groups[g] << "\",\"teams\":[";
    auto rows = standings(t, groups[g]);
    for (size_t i = 0; i < rows.size(); ++i) { if (i) o << ','; teamJson(o, rows[i]); }
    o << "]}";
  }
  o << "],\"matches\":[";
  for (size_t i = 0; i < t.matches.size(); ++i) {
    if (i) o << ',';
    const auto& m = t.matches[i];
    o << "{\"id\":\"" << esc(m.id) << "\",\"group\":\"" << esc(m.group)
      << "\",\"home\":\"" << esc(m.home) << "\",\"away\":\"" << esc(m.away)
      << "\",\"played\":" << (m.played ? "true" : "false") << ",\"winner\":\"" << esc(m.winner)
      << "\",\"homeScore\":" << m.homeScore << ",\"awayScore\":" << m.awayScore << '}';
  }
  o << "],\"bracket\":[";
  for (size_t i = 0; i < t.bracket.size(); ++i) {
    if (i) o << ',';
    const auto& b = t.bracket[i];
    o << "{\"round\":\"" << esc(b.round) << "\",\"matchId\":\"" << esc(b.matchId)
      << "\",\"teamA\":\"" << esc(b.teamA) << "\",\"teamB\":\"" << esc(b.teamB)
      << "\",\"winner\":\"" << esc(b.winner) << "\",\"byeA\":" << (b.byeA ? "true" : "false")
      << ",\"byeB\":" << (b.byeB ? "true" : "false") << '}';
  }
  o << "]}";
  return o.str();
}

} // namespace pulsearena
