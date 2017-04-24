#pragma once

#define DOLL_VERSION_MAJOR 0
#define DOLL_VERSION_MINOR 5
#define DOLL_VERSION_PATCH 0

#define DOLL_VERSION       ( DOLL_VERSION_MAJOR*10000 + DOLL_VERSION_MINOR*100 + DOLL_VERSION_PATCH )

namespace doll
{

	enum class EVariant {
#define DOLL_DEF_VARIANT(MacroName_, SymbolName_) SymbolName_ = DOLL_VARIANT_##MacroName_,
#include "Private/Variants.def.hpp"
#undef DOLL_DEF_VARIANT
	};

	static const unsigned kVersionMajor = DOLL_VERSION_MAJOR;
	static const unsigned kVersionMinor = DOLL_VERSION_MINOR;
	static const unsigned kVersionPatch = DOLL_VERSION_PATCH;

	static const unsigned kVersion      = DOLL_VERSION;
}
