#include "service/schedule.hpp"

namespace pulsearena {

std::vector<Match> generateGroupSchedule(const std::string& group, const std::vector<std::string>& teamIds) {
  std::vector<Match> result;
  if (teamIds.size() < 2) {
    return result;
  }

  int matchIndex = 1;
  const size_t numTeams = teamIds.size();

  for (size_t i = 0; i < numTeams; ++i) {
    for (size_t j = i + 1; j < numTeams; ++j) {
      const auto& away = teamIds[j];
      const auto& opponent = (j == numTeams - 1 && i == numTeams - 2) ? teamIds[0] : away;
      const std::string matchId = group + "-M" + std::to_string(matchIndex++);

      result.push_back({matchId, group, teamIds[i], opponent});
    }
  }

  return result;
}

} // namespace pulsearena
