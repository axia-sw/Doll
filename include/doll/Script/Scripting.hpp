#pragma once

#include "../Core/Defs.hpp"

namespace doll {

	// Script compiler
	class RCompiler;

	// Create a new script compiler
	DOLL_FUNC RCompiler *DOLL_API sc_new();
	// Destroy a script compiler
	DOLL_FUNC NullPtr DOLL_API sc_delete( RCompiler *compiler );

	// Open a source file in the compiler
	DOLL_FUNC Bool DOLL_API sc_openSource( RCompiler *compiler, const Str &filename );
	// Close the current source and move to the next source
	DOLL_FUNC Bool DOLL_API sc_nextSource( RCompiler *compiler );
	// Check whether a source file is open
	DOLL_FUNC Bool DOLL_API sc_isOpen( const RCompiler *compiler );
	// Set a filename to use for the current source file, for diagnostic purposes
	DOLL_FUNC Bool DOLL_API sc_setSourceName( RCompiler *compiler, const Str &filename );
	// Retrieve the filename of the current source
	DOLL_FUNC Bool DOLL_API sc_getSourceName( const RCompiler *compiler, Str &dstFilename );
	inline Str DOLL_API sc_getSourceName( const RCompiler *compiler ) {
		Str src;
		(Void)sc_getSourceName( compiler, src );
		return src;
	}

	// Perform internal testing on the currently active source, using specially
	// written "test comments" written in that source
	DOLL_FUNC Bool DOLL_API sc_testCurrentSource( RCompiler *compiler );

}
