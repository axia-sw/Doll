#pragma once

#include "../Core/Defs.hpp"

#include "SysFS.hpp"

namespace doll
{

	DOLL_FUNC Bool DOLL_API core_loadFile( Str filename, U8 *&out_pDst, UPtr &out_cDstBytes, S32 tag = 0 );
	DOLL_FUNC Void DOLL_API core_freeFile( U8 *pSrc );

	DOLL_FUNC Bool DOLL_API core_readText( MutStr &outText, const Str &inFilename );

	template< typename T >
	inline Bool core_loadFile( const Str &filename, T *&out_pDst, UPtr &out_cDstBytes, S32 tag = 0 )
	{
		return core_loadFile( filename, ( U8 *& )out_pDst, out_cDstBytes, tag );
	}

	class RStreamFile;

	// Open a "streaming file" (uses unbuffered IO and is optimized for linearly
	// streaming a file into memory, e.g., for sound)
	//
	// cBufferBytes is the size in bytes that the internal buffer needs to be
	// allocated to. If it is not sector aligned for the given file then it will
	// be rounded up to the next sector boundary.
	//
	// cMinSectors specifies the minimum number of sectors the buffer should
	// contain. If cBufferBytes (after processing) does not contain at least
	// that many sectors then it will be rounded up to include that many.
	DOLL_FUNC RStreamFile *DOLL_API core_sfopen( Str filename, UPtr cBufferBytes = 0, UPtr cMinSectors = 4 );
	// Closes a file opened with `core_sfopen()` and returns nullptr
	DOLL_FUNC RStreamFile *DOLL_API core_sfclose( RStreamFile *pFile );

	// Retrieves the entire size of the streaming file
	DOLL_FUNC U64 DOLL_API core_sfsize( RStreamFile *pFile );

	// Retrieves the number of sectors we can fit into the buffer
	DOLL_FUNC UPtr DOLL_API core_sfsectorcount( const RStreamFile *pFile );
	// Retrieves the size (in bytes) of a sector
	DOLL_FUNC UPtr DOLL_API core_sfsectorsize( const RStreamFile *pFile );

	// Read from the file
	//
	// Returns the memory pointer to the section read from, or nullptr if the
	// read did not complete successfully.
	//
	// cOutBytes is set to the number of bytes successfully read.
	//
	// uDstSector specifies the internal sector of memory to write to. The
	// default value (~UPtr(0)) indicates that the internal sector written to
	// should be cycled through. (e.g., if the last sector written to was the
	// first internal sector then this operation would write to the second
	// internal sector, and a subsequent equivalent operation would write to the
	// third internal sector, etc.)
	//
	// If the number of bytes that would be made available for the given sector
	// would be less than *64* (due to sector alignment) then a whole additional
	// sector will be read if possible. (The number 64 is used as an average
	// cache size. A fixed number is used for its deterministic benefit.)
	DOLL_FUNC const Void *DOLL_API core_sfread( RStreamFile *pFile, UPtr &cOutBytes, UPtr uDstSector = ~UPtr(0) );

	// Returns true for an end-of-file (EOF) condition
	DOLL_FUNC Bool DOLL_API core_sfeof( const RStreamFile *pFile );

	// Jumps to a specific position within the file
	DOLL_FUNC Bool DOLL_API core_sfseek( RStreamFile *pFile, U64 uOffset );
	// Jumps ahead by a given number of bytes within the file
	DOLL_FUNC Bool DOLL_API core_sfskip( RStreamFile *pFile, S64 iOffset );
	// Retrieves the current position within the file
	DOLL_FUNC U64 DOLL_API core_sftell( const RStreamFile *pFile );

}
