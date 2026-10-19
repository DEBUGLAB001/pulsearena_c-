#pragma once
#include "model/tournament.hpp"
#include <string>

namespace pulsearena {

bool submitResult(Tournament& tournament, const std::string& matchId, const std::string& winner);

} // namespace pulsearena
