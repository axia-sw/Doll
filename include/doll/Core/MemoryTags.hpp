#pragma once

#include "Defs.hpp"

namespace doll
{

	//
	//	Add any new tags here
	//
	#define DOLL__TAGS()\
		DOLL__TAG( Null )       DOLL__TAG_DELIM\
		\
		DOLL__TAG( Config )     DOLL__TAG_DELIM\
		DOLL__TAG( FileSys )    DOLL__TAG_DELIM\
		DOLL__TAG( Sprite )     DOLL__TAG_DELIM\
		DOLL__TAG( Layer )      DOLL__TAG_DELIM\
		DOLL__TAG( Texture )    DOLL__TAG_DELIM\
		DOLL__TAG( Font )       DOLL__TAG_DELIM\
		DOLL__TAG( Poly )       DOLL__TAG_DELIM\
		DOLL__TAG( Console )    DOLL__TAG_DELIM\
		DOLL__TAG( Counter )    DOLL__TAG_DELIM\
		\
		DOLL__TAG( RenderMisc ) DOLL__TAG_DELIM\
		DOLL__TAG( Sound )      DOLL__TAG_DELIM\
		DOLL__TAG( UX )         DOLL__TAG_DELIM\
		DOLL__TAG( Script )     //

	//------------------------------------------------------------------------------

	enum:int
	{
	#define DOLL__TAG_DELIM ,
	#define DOLL__TAG( x ) kTag_##x
		DOLL__TAGS()
	#undef DOLL__TAG
	#undef DOLL__TAG_DELIM
	};

	static const char *const kTagNames[] = {
	#define DOLL__TAG_DELIM ,
	#define DOLL__TAG( x ) #x
		DOLL__TAGS()
	#undef DOLL__TAG
	#undef DOLL__TAG_DELIM
	};

	static const UPtr kMaxTags = sizeof( kTagNames )/sizeof( kTagNames[ 0 ] );

}
