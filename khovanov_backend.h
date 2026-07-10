#pragma once

#include "pd_code.h"

#include <string>

namespace hki {

std::string computeKhovanov(const PDCode& pd);
PDCode simplifyPDCode(const PDCode& pd);

} // namespace hki
