#pragma once

#include "../Core/Defs.hpp"
#include "../Types/IntVector2.hpp"
#include "../Types/Rect.hpp"

#include "API.hpp"

namespace doll
{

	class RLayer;
	class CGfxFrame;

	static const U32 INVALID_VALUE = 0xFFFFFFFFUL;
	typedef U32 NRenderColor;

	enum ERenderCmdId
	{
		RCMD_NONE,

		RCMD_DRAW_DOT,
		RCMD_DRAW_LINE,
		RCMD_DRAW_RECT,
		RCMD_DRAW_ELLIPSE,
		RCMD_DRAW_ROUND_RECT,
		RCMD_DRAW_IMAGE,
		RCMD_SET_SCISSOR,
		RCMD_CLEAR_RECT,
		RCMD_BLEND
	};
	enum ECorner
	{
		CORNER_TOP_LEFT,
		CORNER_TOP_RIGHT,
		CORNER_BOTTOM_LEFT,
		CORNER_BOTTOM_RIGHT,

		CORNER_TL = CORNER_TOP_LEFT,
		CORNER_TR = CORNER_TOP_RIGHT,
		CORNER_BL = CORNER_BOTTOM_LEFT,
		CORNER_BR = CORNER_BOTTOM_RIGHT
	};

#pragma pack(push,1)
	struct SRenderCmd
	{
		U8 cmdId;
		U8 len;
	};
	struct SRenderVec2i
	{
		S16 x, y;
	};
	struct SRenderVec2f
	{
		F32 x, y;
	};
	struct SRenderMat2f
	{
		F32 m[2][2];
	};
	struct SRenderMat3f
	{
		F32 m[3][3];
	};
	struct SRenderMat4f
	{
		F32 m[4][4];
	};
	typedef U16 HRenderImageId;
	typedef U16 HRenderFont;

	struct SRenderCmd_DrawDot
	{
		SRenderCmd rcmd;

		SRenderVec2i origin;
		NRenderColor color;
	};
	struct SRenderCmd_DrawLine
	{
		SRenderCmd rcmd;

		SRenderVec2i origin;
		SRenderVec2i finish;
		NRenderColor color[ 2 ];
	};
	struct SRenderCmd_DrawRect
	{
		SRenderCmd rcmd;

		SRenderVec2i tl;
		SRenderVec2i br;

		NRenderColor outer[ 4 ];
		NRenderColor inner[ 4 ];
	};
	struct SRenderCmd_DrawEllipse
	{
		SRenderCmd rcmd;

		SRenderVec2i origin;
		SRenderVec2i extents;

		NRenderColor outer;
		NRenderColor inner[ 2 ];
	};
	struct SRenderCmd_DrawRoundRect
	{
		SRenderCmd rcmd;

		SRenderVec2i tl;
		SRenderVec2i br;

		NRenderColor outer[ 4 ];
		NRenderColor inner[ 4 ];

		short rounding[ 4 ];
	};
	struct SRenderCmd_DrawImage
	{
		SRenderCmd rcmd;

		SRenderVec2i dstPos;
		SRenderVec2i dstRes;

		SRenderVec2i srcPos;
		SRenderVec2i srcRes;

		NRenderColor diffuse[ 4 ];
		HRenderImageId diffuseImg;
	};
	struct SRenderCmd_SetScissor
	{
		SRenderCmd rcmd;

		SRenderVec2i tl;
		SRenderVec2i br;

		RLayer *layer;
	};
	struct SRenderCmd_ClearRect
	{
		SRenderCmd rcmd;

		SRenderVec2i tl;
		SRenderVec2i br;

		DWORD value;
	};
	struct SRenderCmd_Blend
	{
		SRenderCmd   rcmd;
		EBlendOp     op;
		EBlendFactor src;
		EBlendFactor dst;
	};
#pragma pack(pop)

	DOLL_FUNC RLayer *DOLL_API gfx_getDefaultLayer();
	DOLL_FUNC Void DOLL_API gfx_setCurrentLayer( RLayer *layer );
	DOLL_FUNC RLayer *DOLL_API gfx_getCurrentLayer();

	DOLL_FUNC Void DOLL_API gfx_clearQueue();
	DOLL_FUNC EResult DOLL_API gfx_queDrawDot( S32 x, S32 y, U32 color );
	DOLL_FUNC EResult DOLL_API gfx_queDrawLine( S32 sx, S32 sy, S32 ex, S32 ey, U32 c1, U32 c2 );
	DOLL_FUNC EResult DOLL_API gfx_queDrawRect( S32 l, S32 t, S32 r, S32 b, U32 outerColTL, U32 outerColTR, U32 outerColBL, U32 outerColBR, U32 innerColTL, U32 innerColTR, U32 innerColBL, U32 innerColBR );
	DOLL_FUNC EResult DOLL_API gfx_queDrawEllipse( S32 x, S32 y, S32 rx, S32 ry, U32 outerColor, U32 innerColorO, U32 innerColorI );
	DOLL_FUNC EResult DOLL_API gfx_queDrawRoundRect( S32 l, S32 t, S32 r, S32 b, S32 roundingTL, S32 roundingTR, S32 roundingBL, S32 roundingBR, U32 outerColTL, U32 outerColTR, U32 outerColBL, U32 outerColBR, U32 innerColTL, U32 innerColTR, U32 innerColBL, U32 innerColBR );
	DOLL_FUNC EResult DOLL_API gfx_queDrawImage( S32 dstPosX, S32 dstPosY, S32 dstResX, S32 dstResY, S32 srcPosX, S32 srcPosY, S32 srcResX, S32 srcResY, U32 colorTL, U32 colorTR, U32 colorBL, U32 colorBR, HRenderImageId image );
	DOLL_FUNC EResult DOLL_API gfx_queSetScissor( S32 l, S32 t, S32 r, S32 b );
	DOLL_FUNC EResult DOLL_API gfx_queClearRect( S32 l, S32 t, S32 r, S32 b, U32 value );
	DOLL_FUNC EResult DOLL_API gfx_queBlend( EBlendOp, EBlendFactor src, EBlendFactor dst );

	DOLL_FUNC EResult DOLL_API gfx_drawQueueNowGL( CGfxFrame *pFrame );

	//--------------------------------------------------------------------//

	DOLL_FUNC EResult DOLL_API gfx_ink( DWORD color );

	DOLL_FUNC EResult DOLL_API gfx_dot( S32 x, S32 y );
	DOLL_FUNC EResult DOLL_API gfx_line( S32 x1, S32 y1, S32 x2, S32 y2 );
	DOLL_FUNC EResult DOLL_API gfx_outline( S32 x1, S32 y1, S32 x2, S32 y2 );
	DOLL_FUNC EResult DOLL_API gfx_box( S32 l, S32 t, S32 r, S32 b );

	DOLL_FUNC EResult DOLL_API gfx_hgradBox( S32 l, S32 t, S32 r, S32 b, U32 lcolor, U32 rcolor );
	DOLL_FUNC EResult DOLL_API gfx_vgradBox( S32 l, S32 t, S32 r, S32 b, U32 tcolor, U32 bcolor );
	DOLL_FUNC EResult DOLL_API gfx_gradBox( S32 l, S32 t, S32 r, S32 b, U32 tlcolor, U32 trcolor, U32 blcolor, U32 brcolor );

	DOLL_FUNC EResult DOLL_API gfx_ellipse( S32 x, S32 y, S32 rx, S32 ry );
	DOLL_FUNC EResult DOLL_API gfx_circle( S32 x, S32 y, S32 radius );
	DOLL_FUNC EResult DOLL_API gfx_roundedBox( S32 l, S32 t, S32 r, S32 b, S32 radius );

	DOLL_FUNC EResult DOLL_API gfx_blitImage( U16 img, S32 x, S32 y );
	DOLL_FUNC EResult DOLL_API gfx_stretchImage( U16 img, S32 x, S32 y, S32 w, S32 h );
	DOLL_FUNC EResult DOLL_API gfx_blitSubimage( U16 img, S32 dstX, S32 dstY, S32 srcX, S32 srcY, S32 w, S32 h );
	DOLL_FUNC EResult DOLL_API gfx_stretchSubimage( U16 img, S32 dstX, S32 dstY, S32 dstW, S32 dstH, S32 srcX, S32 srcY, S32 srcW, S32 srcH );

	DOLL_FUNC EResult DOLL_API gfx_blitImageColored( U16 img, S32 x, S32 y, U32 tlcol, U32 trcol, U32 blcol, U32 brcol );

	DOLL_FUNC EResult DOLL_API gfx_light( S32 l, S32 t, S32 r, S32 b, float levelSnorm );

	inline EResult DOLL_API gfx_dot( const SIntVector2 &pos )
	{
		return gfx_dot( pos.x, pos.y );
	}
	inline EResult DOLL_API gfx_line( const SIntVector2 &s, const SIntVector2 &e )
	{
		return gfx_line( s.x, s.y, e.x, e.y );
	}
	inline EResult DOLL_API gfx_outline( const SRect &r )
	{
		return gfx_outline( r.x1, r.y1, r.x2, r.y2 );
	}
	inline EResult DOLL_API gfx_box( const SRect &r )
	{
		return gfx_box( r.x1, r.y1, r.x2, r.y2 );
	}
	inline EResult DOLL_API gfx_hgradBox( const SRect &r, U32 lcolor, U32 rcolor )
	{
		return gfx_hgradBox( r.x1, r.y1, r.x2, r.y2, lcolor, rcolor );
	}
	inline EResult DOLL_API gfx_vgradBox( const SRect &r, U32 tcolor, U32 bcolor )
	{
		return gfx_vgradBox( r.x1, r.y1, r.x2, r.y2, tcolor, bcolor );
	}
	inline EResult DOLL_API gfx_gradBox( const SRect &r, U32 tlcolor, U32 trcolor, U32 blcolor, U32 brcolor )
	{
		return gfx_gradBox( r.x1, r.y1, r.x2, r.y2, tlcolor, trcolor, blcolor, brcolor );
	}
	inline EResult DOLL_API gfx_ellipse( const SIntVector2 &pos, const SIntVector2 &radius )
	{
		return gfx_ellipse( pos.x, pos.y, radius.x, radius.y );
	}
	inline EResult DOLL_API gfx_circle( const SIntVector2 &pos, S32 radius )
	{
		return gfx_circle( pos.x, pos.y, radius );
	}
	inline EResult DOLL_API gfx_roundedBox( const SRect &r, S32 radius )
	{
		return gfx_roundedBox( r.x1, r.y1, r.x2, r.y2, radius );
	}
	inline EResult DOLL_API gfx_blitImage( U16 img, const SIntVector2 &pos )
	{
		return gfx_blitImage( img, pos.x, pos.y );
	}
	inline EResult DOLL_API gfx_stretchImage( U16 img, const SRect &r )
	{
		return gfx_stretchImage( img, r.x1, r.y1, r.resX(), r.resY() );
	}
	inline EResult DOLL_API gfx_blitSubimage( U16 img, const SIntVector2 &dstPos, const SRect &srcBox )
	{
		return gfx_blitSubimage( img, dstPos.x, dstPos.y, srcBox.x1, srcBox.y1, srcBox.resX(), srcBox.resY() );
	}
	inline EResult DOLL_API gfx_stretchSubimage( U16 img, const SRect &dstBox, const SRect &srcBox )
	{
		return gfx_stretchSubimage( img, dstBox.x1, dstBox.y1, dstBox.resX(), dstBox.resY(), srcBox.x1, srcBox.y1, srcBox.resX(), srcBox.resY() );
	}
	inline EResult DOLL_API gfx_blitImageColored( U16 img, const SIntVector2 &pos, U32 tlcol, U32 trcol, U32 blcol, U32 brcol )
	{
		return gfx_blitImageColored( img, pos.x, pos.y, tlcol, trcol, blcol, brcol );
	}

	inline EResult DOLL_API gfx_light( const SRect &box, float levelSnorm )
	{
		return gfx_light( box.x1, box.y1, box.x2, box.y2, levelSnorm );
	}

}
