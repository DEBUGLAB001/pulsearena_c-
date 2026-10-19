#pragma once
#include "model/tournament.hpp"
#include <string>
#include <vector>

namespace pulsearena {

std::vector<Team> standings(const Tournament& tournament, const std::string& group);

} // namespace pulsearena
