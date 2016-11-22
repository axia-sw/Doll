#pragma once

#include "../Core/Defs.hpp"

namespace doll { namespace script {

	class Source;

	struct SourceLoc
	{
		const Source *pSource;
		U32           uOffset;

		SourceLoc()
		: pSource( nullptr )
		, uOffset( 0 )
		{
		}
		SourceLoc( const Source *pSource, U32 uOffset )
		: pSource( pSource )
		, uOffset( uOffset )
		{
		}

		SourceLoc( const SourceLoc & ) = default;
	};

	struct SourceRange: public SourceLoc
	{
		U32 cBytes;

		SourceRange()
		: SourceLoc()
		, cBytes( 0 )
		{
		}
		SourceRange( const Source *pSource, U32 uOffset, U32 cBytes )
		: SourceLoc( pSource, uOffset )
		, cBytes( cBytes )
		{
		}
		SourceRange( const SourceLoc &loc )
		: SourceLoc( loc )
		, cBytes( 0 )
		{
		}

		SourceRange( const SourceRange & ) = default;
	};

	DOLL_FUNC Bool DOLL_API scr_calcLineInfo( U32 &dstRow, U32 &dstCol, SourceLoc loc );

}}
