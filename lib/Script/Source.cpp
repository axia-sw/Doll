#include "../BuildSettings.hpp"

#include "doll/Script/Source.hpp"
#include "doll/Script/Compiler.hpp"

namespace doll { namespace script {

	Source::Source( CCompilerContext &ctx )
	: CompilerObject( ctx )
	, filename()
	, buffer()
	, index( 0 )
	{
	}
	Source::~Source()
	{
	}

	DOLL_FUNC Bool DOLL_API scr_calcLineInfo( U32 &dstRow, U32 &dstCol, SourceLoc loc )
	{
		if( !loc.pSource || axstr_size_t( loc.uOffset ) > loc.pSource->buffer.len() ) {
			return false;
		}

		const char *const e = loc.pSource->buffer.pointer( axstr_size_t( loc.uOffset ) );
		const char *const s = loc.pSource->buffer.pointer();
		const char *b = s;

		U32 row = 1;
		for( const char *p = s; p < e; ++p ) {
			if( *p == '\r' ) {
				if( *( p + 1 ) == '\n' ) {
					++p;
				}

				++row;
				b = p + 1;
			} else if( *p == '\n' ) {
				++row;
				b = p + 1;
			}
		}

		const Str linetext( b, e );

		dstRow = row;
		dstCol = U32( linetext.numColumns() );

		return true;
	}

}}