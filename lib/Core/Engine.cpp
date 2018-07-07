#include "../BuildSettings.hpp"
#include "doll/Core/Engine.hpp"

namespace doll
{

	SCoreStruc g_core;

	DOLL_FUNC SCoreStruc *DOLL_API doll_getCoreStruc()
	{
		return &g_core;
	}

}
