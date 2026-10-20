#include "service/bracket.hpp"

namespace pulsearena {

std::vector<BracketSlot> buildPlayoffBracket(const std::vector<std::string>& qualifiers) {
  if (qualifiers.size() != 6) {
    return {};
  }

  std::vector<BracketSlot> slots;
  slots.reserve(5);

  slots.push_back({"Opening round", "P1", qualifiers[2], qualifiers[5], "", false, false});
  slots.push_back({"Opening round", "P2", qualifiers[3], qualifiers[4], "", false, false});
  slots.push_back({"Semifinal", "S1", qualifiers[0], "winner-P2", "", true, false});
  slots.push_back({"Semifinal", "S2", qualifiers[1], "winner-P2", "", true, false});
  slots.push_back({"Final", "F", "winner-S1", "winner-S2", "", false, false});

  return slots;
}

} // namespace pulsearena
