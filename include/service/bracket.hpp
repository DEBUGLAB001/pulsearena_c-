#pragma once
#include "model/tournament.hpp"
#include <string>
#include <vector>

namespace pulsearena {

std::vector<BracketSlot> buildPlayoffBracket(const std::vector<std::string>& orderedQualifiers);

} // namespace pulsearena
