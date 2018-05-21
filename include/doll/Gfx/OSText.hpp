#pragma once

#include "../Core/Defs.hpp"

#include "../Math/IntVector2.hpp"
#include "../Math/Rect.hpp"

namespace doll
{

#define DOLL_OSTEXT_LINE_COLOR 0xFF000000
#define DOLL_OSTEXT_FILL_COLOR 0xFFFFFFFF

	struct STextItem;

	class CTextureAtlas;
	class RTexture;

	DOLL_FUNC STextItem *DOLL_API gfx_newOSText( Str text, const SIntVector2 &size, U32 lineColor = DOLL_OSTEXT_LINE_COLOR, U32 fillColor = DOLL_OSTEXT_FILL_COLOR );
	DOLL_FUNC STextItem *DOLL_API gfx_deleteOSText( STextItem *pText );
	DOLL_FUNC Void DOLL_API gfx_ostext_fillCache( STextItem *pText );
	DOLL_FUNC U32 DOLL_API gfx_ostext_resX( const STextItem *pText );
	DOLL_FUNC U32 DOLL_API gfx_ostext_resY( const STextItem *pText );
	DOLL_FUNC const Void *DOLL_API gfx_ostext_getBits( const STextItem *pText );
	DOLL_FUNC RTexture *DOLL_API gfx_ostext_makeTexture( const STextItem *pText, CTextureAtlas *pDefAtlas = nullptr );

	DOLL_FUNC RTexture *DOLL_API gfx_renderOSText( Str text, const SIntVector2 &size, U32 lineColor = DOLL_OSTEXT_LINE_COLOR, U32 fillColor = DOLL_OSTEXT_FILL_COLOR, CTextureAtlas *pDefAtlas = nullptr );
	DOLL_FUNC void DOLL_API gfx_drawOSText( Str text, const SRect &area );

	DOLL_FUNC void DOLL_API gfx_measureOSText( Str text, SRect &dstArea );

}
