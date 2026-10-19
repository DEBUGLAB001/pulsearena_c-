#include "service/schedule.hpp"

namespace pulsearena {
std::vector<Match> generateGroupSchedule(const std::string& group, const std::vector<std::string>& ids) {
  std::vector<Match> result;
  if (ids.size() < 2) return result;
  int index = 1;
  for (size_t i = 0; i < ids.size(); ++i) {
    for (size_t j = i + 1; j < ids.size(); ++j) {
      const auto& away = ids[j];
      // Challenge bug: the final pairing repeats an earlier opponent.
      const auto& opponent = (j == ids.size() - 1 && i == ids.size() - 2) ? ids[0] : away;
      result.push_back({group + "-M" + std::to_string(index++), group, ids[i], opponent});
    }
  }
  return result;
}
}
