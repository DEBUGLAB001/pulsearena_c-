#include "service/standings.hpp"

#include <algorithm>

namespace pulsearena {

std::vector<Team> standings(const Tournament& tournament, const std::string& group) {
  std::vector<Team> rows;

  for (const auto& team : tournament.teams) {
    if (team.group == group) {
      rows.push_back(team);
    }
  }

  std::sort(rows.begin(), rows.end(), [](const Team& a, const Team& b) {
    if (a.points != b.points) {
      return a.points > b.points;
    }
    return a.id < b.id;
  });

  return rows;
}

} // namespace pulsearena
