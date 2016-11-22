#pragma once

#include "../Core/Defs.hpp"

#include "CompilerMemory.hpp"
#include "Token.hpp"

namespace doll { namespace script {

	// A source file and all its state
	class Source: public CompilerObject
	{
	public:
		// Name of the file
		MutStr      filename;
		// Source buffer
		MutStr      buffer;

		// Index of this source file
		U16         index;
		static const U16 kMaxSources = U16( 1 )<<12;

		Source( CCompilerContext &ctx );
		virtual ~Source();
	};

}}
