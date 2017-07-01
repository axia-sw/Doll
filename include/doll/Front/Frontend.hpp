#pragma once

#include "../Core/Defs.hpp"
#include "../Core/Engine.hpp"

namespace doll
{

	using namespace ax;

	DOLL_FUNC const char *DOLL_API doll_getEngineString();

	DOLL_FUNC Void DOLL_API doll_preInit();
	DOLL_FUNC Bool DOLL_API doll_init( const SCoreConfig *pConf = nullptr );
	DOLL_FUNC Bool DOLL_API doll_initConsoleApp();
	DOLL_FUNC Void DOLL_API doll_fini();

	DOLL_FUNC Bool DOLL_API doll_sync_app();
	DOLL_FUNC Void DOLL_API doll_sync_update();
	DOLL_FUNC Void DOLL_API doll_sync_render();
	DOLL_FUNC Void DOLL_API doll_sync_timing();

	DOLL_FUNC Bool DOLL_API doll_sync();

}
