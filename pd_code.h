#pragma once

#include <array>
#include <string>
#include <vector>

namespace hki {

using Crossing = std::array<int, 4>;
using PDCode = std::vector<Crossing>;

PDCode parsePDCode(const std::string& text);
void validatePDCode(const PDCode& pd);
std::string formatPDCodeList(const PDCode& pd);
std::string formatPDCodeKnotTheory(const PDCode& pd);

} // namespace hki
