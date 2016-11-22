#include "doll/Gfx/RenderCommands.hpp"
#include "doll/Gfx/PrimitiveBuffer.hpp"
#include "doll/Gfx/Texture.hpp"
#include "doll/Gfx/API-GL.hpp"
#include "doll/Gfx/Layer.hpp"
#include "doll/Math/Math.hpp"
#include "doll/Core/Logger.hpp"

//#include <gl/GL.h>

// ### TODO ### - Stop using Windows-specific data structures

namespace doll
{

	static RLayer *         g_pCurrentLayer   = nullptr;
	static TMutArr<U8> *    g_pRenderCommands = nullptr;
	static PrimitiveBuffer *g_pRenderPrims    = nullptr;

	DOLL_FUNC Void DOLL_API gfx_setCurrentLayer( RLayer *layer )
	{
		if( !layer ) {
			layer = gfx_getDefaultLayer();
		}

		g_pCurrentLayer   = layer;

		g_pRenderCommands = !layer ? nullptr : &layer->renderCommands();
		g_pRenderPrims    = !layer ? nullptr : &layer->renderPrimitives();
	}
	DOLL_FUNC RLayer *DOLL_API gfx_getCurrentLayer()
	{
		return g_pCurrentLayer;
	}

	template< typename T >
	Void swap( T &a, T &b)
	{
		const T c = a; a = b; b = c;
	}

	Void getRect(RECT *rc, const SRenderVec2i *tl, const SRenderVec2i *br)
	{
		rc->left = tl->x;
		rc->top = tl->y;
		rc->right = br->x;
		rc->bottom = br->y;

		if( rc->left > rc->right ) {
			swap( rc->left, rc->right );
		}
		if( rc->top > rc->bottom ) {
			swap( rc->top, rc->bottom );
		}
	}

	DOLL_FUNC Void DOLL_API gfx_clearQueue()
	{
		g_pRenderCommands->clear();
	}
	DOLL_FUNC EResult DOLL_API gfx_queDrawDot( S32 x, S32 y, U32 color )
	{
		SRenderCmd_DrawDot cmd;

		cmd.rcmd.cmdId = RCMD_DRAW_DOT;
		cmd.rcmd.len = sizeof( cmd );

		cmd.origin.x = x;
		cmd.origin.y = y;
		cmd.color = color;

		const Bool r = g_pRenderCommands->append( sizeof( cmd ), ( const U8 * )&cmd );
		return r ? kSuccess : kError_OutOfMemory;
	}
	DOLL_FUNC EResult DOLL_API gfx_queDrawLine( S32 sx, S32 sy, S32 ex, S32 ey, U32 c1, U32 c2 )
	{
		SRenderCmd_DrawLine cmd;

		cmd.rcmd.cmdId = RCMD_DRAW_LINE;
		cmd.rcmd.len = sizeof( cmd );

		cmd.origin.x = sx;
		cmd.origin.y = sy;
		cmd.finish.x = ex;
		cmd.finish.y = ey + 1;

		cmd.color[ 0 ] = c1;
		cmd.color[ 1 ] = c2;

		const Bool r = g_pRenderCommands->append( sizeof( cmd ), ( const U8 * )&cmd );
		return r ? kSuccess : kError_OutOfMemory;
	}
	DOLL_FUNC EResult DOLL_API gfx_queDrawRect( S32 l, S32 t, S32 r, S32 b, U32 outerColTL, U32 outerColTR, U32 outerColBL, U32 outerColBR, U32 innerColTL, U32 innerColTR, U32 innerColBL, U32 innerColBR )
	{
		SRenderCmd_DrawRect cmd;

		cmd.rcmd.cmdId = RCMD_DRAW_RECT;
		cmd.rcmd.len = sizeof( cmd );

		cmd.tl.x = l;
		cmd.tl.y = t;
		cmd.br.x = r;
		cmd.br.y = b;

		cmd.outer[ CORNER_TOP_LEFT ]     = outerColTL;
		cmd.outer[ CORNER_TOP_RIGHT ]    = outerColTR;
		cmd.outer[ CORNER_BOTTOM_LEFT ]  = outerColBL;
		cmd.outer[ CORNER_BOTTOM_RIGHT ] = outerColBR;

		cmd.inner[ CORNER_TOP_LEFT ]     = innerColTL;
		cmd.inner[ CORNER_TOP_RIGHT ]    = innerColTR;
		cmd.inner[ CORNER_BOTTOM_LEFT ]  = innerColBL;
		cmd.inner[ CORNER_BOTTOM_RIGHT ] = innerColBR;

		const Bool x = g_pRenderCommands->append( sizeof( cmd ), ( const U8 * )&cmd );
		return x ? kSuccess : kError_OutOfMemory;
	}
	DOLL_FUNC EResult DOLL_API gfx_queDrawEllipse( S32 x, S32 y, S32 rx, S32 ry, U32 outerColor, U32 innerColorO, U32 innerColorI )
	{
		SRenderCmd_DrawEllipse cmd;

		cmd.rcmd.cmdId = RCMD_DRAW_ELLIPSE;
		cmd.rcmd.len = sizeof( cmd );

		cmd.origin.x = x;
		cmd.origin.y = y;
		cmd.extents.x = rx;
		cmd.extents.y = ry;
	
		cmd.outer = outerColor;

		cmd.inner[ 0 ] = innerColorO;
		cmd.inner[ 1 ] = innerColorI;

		const Bool r = g_pRenderCommands->append( sizeof( cmd ), ( const U8 * )&cmd );
		return r ? kSuccess : kError_OutOfMemory;
	}
	DOLL_FUNC EResult DOLL_API gfx_queDrawRoundRect( S32 l, S32 t, S32 r, S32 b, S32 roundingTL, S32 roundingTR, S32 roundingBL, S32 roundingBR, U32 outerColTL, U32 outerColTR, U32 outerColBL, U32 outerColBR, U32 innerColTL, U32 innerColTR, U32 innerColBL, U32 innerColBR )
	{
		SRenderCmd_DrawRoundRect cmd;

		cmd.rcmd.cmdId = RCMD_DRAW_ROUND_RECT;
		cmd.rcmd.len = sizeof( cmd );

		cmd.tl.x = l;
		cmd.tl.y = t;
		cmd.br.x = r;
		cmd.br.y = b;

		cmd.outer[ CORNER_TOP_LEFT ]     = outerColTL;
		cmd.outer[ CORNER_TOP_RIGHT ]    = outerColTR;
		cmd.outer[ CORNER_BOTTOM_LEFT ]  = outerColBL;
		cmd.outer[ CORNER_BOTTOM_RIGHT ] = outerColBR;

		cmd.inner[ CORNER_TOP_LEFT ]     = innerColTL;
		cmd.inner[ CORNER_TOP_RIGHT ]    = innerColTR;
		cmd.inner[ CORNER_BOTTOM_LEFT ]  = innerColBL;
		cmd.inner[ CORNER_BOTTOM_RIGHT ] = innerColBR;

		cmd.rounding[ CORNER_TOP_LEFT ]     = roundingTL;
		cmd.rounding[ CORNER_TOP_RIGHT ]    = roundingTR;
		cmd.rounding[ CORNER_BOTTOM_LEFT ]  = roundingBL;
		cmd.rounding[ CORNER_BOTTOM_RIGHT ] = roundingBR;

		const Bool x = g_pRenderCommands->append( sizeof( cmd ), ( const U8 * )&cmd );
		return x ? kSuccess : kError_OutOfMemory;
	}

	DOLL_FUNC EResult DOLL_API gfx_queDrawImage( S32 dstPosX, S32 dstPosY, S32 dstResX, S32 dstResY, S32 srcPosX, S32 srcPosY, S32 srcResX, S32 srcResY, U32 colorTL, U32 colorTR, U32 colorBL, U32 colorBR, HRenderImageId image )
	{
		SRenderCmd_DrawImage cmd;

		cmd.rcmd.cmdId = RCMD_DRAW_IMAGE;
		cmd.rcmd.len = sizeof( cmd );

		cmd.dstPos.x = dstPosX;
		cmd.dstPos.y = dstPosY;
		cmd.dstRes.x = dstResX;
		cmd.dstRes.y = dstResY;
		cmd.srcPos.x = srcPosX;
		cmd.srcPos.y = srcPosY;
		cmd.srcRes.x = srcResX;
		cmd.srcRes.y = srcResY;

		cmd.diffuse[ CORNER_TOP_LEFT ]     = colorTL;
		cmd.diffuse[ CORNER_TOP_RIGHT ]    = colorTR;
		cmd.diffuse[ CORNER_BOTTOM_LEFT ]  = colorBL;
		cmd.diffuse[ CORNER_BOTTOM_RIGHT ] = colorBR;

		cmd.diffuseImg = image;

		const Bool r = g_pRenderCommands->append( sizeof( cmd ), ( const U8 * )&cmd );
		return r ? kSuccess : kError_OutOfMemory;
	}
	DOLL_FUNC EResult DOLL_API gfx_queSetScissor( S32 l, S32 t, S32 r, S32 b )
	{
		SRenderCmd_SetScissor cmd;

		cmd.rcmd.cmdId = RCMD_SET_SCISSOR;
		cmd.rcmd.len = sizeof( cmd );

		cmd.tl.x = l;
		cmd.tl.y = t;
		cmd.br.x = r;
		cmd.br.y = b;

		cmd.layer = g_pCurrentLayer;

		const Bool x = g_pRenderCommands->append( sizeof( cmd ), ( const U8 * )&cmd );
		return x ? kSuccess : kError_OutOfMemory;
	}
	DOLL_FUNC EResult DOLL_API gfx_queClearRect( S32 l, S32 t, S32 r, S32 b, U32 value )
	{
		SRenderCmd_ClearRect cmd;

		cmd.rcmd.cmdId = RCMD_CLEAR_RECT;
		cmd.rcmd.len   = sizeof( cmd );

		cmd.tl.x = l;
		cmd.tl.y = t;
		cmd.br.x = r;
		cmd.br.y = b;

		cmd.value = value;

		const Bool x = g_pRenderCommands->append( sizeof( cmd ), ( const U8 * )&cmd );
		return x ? kSuccess : kError_OutOfMemory;
	}
	DOLL_FUNC EResult DOLL_API gfx_queBlend( EBlendOp op, EBlendFactor src, EBlendFactor dst )
	{
		SRenderCmd_Blend cmd;

		cmd.rcmd.cmdId = RCMD_BLEND;
		cmd.rcmd.len   = sizeof( cmd );

		cmd.op  = op;
		cmd.src = src;
		cmd.dst = dst;

		const Bool x = g_pRenderCommands->append( sizeof( cmd ), ( const U8 * )&cmd );
		return x ? kSuccess : kError_OutOfMemory;
	}

#define DRAW_NOWGL(what_)\
	Void draw##what_##GL(const SRenderCmd_Draw##what_ *cmd)
#define DO_NOWGL(what_)\
	Void do##what_##GL(const SRenderCmd_##what_ *cmd)

#define ASSERT_PARMSGL()\
	AX_ASSERT_MSG( cmd!=NULL, "NULL command pointer" )

	DRAW_NOWGL(Dot)
	{
		ASSERT_PARMSGL();

#if _MSC_VER
# pragma warning(push)
# pragma warning(disable:6011)
#endif
		g_pRenderPrims->setPrimitiveType( kTopologyPointList );
		g_pRenderPrims->setVertexFormat( PrimitiveConfig::kFormat_Colored );
		g_pRenderPrims->setTexture( 0 );

		g_pRenderPrims->color( cmd->color );
		g_pRenderPrims->vertex2i( cmd->origin.x, cmd->origin.y );
#if _MSC_VER
# pragma warning(pop)
#endif
	}
	DRAW_NOWGL(Line)
	{
		ASSERT_PARMSGL();

#if _MSC_VER
# pragma warning(push)
# pragma warning(disable:6011)
#endif
		g_pRenderPrims->setPrimitiveType( kTopologyLineList );
		g_pRenderPrims->setVertexFormat( PrimitiveConfig::kFormat_Colored );
		g_pRenderPrims->setTexture( 0 );

		g_pRenderPrims->color( cmd->color[0] );
		g_pRenderPrims->vertex2i( cmd->origin.x, cmd->origin.y );
		g_pRenderPrims->color( cmd->color[1] );
		g_pRenderPrims->vertex2i( cmd->finish.x, cmd->finish.y );
#if _MSC_VER
# pragma warning(pop)
#endif
	}
	DRAW_NOWGL(Rect)
	{
		ASSERT_PARMSGL();

#if _MSC_VER
# pragma warning(push)
# pragma warning(disable:6011)
#endif
		U32 totalAlpha = 0;
		totalAlpha += DOLL_COLOR_A( cmd->outer[CORNER_TL] );
		totalAlpha += DOLL_COLOR_A( cmd->outer[CORNER_TR] );
		totalAlpha += DOLL_COLOR_A( cmd->outer[CORNER_BL] );
		totalAlpha += DOLL_COLOR_A( cmd->outer[CORNER_BR] );

		g_pRenderPrims->setPrimitiveType( kTopologyTriangleList );
		g_pRenderPrims->setVertexFormat( PrimitiveConfig::kFormat_Colored );
		g_pRenderPrims->setTexture( 0 );

		const S32 l = cmd->tl.x;
		const S32 t = cmd->tl.y;
		const S32 r = cmd->br.x;
		const S32 b = cmd->br.y;

		g_pRenderPrims->color( cmd->inner[ CORNER_TL ] );
		g_pRenderPrims->vertex2i( l, t );
		g_pRenderPrims->color( cmd->inner[ CORNER_TR ] );
		g_pRenderPrims->vertex2i( r, t );
		g_pRenderPrims->color( cmd->inner[ CORNER_BL ] );
		g_pRenderPrims->vertex2i( l, b );
		g_pRenderPrims->color( cmd->inner[ CORNER_BR ] );
		g_pRenderPrims->vertex2i( r, b );
		g_pRenderPrims->color( cmd->inner[ CORNER_BL ] );
		g_pRenderPrims->vertex2i( l, b );
		g_pRenderPrims->color( cmd->inner[ CORNER_TR ] );
		g_pRenderPrims->vertex2i( r, t );

		if( !totalAlpha ) {
			return;
		}

		g_pRenderPrims->setPrimitiveType( kTopologyLineList );
		g_pRenderPrims->color( cmd->outer[ CORNER_TL ] );
		g_pRenderPrims->vertex2i( l, t );
		g_pRenderPrims->color( cmd->outer[ CORNER_TR ] );
		g_pRenderPrims->vertex2i( r - 1, t );
		g_pRenderPrims->vertex2i( r - 1, t );
		g_pRenderPrims->color( cmd->outer[ CORNER_BR ] );
		g_pRenderPrims->vertex2i( r - 1, b - 1 );
		g_pRenderPrims->vertex2i( r - 1, b - 1 );
		g_pRenderPrims->color( cmd->outer[ CORNER_BL ] );
		g_pRenderPrims->vertex2i( l, b - 1 );
		g_pRenderPrims->vertex2i( l, b - 1 );
		g_pRenderPrims->color( cmd->outer[ CORNER_TL ] );
		g_pRenderPrims->vertex2i( l, t );
#if _MSC_VER
# pragma warning(pop)
#endif
	}
	DRAW_NOWGL(Ellipse)
	{
		ASSERT_PARMSGL();

#if _MSC_VER
# pragma warning(push)
# pragma warning(disable:6011)
#endif
		U32 segmentCount = 3 + ( cmd->extents.x*cmd->extents.y )/64;
		if( segmentCount > 513 ) {
			segmentCount = 513;
		}

		SRenderVec2f a = { 0.0f, 0.0f };
		SRenderVec2f b = { 0.0f, 0.0f };

		g_pRenderPrims->setPrimitiveType( kTopologyTriangleList );
		g_pRenderPrims->setVertexFormat( PrimitiveConfig::kFormat_Colored );
		g_pRenderPrims->setTexture( 0 );

		b.x = cmd->origin.x + sinf( 0.0f )*cmd->extents.x;
		b.y = cmd->origin.y + cosf( 0.0f )*cmd->extents.y;

		U32 i = 0;
		U32 j = 0;
		const F32 s = ( 3.14159265358979f*2.0f )/F32( segmentCount );

		for( i=0; i<segmentCount; i=j ) {
			a = b;

			j = i + 1;
			b.x = cmd->origin.x + sinf( F32( j )*s )*cmd->extents.x;
			b.y = cmd->origin.y + cosf( F32( j )*s )*cmd->extents.y;

			g_pRenderPrims->color( cmd->inner[ 0 ] );
			g_pRenderPrims->vertex2f( cmd->origin.x, cmd->origin.y );

			g_pRenderPrims->color( cmd->inner[ 1 ] );
			g_pRenderPrims->vertex2f( a.x, a.y );

			g_pRenderPrims->color( cmd->inner[ 1 ] );
			g_pRenderPrims->vertex2f( b.x, b.y );
		}

		if( DOLL_COLOR_A( cmd->outer ) ) {
			return;
		}

		g_pRenderPrims->setPrimitiveType( kTopologyLineList );

		b.x = cmd->origin.x + sinf( 0.0f )*cmd->extents.x;
		b.y = cmd->origin.y + cosf( 0.0f )*cmd->extents.y;

		g_pRenderPrims->color( cmd->outer );
		for( i=0; i<segmentCount; i++ ) {
			a = b;

			j = i + 1;
			b.x = cmd->origin.x + sinf( F32( j )*s )*cmd->extents.x;
			b.y = cmd->origin.y + cosf( F32( j )*s )*cmd->extents.y;

			g_pRenderPrims->vertex2f( a.x, a.y );
			g_pRenderPrims->vertex2f( b.x, b.y );
		}
#if _MSC_VER
# pragma warning(pop)
#endif
	}
	DRAW_NOWGL(RoundRect)
	{
		ASSERT_PARMSGL();

#if _MSC_VER
# pragma warning(push)
# pragma warning(disable:6011)
#endif
		static const F32 off[ 4 ] = {
			RADIAN_QUAD1,
			RADIAN_QUAD2,
			RADIAN_QUAD3,
			RADIAN_QUAD4
		};
		const S32 l = cmd->tl.x;
		const S32 t = cmd->tl.y;
		const S32 r = cmd->br.x;
		const S32 b = cmd->br.y;
		const F32 cx = ( F32 )( l + ( r - l )/2 );
		const F32 cy = ( F32 )( t + ( b - t )/2 );
		F32 x;
		F32 y;
		U32 i;
		U32 j;
		SRenderVec2f last;
		SRenderVec2f cur;
		U32 segmentCount;
		F32 s;

		g_pRenderPrims->setPrimitiveType( kTopologyTriangleList );
		g_pRenderPrims->setVertexFormat( PrimitiveConfig::kFormat_Colored );
		g_pRenderPrims->setTexture( 0 );

		// top left
		segmentCount = 1 + cmd->rounding[ CORNER_TL ]/3;
		s = HALF_PI/F32( segmentCount );

		x = ( F32 )( l + cmd->rounding[ CORNER_TL ] );
		y = ( F32 )( t + cmd->rounding[ CORNER_TL ] );
		cur.x = x + sinf( off[ 2 ] + 0.0f )*cmd->rounding[ CORNER_TL ];
		cur.y = y + cosf( off[ 2 ] + 0.0f )*cmd->rounding[ CORNER_TL ];

		for( i=0; i<segmentCount; i=j ) {
			last = cur;
			j = i + 1;

			cur.x = x + sinf( off[ 2 ] + F32( j )*s )*cmd->rounding[ CORNER_TL ];
			cur.y = y + cosf( off[ 2 ] + F32( j )*s )*cmd->rounding[ CORNER_TL ];

			g_pRenderPrims->color( cmd->inner[ CORNER_TL ] );
			g_pRenderPrims->vertex2f( cx, cy);

			g_pRenderPrims->color( cmd->inner[ CORNER_TL ] );
			g_pRenderPrims->vertex2f( last.x, last.y );

			g_pRenderPrims->color( cmd->inner[ CORNER_TL ] );
			g_pRenderPrims->vertex2f( cur.x, cur.y );
		}

		g_pRenderPrims->color( cmd->inner[ CORNER_TL ] );
		g_pRenderPrims->vertex2f( cx, cy );

		g_pRenderPrims->color( cmd->inner[ CORNER_TL ] );
		g_pRenderPrims->vertex2i( l + cmd->rounding[ CORNER_TL ], t );

		g_pRenderPrims->color( cmd->inner[ CORNER_TR ] );
		g_pRenderPrims->vertex2i( r - cmd->rounding[ CORNER_TR ], t );

		// top right
		segmentCount = 1 + cmd->rounding[ CORNER_TR ]/3;
		s = HALF_PI/F32( segmentCount );

		x = ( F32 )( r - cmd->rounding[ CORNER_TR ] );
		y = ( F32 )( t + cmd->rounding[ CORNER_TR ] );
		cur.x = x + sinf( off[ 1 ] + 0.0f )*cmd->rounding[ CORNER_TR ];
		cur.y = y + cosf( off[ 1 ] + 0.0f )*cmd->rounding[ CORNER_TR ];

		for( i=0; i<segmentCount; i=j ) {
			last = cur;
			j = i + 1;

			cur.x = x + sinf( off[ 1 ] + F32( j )*s )*cmd->rounding[ CORNER_TR ];
			cur.y = y + cosf( off[ 1 ] + F32( j )*s )*cmd->rounding[ CORNER_TR ];

			g_pRenderPrims->color( cmd->inner[ CORNER_TR ] );
			g_pRenderPrims->vertex2f( cx, cy);

			g_pRenderPrims->color( cmd->inner[ CORNER_TR ] );
			g_pRenderPrims->vertex2f( last.x, last.y );

			g_pRenderPrims->color( cmd->inner[ CORNER_TR ] );
			g_pRenderPrims->vertex2f( cur.x, cur.y );
		}

		g_pRenderPrims->color( cmd->inner[ CORNER_TR ] );
		g_pRenderPrims->vertex2f( cx, cy );

		g_pRenderPrims->color( cmd->inner[ CORNER_TR ] );
		g_pRenderPrims->vertex2i( r, t + cmd->rounding[ CORNER_TR ] );

		g_pRenderPrims->color( cmd->inner[ CORNER_BR ] );
		g_pRenderPrims->vertex2i( r, b - cmd->rounding[ CORNER_BR ] );

		// bottom right
		segmentCount = 1 + cmd->rounding[ CORNER_BR ]/3;
		s = HALF_PI/F32( segmentCount );

		x = ( F32 )( r - cmd->rounding[ CORNER_BR ] );
		y = ( F32 )( b - cmd->rounding[ CORNER_BR ] );
		cur.x = x + sinf( off[ 0 ] ) * cmd->rounding[ CORNER_BR ];
		cur.y = y + cosf( off[ 0 ] ) * cmd->rounding[ CORNER_BR ];

		for( i=0; i<segmentCount; i=j ) {
			last = cur;
			j = i + 1;

			cur.x = x + sinf( off[ 0 ] + F32( j )*s )*cmd->rounding[ CORNER_BR ];
			cur.y = y + cosf( off[ 0 ] + F32( j )*s )*cmd->rounding[ CORNER_BR ];

			g_pRenderPrims->color( cmd->inner[ CORNER_BR ] );
			g_pRenderPrims->vertex2f( cx, cy);

			g_pRenderPrims->color( cmd->inner[ CORNER_BR ] );
			g_pRenderPrims->vertex2f( last.x, last.y );

			g_pRenderPrims->color( cmd->inner[ CORNER_BR ] );
			g_pRenderPrims->vertex2f( cur.x, cur.y );
		}

		g_pRenderPrims->color( cmd->inner[ CORNER_BR ] );
		g_pRenderPrims->vertex2f( cx, cy );

		g_pRenderPrims->color( cmd->inner[ CORNER_BR ] );
		g_pRenderPrims->vertex2i( r - cmd->rounding[ CORNER_BR ], b );

		g_pRenderPrims->color( cmd->inner[ CORNER_BL ] );
		g_pRenderPrims->vertex2i( l + cmd->rounding[ CORNER_BL ], b );

		// bottom left
		segmentCount = 1 + cmd->rounding[ CORNER_BL ]/3;
		s = HALF_PI/F32( segmentCount );

		x = ( F32 )( l + cmd->rounding[ CORNER_BL ] );
		y = ( F32 )( b - cmd->rounding[ CORNER_BL ] );
		cur.x = x + sinf( off[ 3 ] + 0.0f )*cmd->rounding[ CORNER_BL ];
		cur.y = y + cosf( off[ 3 ] + 0.0f )*cmd->rounding[ CORNER_BL ];

		for( i=0; i<segmentCount; i=j ) {
			last = cur;
			j = i + 1;

			cur.x = x + sinf( off[ 3 ] + F32( j )*s )*cmd->rounding[ CORNER_BL ];
			cur.y = y + cosf( off[ 3 ] + F32( j )*s )*cmd->rounding[ CORNER_BL ];

			g_pRenderPrims->color( cmd->inner[ CORNER_BL ] );
			g_pRenderPrims->vertex2f( cx, cy);

			g_pRenderPrims->color( cmd->inner[ CORNER_BL ] );
			g_pRenderPrims->vertex2f( last.x, last.y );

			g_pRenderPrims->color( cmd->inner[ CORNER_BL ] );
			g_pRenderPrims->vertex2f( cur.x, cur.y );
		}

		g_pRenderPrims->color( cmd->inner[ CORNER_BL ] );
		g_pRenderPrims->vertex2f( cx, cy );

		g_pRenderPrims->color( cmd->inner[ CORNER_BL ] );
		g_pRenderPrims->vertex2i( l, b - cmd->rounding[ CORNER_BL ] );

		g_pRenderPrims->color( cmd->inner[ CORNER_TL ] );
		g_pRenderPrims->vertex2i( l, t + cmd->rounding[ CORNER_TL ] );

#if _MSC_VER
# pragma warning(pop)
#endif
	}
	DRAW_NOWGL(Image)
	{
		ASSERT_PARMSGL();

#if _MSC_VER
# pragma warning(push)
# pragma warning(disable:6011)
#endif

#if 0 // ? WTF was this ?
		S32 s = kuGetInternalState_int( 25 );
		if( s > 0 ) {
			kuSetInternalState_int( 25, s - 1 );
			DebugListStates( "kuneDebug-drawImage-states.log" );
		}
#endif

		RTexture *const tex = g_textureMgr.getTextureById( cmd->diffuseImg );
		if( !tex ) {
			return;
		}

		UPtr gltex = tex->getBackingTexture();
		if( !gltex ) {
			return;
		}

		g_pRenderPrims->setPrimitiveType( kTopologyTriangleList );
		g_pRenderPrims->setVertexFormat( PrimitiveConfig::kFormat_Textured );
		g_pRenderPrims->setTexture( gltex );

		STextureRect texRect;
		const SPixelRect srcRect = {
			{ ( U16 )cmd->srcPos.x, ( U16 )cmd->srcPos.y },
			{ ( U16 )cmd->srcRes.x, ( U16 )cmd->srcRes.y }
		};

		tex->translateCoordinates( texRect, srcRect );

#if 0
		const F32 x = (F32)tex->getResolution().x;
		const F32 y = (F32)tex->getResolution().y;
#endif

		const S32 l = cmd->dstPos.x;
		const S32 t = cmd->dstPos.y;
		const S32 r = l + cmd->dstRes.x;
		const S32 b = t + cmd->dstRes.y;

#define TEXFLIP 1

		const F32 tL = texRect.pos[ 0 ];
		const F32 tR = texRect.res[ 0 ] + texRect.pos[ 0 ];

#if !TEXFLIP
		const F32 tT = texRect.pos[ 1 ];
		const F32 tB = texRect.res[ 1 ] + texRect.pos[ 1 ];
#else
		const F32 tT = texRect.pos[ 1 ] + texRect.res[ 1 ];
		const F32 tB = texRect.pos[ 1 ];
#endif

#if 0
		static int iDebugCount = 0;
		if( iDebugCount < 2 ) {
			++iDebugCount;

			g_DebugLog += axf( "L=%.4f; R=%.4f :: T=%.4f; B=%.4F\n", tL, tR, tT, tB );
		}
#endif

		g_pRenderPrims->color( cmd->diffuse[ CORNER_TL ] );
		g_pRenderPrims->texcoord2f( tL, tT );
		g_pRenderPrims->vertex2i( l, t );
		g_pRenderPrims->color( cmd->diffuse[ CORNER_TR ] );
		g_pRenderPrims->texcoord2f( tR, tT );
		g_pRenderPrims->vertex2i( r, t );
		g_pRenderPrims->color( cmd->diffuse[ CORNER_BL ] );
		g_pRenderPrims->texcoord2f( tL, tB );
		g_pRenderPrims->vertex2i( l, b );

		g_pRenderPrims->color( cmd->diffuse[ CORNER_BL ] );
		g_pRenderPrims->texcoord2f( tL, tB );
		g_pRenderPrims->vertex2i( l, b );
		g_pRenderPrims->color( cmd->diffuse[ CORNER_TR ] );
		g_pRenderPrims->texcoord2f( tR, tT );
		g_pRenderPrims->vertex2i( r, t );
		g_pRenderPrims->color( cmd->diffuse[ CORNER_BR ] );
		g_pRenderPrims->texcoord2f( tR, tB );
		g_pRenderPrims->vertex2i( r, b );
#if _MSC_VER
# pragma warning(pop)
#endif
	}
	DO_NOWGL(SetScissor)
	{
		ASSERT_PARMSGL();

#if _MSC_VER
# pragma warning(push)
# pragma warning(disable:6011)
#endif
		RECT rc;
		getRect( &rc, &cmd->tl, &cmd->br );

		RLayer *const layer = cmd->layer;
		AX_ASSERT_NOT_NULL( layer );

		//if( layer->getRestrictScissor() ) {
		if( true ) {
#if 0
			const SIntVector2 TL = layer->localToGlobal( SIntVector2( cmd->tl.x, cmd->tl.y ) );
			const SIntVector2 BR = layer->localToGlobal( SIntVector2( cmd->br.x, cmd->br.y ) );
#endif

			const SRect &Shape = layer->view().shape;
			const SIntVector2 ShapeTL = layer->localToGlobal( SIntVector2( 0, 0 ) );
			const SIntVector2 ShapeBR = layer->localToGlobal( Shape.size() );

			S32 rs[ 4 ] = { ShapeTL.x, ShapeTL.y, ShapeBR.x, ShapeBR.y };

			// the requested scissor rectangle must be capped to the current layer's
			// restriction setting
			if( rc.left < rs[ 0 ] ) {
				rc.left = rs[ 0 ];
			}
			if( rc.top < rs[ 1 ] ) {
				rc.top = rs[ 1 ];
			}
			if( rc.right > rs[ 2 ] ) {
				rc.right = rs[ 2 ];
			}
			if( rc.bottom > rs[ 3 ] ) {
				rc.bottom = rs[ 3 ];
			}
		}

		g_pRenderPrims->submit();

		gfx_r_setScissor( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top );
#if _MSC_VER
# pragma warning(pop)
#endif
	}
	DO_NOWGL(ClearRect)
	{
		ASSERT_PARMSGL();

#if _MSC_VER
# pragma warning(push)
# pragma warning(disable:6011)
#endif
		g_pRenderPrims->submit();
	
		RECT rc;
		getRect( &rc, &cmd->tl, &cmd->br );

		F32 offX, offY;
		g_pRenderPrims->getOffset( &offX, &offY );
		rc.left += ( S32 )offX;
		rc.top += ( S32 )offY;
		rc.right += ( S32 )offX;
		rc.bottom += ( S32 )offY;

		gfx_r_clearRect( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, cmd->value );
#if _MSC_VER
# pragma warning(pop)
#endif
	}
	DO_NOWGL( Blend )
	{
		ASSERT_PARMSGL();

#if _MSC_VER
# pragma warning(push)
# pragma warning(disable:6011)
#endif
		g_pRenderPrims->submit();
		gfx_r_setBlend( cmd->op, cmd->src, cmd->dst, cmd->src, cmd->dst );
#if _MSC_VER
# pragma warning(pop)
#endif
	}
#undef DO_NOWGL
#undef DRAW_NOWGL

	DOLL_FUNC EResult DOLL_API gfx_drawQueueNowGL( CGfxFrame *pFrame )
	{
#if _MSC_VER
# pragma warning(push)
# pragma warning(disable:6011)
#endif
		if( !AX_VERIFY_MSG( pFrame != NULL, "Expected non-NULL CGfxFrame instance" ) ) {
			return kError_InvalidParameter;
		}
		RLayer *const layer = gfx_getCurrentLayer();
		if( !AX_VERIFY_MSG( layer != NULL, "Expected a current layer to be set" ) ) {
			return kError_InvalidOperation;
		}

		gfx_r_setFrame( pFrame );

#if 0
		static const D3DMATRIX ident = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

		S32 x = 0, y = 0;
		S32 sx1, sy1, sx2, sy2;
		layer->getScissorRect( &sx1, &sy1, &sx2, &sy2 );
		layer->clientToScreen( &x, &y );
		x -= sx1;
		y -= sy1;
		D3DMATRIX tform = ident;
		tform._41 = F32( x );
		tform._42 = F32( y );
#endif

		pFrame->setDefaultState();

		const SViewport topView = g_layerMgr->renderer().topViewport();
		const SIntVector2 offset = topView.origin - topView.shape.origin();

		const F32 x = F32( offset.x + layer->getOffset().x ) - topView.origin.x;
		const F32 y = F32( offset.y + layer->getOffset().y ) - topView.origin.y;

#if 0
		static const F32 tform[ 16 ] = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			x, y, 0, 1
		};

		glMatrixMode( GL_MODELVIEW );
		glLoadMatrixf( tform );
#else
		//glMatrixMode( GL_MODELVIEW );
		//glLoadIdentity();
#endif

		gfx_r_enableScissor();

#if 0
		//RECT rc;

# if 0
		D3DVIEWPORT9 d3dvp = { 0, 0, 0, 0, 0, 0 };
		device->GetViewport( &d3dvp );

		rc.left = d3dvp.X;
		rc.top = d3dvp.Y;
		rc.right = rc.left + d3dvp.Width;
		rc.bottom = rc.top + d3dvp.Height;
# else
#  if 0
		//layer->getPosition( ( S32 * )&rc.left, ( S32 * )&rc.top );
		layer->getScissorRect( ( S32 * )&rc.left, ( S32 * )&rc.top,
			( S32 * )&rc.right, ( S32 * )&rc.bottom );
#  endif
# endif

		//device->SetScissorRect( &rc );
#endif

		g_pRenderPrims->reset();
		g_pRenderPrims->setOffset( -x, -y );

		UPtr size = g_pRenderCommands->num();
		const U8 *buffer = g_pRenderCommands->pointer();
		for( UPtr index = 0, nextIndex = 0; index < size; index = nextIndex ) {
			AX_ASSERT_MSG( index + sizeof(SRenderCmd) <= size, "Invalid state" );

			const SRenderCmd *cmd = (const SRenderCmd *)&buffer[ index ];
			nextIndex = index + (UPtr)( cmd->len );

			switch( cmd->cmdId ) {
			case RCMD_NONE:
				break;
			case RCMD_DRAW_DOT:
				drawDotGL( (const SRenderCmd_DrawDot *)cmd );
				break;
			case RCMD_DRAW_LINE:
				drawLineGL( (const SRenderCmd_DrawLine *)cmd );
				break;
			case RCMD_DRAW_RECT:
				drawRectGL( (const SRenderCmd_DrawRect *)cmd );
				break;
			case RCMD_DRAW_ELLIPSE:
				drawEllipseGL( (const SRenderCmd_DrawEllipse *)cmd );
				break;
			case RCMD_DRAW_ROUND_RECT:
				drawRoundRectGL( (const SRenderCmd_DrawRoundRect *)cmd );
				break;
			case RCMD_DRAW_IMAGE:
				drawImageGL( (const SRenderCmd_DrawImage *)cmd );
				break;
			case RCMD_SET_SCISSOR:
				doSetScissorGL( (const SRenderCmd_SetScissor *)cmd );
				break;
			case RCMD_CLEAR_RECT:
				doClearRectGL( (const SRenderCmd_ClearRect *)cmd );
				break;
			case RCMD_BLEND:
				doBlendGL( (const SRenderCmd_Blend *)cmd );
				break;
			}
		}

		g_pRenderPrims->submit();

		return kSuccess;
#if _MSC_VER
# pragma warning(pop)
#endif
	}


	//--------------------------------------------------------------------//

	static U32 g_curInk = 0xFFFFFFFF;

	DOLL_FUNC EResult DOLL_API gfx_ink( DWORD color )
	{
		g_curInk = color;
		return kSuccess;
	}

	DOLL_FUNC EResult DOLL_API gfx_dot( S32 x, S32 y )
	{
		return gfx_queDrawDot( x, y, g_curInk );
	}

	DOLL_FUNC EResult DOLL_API gfx_line( S32 x1, S32 y1, S32 x2, S32 y2 )
	{
		return gfx_queDrawLine( x1, y1, x2, y2, g_curInk, g_curInk );
	}

	DOLL_FUNC EResult DOLL_API gfx_outline( S32 x1, S32 y1, S32 x2, S32 y2 )
	{
		const EResult err1 = gfx_queDrawLine( x1, y1, x2, y1, g_curInk, g_curInk );
		const EResult err2 = gfx_queDrawLine( x2, y1, x2, y2, g_curInk, g_curInk );
		const EResult err3 = gfx_queDrawLine( x2, y2, x1, y2, g_curInk, g_curInk );
		const EResult err4 = gfx_queDrawLine( x1, y2, x1, y1, g_curInk, g_curInk );

		if( err1 != kSuccess ) { return err1; }
		if( err2 != kSuccess ) { return err2; }
		if( err3 != kSuccess ) { return err3; }
		if( err4 != kSuccess ) { return err4; }

		return kSuccess;
	}

	DOLL_FUNC EResult DOLL_API gfx_box( S32 l, S32 t, S32 r, S32 b )
	{
		return gfx_queDrawRect( l, t, r, b, 0, 0, 0, 0, g_curInk, g_curInk, g_curInk, g_curInk );
	}

	DOLL_FUNC EResult DOLL_API gfx_hgradBox( S32 l, S32 t, S32 r, S32 b, U32 lcolor, U32 rcolor )
	{
		return gfx_queDrawRect( l, t, r, b, 0, 0, 0, 0, lcolor, rcolor, lcolor, rcolor );
	}

	DOLL_FUNC EResult DOLL_API gfx_vgradBox( S32 l, S32 t, S32 r, S32 b, U32 tcolor, U32 bcolor )
	{
		return gfx_queDrawRect( l, t, r, b, 0, 0, 0, 0, tcolor, tcolor, bcolor, bcolor );
	}

	DOLL_FUNC EResult DOLL_API gfx_gradBox( S32 l, S32 t, S32 r, S32 b, U32 tlcolor, U32 trcolor, U32 blcolor, U32 brcolor )
	{
		return gfx_queDrawRect( l, t, r, b, 0, 0, 0, 0, tlcolor, trcolor, blcolor, brcolor );
	}

	DOLL_FUNC EResult DOLL_API gfx_ellipse( S32 x, S32 y, S32 rx, S32 ry )
	{
		return gfx_queDrawEllipse( x, y, rx, ry, 0, g_curInk, g_curInk );
	}

	DOLL_FUNC EResult DOLL_API gfx_circle( S32 x, S32 y, S32 radius )
	{
		return gfx_queDrawEllipse( x, y, radius, radius, 0, g_curInk, g_curInk );
	}

	DOLL_FUNC EResult DOLL_API gfx_roundedBox( S32 l, S32 t, S32 r, S32 b, S32 radius )
	{
		return gfx_queDrawRoundRect( l, t, r, b, radius, radius, radius, radius, 0, 0, 0, 0, g_curInk, g_curInk, g_curInk, g_curInk );
	}

	DOLL_FUNC EResult DOLL_API gfx_blitImage( U16 img, S32 x, S32 y )
	{
		const RTexture *tex;

		if( !AX_VERIFY_MSG( img != 0, "Invalid image" ) )
			return kError_InvalidParameter;

		tex = g_textureMgr.getTextureById( img );

		const SPixelVec2 res = tex->getResolution();

		return gfx_queDrawImage( x, y, res.x, res.y, 0, 0, res.x, res.y, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, img );
	}

	DOLL_FUNC EResult DOLL_API gfx_stretchImage( U16 img, S32 x, S32 y, S32 w, S32 h )
	{
		const RTexture *tex;

		if( !AX_VERIFY_MSG( img != 0, "Invalid image" ) )
			return kError_InvalidParameter;

		tex = g_textureMgr.getTextureById( img );

		const SPixelVec2 res = tex->getResolution();

		return gfx_queDrawImage( x, y, w, h, 0, 0, res.x, res.y, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, img );
	}

	DOLL_FUNC EResult DOLL_API gfx_blitSubimage( U16 img, S32 dstX, S32 dstY, S32 srcX, S32 srcY, S32 w, S32 h )
	{
		const RTexture *tex;

		if( !AX_VERIFY_MSG( img != 0, "Invalid image" ) ) {
			return kError_InvalidParameter;
		}

		tex = g_textureMgr.getTextureById( img );

		SPixelVec2 res = tex->getResolution();
		if( !w ) {
			w = res.x;
		}
		if( !h ) {
			h = res.y;
		}

		return gfx_queDrawImage( dstX, dstY, w, h, srcX, srcY, w, h, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, img );
	}

	DOLL_FUNC EResult DOLL_API gfx_stretchSubimage( U16 img, S32 dstX, S32 dstY, S32 dstW, S32 dstH, S32 srcX, S32 srcY, S32 srcW, S32 srcH )
	{
		if( !AX_VERIFY_MSG( img != 0, "Invalid image" ) ) {
			return kError_InvalidParameter;
		}

		return gfx_queDrawImage( dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, img );
	}

	DOLL_FUNC EResult DOLL_API gfx_blitImageColored( U16 img, S32 x, S32 y, U32 tlcol, U32 trcol, U32 blcol, U32 brcol )
	{
		const RTexture *tex;

		if( !AX_VERIFY_MSG( img != 0, "Invalid image" ) )
			return kError_InvalidParameter;

		tex = g_textureMgr.getTextureById( img );

		const SPixelVec2 res = tex->getResolution();

		return gfx_queDrawImage( x, y, res.x, res.y, 0, 0, res.x, res.y, tlcol, trcol, blcol, brcol, img );
	}

	DOLL_FUNC EResult DOLL_API gfx_light( S32 l, S32 t, S32 r, S32 b, float levelSnorm )
	{
		const Bool n = levelSnorm < 0;
		const F32 f  = saturate( n ? -levelSnorm : levelSnorm );
		const U32 g  = U32( f*255 );
		const U32 c  = DOLL_RGBA( g, g, g, g );

		EResult res;

		if( n ) {
			res = gfx_queBlend( kBlendAdd, kBlendSrcAlpha, kBlendInvSrcAlpha );
		} else {
			res = gfx_queBlend( kBlendAdd, kBlendOne, kBlendInvSrcColor );
		}
		if( res != kSuccess ) {
			return res;
		}

		if( ( res = gfx_queDrawRect( l, t, r, b, 0, 0, 0, 0, c, c, c, c ) ) != kSuccess ) {
			return res;
		}

		if( !n ) {
			res = gfx_queBlend( kBlendAdd, kBlendSrcAlpha, kBlendInvSrcAlpha );
		}

		return res;
	}

}
