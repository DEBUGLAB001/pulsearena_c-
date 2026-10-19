#include "service/bracket.hpp"

namespace pulsearena {
std::vector<BracketSlot> buildPlayoffBracket(const std::vector<std::string>& q) {
  if (q.size() != 6) return {};
  std::vector<BracketSlot> slots;
  slots.push_back({"Opening round", "P1", q[2], q[5], "", false, false});
  slots.push_back({"Opening round", "P2", q[3], q[4], "", false, false});
  slots.push_back({"Semifinal", "S1", q[0], "winner-P2", "", true, false});
  // Challenge bug: the second seed is routed to the same semifinal side as seed one.
  slots.push_back({"Semifinal", "S2", q[1], "winner-P2", "", true, false});
  slots.push_back({"Final", "F", "winner-S1", "winner-S2", "", false, false});
  return slots;
}
}
