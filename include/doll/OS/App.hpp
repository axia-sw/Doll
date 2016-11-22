#pragma once

#include "../Core/Defs.hpp"

namespace doll {

	DOLL_FUNC Void DOLL_API os_submitQuitEvent();
	DOLL_FUNC Bool DOLL_API os_receivedQuitEvent();

	DOLL_FUNC Bool DOLL_API os_waitForAndProcessEvent();
	DOLL_FUNC Bool DOLL_API os_processAllQueuedEvents();

}
