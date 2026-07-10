#pragma once

namespace hki {

void installInterruptHandlers();
bool interrupted();
void requestInterrupt();

} // namespace hki
