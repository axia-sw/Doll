#pragma once

#include "../Core/Defs.hpp"
#include "Ident.hpp"

namespace doll { namespace script {

	class CCompilerContext;

	enum EVersion: U32
	{
		kVer_1_0
	};

	// Initialize the compiler's dictionary with the given version's keywords and types
	Bool initCompilerDictionary( CCompilerContext &ctx, IdentDictionary &dstDict, EVersion ver );

}}