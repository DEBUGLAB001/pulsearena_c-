#pragma once
#include "model/tournament.hpp"
#include <string>
#include <vector>

namespace pulsearena {

std::vector<Match> generateGroupSchedule(const std::string& group, const std::vector<std::string>& teamIds);

} // namespace pulsearena
