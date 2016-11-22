#pragma once

#include "../Core/Defs.hpp"
#include "WaveFmt.hpp"

namespace doll
{

	DOLL_FUNC U32 DOLL_API snd_stringToChannelMask( const Str &src );
	DOLL_FUNC UPtr DOLL_API snd_channelMaskToString( char *pszDst, UPtr cDstMax, U32 uChannelMask );
	template< UPtr tDstMax >
	inline UPtr DOLL_API snd_channelMaskToString( char( &szDst )[ tDstMax ], U32 uChannelMask )
	{
		return snd_channelMaskToString( szDst, tDstMax, uChannelMask );
	}
	inline MutStr DOLL_API snd_channelMaskToString( U32 uChannelMask )
	{
		char szBuf[ 256 ];
		if( !snd_channelMaskToString( szBuf, sizeof( szBuf ), uChannelMask ) ) {
			return MutStr();
		}
		return MutStr( szBuf );
	}

}
