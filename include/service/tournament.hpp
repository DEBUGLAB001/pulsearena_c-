#pragma once
#include "model/tournament.hpp"
#include <string>

namespace pulsearena {

Tournament sampleTournament();
std::string stateJson(const Tournament& tournament);

} // namespace pulsearena
