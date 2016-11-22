#pragma once

#define DOLL_VERSION_MAJOR 0
#define DOLL_VERSION_MINOR 4
#define DOLL_VERSION_PATCH 0

#define DOLL_VERSION       ( DOLL_VERSION_MAJOR*10000 + DOLL_VERSION_MINOR*100 + DOLL_VERSION_PATCH )

namespace doll
{

	static const unsigned kVersionMajor = DOLL_VERSION_MAJOR;
	static const unsigned kVersionMinor = DOLL_VERSION_MINOR;
	static const unsigned kVersionPatch = DOLL_VERSION_PATCH;

	static const unsigned kVersion      = DOLL_VERSION;

}
