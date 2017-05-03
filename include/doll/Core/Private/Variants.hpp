#pragma once

#include "../Version.hpp"

#ifndef DOLL_BUILD_VARIANT
# define DOLL_BUILD_VARIANT DOLL_VARIANT_RELEASE
#endif

namespace doll {

	static constexpr EVariant kVariant = EVariant(DOLL_BUILD_VARIANT);

#define DOLL_DEF_VARIANT(MacroName_, SymbolName_) static constexpr bool MacroName_ = kVariant == EVariant::SymbolName_;
#include "Variants.def.hpp"
#undef DOLL_DEF_VARIANT

	constexpr const char *variantToString( EVariant variant ) {
		switch( variant ) {
#define DOLL_DEF_VARIANT(MacroName_, SymbolName_) case EVariant::SymbolName_: return #MacroName_;
#include "Variants.def.hpp"
#undef DOLL_DEF_VARIANT
		}

		return "unknown";
	}

}
