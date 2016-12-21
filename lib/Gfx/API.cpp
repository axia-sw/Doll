#include "doll/Gfx/API.hpp"

// FIXME: Find a better way to include files.

#if DOLL_GFX_OPENGL_ENABLED
# include "doll/Gfx/API-GL.hpp"
#endif
#if DOLL_GFX_VULKAN_ENABLED
# include "doll/Gfx/API-Vk.hpp"
#endif
#if DOLL_GFX_DIRECT3D11_ENABLED
# include "doll/Gfx/API-D3D11.hpp"
#endif
#if DOLL_GFX_DIRECT3D12_ENABLED
# include "doll/Gfx/API-D3D12.hpp"
#endif

#include "doll/Core/Logger.hpp"

#ifndef DOLL_GFXAPI_DEBUG_ENABLED
# define DOLL_GFXAPI_DEBUG_ENABLED AX_DEBUG_ENABLED
#endif

#if DOLL_GFXAPI_DEBUG_ENABLED
# define DOLL_GFXAPI_ASSERT_CURFRAME(P_) AX_ASSERT_MSG((P_)!=nullptr, "[gfxapi] Current frame not set")
#else
# define DOLL_GFXAPI_ASSERT_CURFRAME(P_) ((void)0)
#endif

namespace doll
{

	static CGfxFrame *g_pCurrentFrame = nullptr;
	static IGfxAPI *  g_pCurrentAPI   = nullptr;

	// ### TODO ### Higher performance transient (memory) vbuffer management
	//
	// Preferably make use of an offset so that we cycle through the buffer at
	// 32-byte aligned increments when the batch size is sufficiently small
	//
	// That way multiple batches don't necessarily result flushes due to buffer
	// overwrite

	CGfxFrame::CGfxFrame( IGfxAPI &ctx )
	: m_context( ctx )
	, m_uResX( 0 )
	, m_uResY( 0 )
	, m_proj2D()
	, m_pMemVBuf( nullptr )
	, m_cVBufBytes( 0 )
	{
		m_context.getSize( m_uResX, m_uResY );
	}
	CGfxFrame::~CGfxFrame()
	{
		m_context.destroyVBuffer( m_pMemVBuf );
		m_memVBuf = 0;
		m_cVBufBytes = 0;
	}

	void CGfxFrame::setDefaultState()
	{
		m_context.setDefaultState( m_proj2D );
	}

	void CGfxFrame::resize( U32 uResX, U32 uResY )
	{
		m_uResX = uResX;
		m_uResY = uResY;

		m_proj2D.loadOrthoProj( 0.0f, F32(uResX), F32(uResY), 0.0f, 0.0f, 1000.0f );
	}
	Void CGfxFrame::wsiPresent()
	{
		m_context.wsiPresent();
	}

	IGfxAPIVBuffer *CGfxFrame::getMemVBuf( UPtr cBytes )
	{
		if( m_pMemVBuf != nullptr && m_cVBufBytes <= cBytes ) {
			return m_pMemVBuf;
		}

		IGfxAPIVBuffer *const vbuf = m_context.createVBuffer( cBytes + 512 - cBytes%512, nullptr, kBufferPerfStream, kBufferPurposeDraw );
		if( !vbuf ) {
			return 0;
		}

		m_context.destroyVBuffer( m_pMemVBuf );
		m_pMemVBuf = vbuf;

		return m_pMemVBuf;
	}

	Void CGfxFrame::setLayout( SGfxLayout *p )
	{
		if( m_pLayout != p ) {
			m_pLayout = p;
			m_context.iaSetLayout( p != nullptr ? p->pAPIObj : nullptr );
		}
	}
	SGfxLayout *CGfxFrame::getLayout()
	{
		return m_pLayout;
	}
	Void CGfxFrame::finiLayout( SGfxLayout *p )
	{
		if( !p ) {
			return;
		}

		if( m_pLayout == p ) {
			m_pLayout = nullptr;
		}

		m_context.destroyLayout( p->pAPIObj );
	}
	IGfxAPIVLayout *CGfxFrame::compileLayout( const SGfxLayout &layout )
	{
		return m_context.createLayout( layout );
	}

	DOLL_FUNC IGfxAPI *DOLL_API gfx_initAPI( OSWindow wnd, const SGfxInitDesc *pInitDesc )
	{
#if DOLL__USE_GLFW
		AX_ASSERT_IS_NULL( wnd );
#else
		AX_ASSERT_NOT_NULL( wnd );
#endif

		if( !pInitDesc || pInitDesc->apis.isEmpty() ) {
			static const EGfxAPI defAPIs[] = {
#if defined( _WIN32 ) && !DOLL__USE_GLFW
				kGfxAPIDirect3D11,
#endif
				kGfxAPIOpenGL
			};

			SGfxInitDesc desc;
			desc.apis = defAPIs;
			if( pInitDesc != nullptr ) {
				desc.windowing = pInitDesc->windowing;
				desc.vsync     = pInitDesc->vsync;
			} else {
				desc.windowing = kGfxScreenModeWindowed;
				desc.vsync     = 0;
			}

			return gfx_initAPI( wnd, &desc );
		}

		for( EGfxAPI api : pInitDesc->apis ) {
			IGfxAPI *pAPI = nullptr;

			switch( api ) {
#define DOLL_GFX__API(Name_,BriefName_) \
	case kGfxAPI##Name_: \
		pAPI = CGfxAPI_##BriefName_::init( wnd, *pInitDesc ); \
		break;

#include "doll/Gfx/APIs.def.hpp"

#undef DOLL_GFX__API

			case kNumGfxAPIs:
				AX_UNREACHABLE();
			}

			if( pAPI != nullptr ) {
				return pAPI;
			}
		}

		return nullptr;
	}
	DOLL_FUNC NullPtr DOLL_API gfx_finiAPI( IGfxAPI *p )
	{
		delete p;
		return nullptr;
	}

	DOLL_FUNC Void DOLL_API gfx_r_setFrame( CGfxFrame *pFrame )
	{
		g_pCurrentFrame =  pFrame;
		g_pCurrentAPI   = &pFrame->getContext();
	}
	DOLL_FUNC CGfxFrame *DOLL_API gfx_r_getFrame()
	{
		return g_pCurrentFrame;
	}

	DOLL_FUNC U32 DOLL_API gfx_r_resX()
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentFrame );
		return g_pCurrentFrame->getResX();
	}
	DOLL_FUNC U32 DOLL_API gfx_r_resY()
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentFrame );
		return g_pCurrentFrame->getResY();
	}

	DOLL_FUNC Void DOLL_API gfx_r_loadProjection( const F32 *matrix )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->vsSetProjectionMatrix( matrix );
	}
	DOLL_FUNC Void DOLL_API gfx_r_loadModelView( const F32 *matrix )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->vsSetModelViewMatrix( matrix );
	}

	DOLL_FUNC Void DOLL_API gfx_r_enableScissor()
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->psoSetScissorEnable( true );
	}
	DOLL_FUNC Void DOLL_API gfx_r_disableScissor()
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->psoSetScissorEnable( false );
	}
	DOLL_FUNC Void DOLL_API gfx_r_setScissor( S32 posX, S32 posY, U32 resX, U32 resY )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->rsSetScissor( posX, posY, resX, resY );
	}
	DOLL_FUNC Void DOLL_API gfx_r_clearRect( S32 posX, S32 posY, U32 resX, U32 resY, U32 value )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->cmdClearRect( posX, posY, resX, resY, value );
	}

	DOLL_FUNC Void DOLL_API gfx_r_setViewport( S32 posX, S32 posY, U32 resX, U32 resY )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->rsSetViewport( posX, posY, resX, resY );
	}

	DOLL_FUNC UPtr DOLL_API gfx_r_createTexture( ETextureFormat fmt, U16 resX, U16 resY, const U8 *data )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		return (UPtr)g_pCurrentAPI->createTexture( fmt, resX, resY, data );
	}
	DOLL_FUNC Void DOLL_API gfx_r_destroyTexture( UPtr tex )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->destroyTexture( (IGfxAPITexture*)tex );
	}

	DOLL_FUNC Void DOLL_API gfx_r_updateTexture( UPtr tex, U16 posX, U16 posY, U16 resX, U16 resY, const U8 *data )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->cmdUpdateTexture( (IGfxAPITexture*)tex, posX, posY, resX, resY, data );
	}

	DOLL_FUNC Void DOLL_API gfx_r_setBlend( EBlendOp op, EBlendFactor srgb, EBlendFactor drgb, EBlendFactor sa, EBlendFactor da )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->psoSetBlend( op, srgb, drgb, sa, da );
	}

	DOLL_FUNC Void DOLL_API gfx_r_enableTexture2D()
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->psoSetTextureEnable( true );
	}
	DOLL_FUNC Void DOLL_API gfx_r_disableTexture2D()
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->psoSetTextureEnable( false );
	}

	DOLL_FUNC Void DOLL_API gfx_r_setTexture( UPtr tex, U32 stage )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->tsBindTexture( (IGfxAPITexture*)tex, stage );
	}

	DOLL_FUNC UPtr DOLL_API gfx_r_createLayout( UPtr stride )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentFrame );

		SGfxLayout *const p = new SGfxLayout();
		if( !AX_VERIFY_MEMORY( p ) ) {
			return 0;
		}

		p->stride    = stride;
		p->cElements = 0;

		p->pAPIObj   = nullptr;

		return UPtr( p );
	}
	DOLL_FUNC Void DOLL_API gfx_r_destroyLayout( UPtr layout )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentFrame );

		SGfxLayout *const p = ( SGfxLayout * )layout;
		if( !p ) {
			return;
		}

		g_pCurrentFrame->finiLayout( p );
		delete p;
	}

	static SGfxLayoutElement *gfx_r_nextElement( SGfxLayout *p, UPtr offset )
	{
		AX_ASSERT_NOT_NULL( p );
		AX_ASSERT( p->cElements < kMaxLayoutElements );

		if( offset == ~0U ) {
			if( p->cElements > 0 ) {
				offset = p->elements[p->cElements-1].uOffset + p->elements[p->cElements-1].cBytes;
			} else {
				offset = 0;
			}
		}

		SGfxLayoutElement *const pElm = &p->elements[ p->cElements++ ];
		pElm->uOffset = offset;

		return pElm;
	}
#if 0
	static UPtr gfx_r_nextOffset( const SGfxLayout *p )
	{
		AX_ASSERT_NOT_NULL( p );
		AX_ASSERT( p->cElements > 0 );

		return p->elements[ p->cElements - 1 ].uOffset + p->elements[ p->cElements - 1 ].cBytes;
	}
#endif

	inline Bool gfx_r_layoutHasVertex( const SGfxLayout *p )
	{
		for( UPtr i = 0; i < p->cElements; ++i ) {
			if( p->elements[ i ].type == kGfxLayoutElementVertex ) {
				return true;
			}
		}
		
		return false;
	}
	inline Bool gfx_r_layoutHasNormal( const SGfxLayout *p )
	{
		for( UPtr i = 0; i < p->cElements; ++i ) {
			if( p->elements[ i ].type == kGfxLayoutElementNormal ) {
				return true;
			}
		}

		return false;
	}
	inline Bool gfx_r_layoutHasColor( const SGfxLayout *p )
	{
		for( UPtr i = 0; i < p->cElements; ++i ) {
			if( p->elements[ i ].type == kGfxLayoutElementColor ) {
				return true;
			}
		}

		return false;
	}

	DOLL_FUNC Void DOLL_API gfx_r_layoutVertex( UPtr layout, EVectorSize size, EVectorType type, UPtr offset )
	{
		AX_ASSERT( !gfx_r_layoutHasVertex( ( SGfxLayout * )layout ) );

		SGfxLayoutElement *const p = gfx_r_nextElement( ( SGfxLayout * )layout, offset );

		p->type = kGfxLayoutElementVertex;
		
		p->cComps = size;
		p->compTy = type;

		p->cBytes = gfx_r_calcSize( size, type );
	}
	DOLL_FUNC Void DOLL_API gfx_r_layoutNormal( UPtr layout, EVectorType type, UPtr offset )
	{
		AX_ASSERT( !gfx_r_layoutHasNormal( ( SGfxLayout * )layout ) );

		SGfxLayoutElement *const p = gfx_r_nextElement( ( SGfxLayout * )layout, offset );

		p->type = kGfxLayoutElementNormal;

		p->cComps = kVectorSize3;
		p->compTy = type;

		p->cBytes = gfx_r_calcSize( kVectorSize3, type );
	}
	DOLL_FUNC Void DOLL_API gfx_r_layoutColor( UPtr layout, EVectorSize size, EVectorType type, UPtr offset )
	{
		AX_ASSERT( !gfx_r_layoutHasColor( ( SGfxLayout * )layout ) );

		SGfxLayoutElement *const p = gfx_r_nextElement( ( SGfxLayout * )layout, offset );

		p->type = kGfxLayoutElementColor;

		p->cComps = size;
		p->compTy = type;

		p->cBytes = gfx_r_calcSize( size, type );
	}
	DOLL_FUNC Void DOLL_API gfx_r_layoutTexCoord( UPtr layout, EVectorSize size, EVectorType type, UPtr offset )
	{
		SGfxLayoutElement *const p = gfx_r_nextElement( ( SGfxLayout * )layout, offset );

		p->type = kGfxLayoutElementTexCoord;

		p->cComps = size;
		p->compTy = type;

		p->cBytes = gfx_r_calcSize( size, type );
	}
	DOLL_FUNC Bool DOLL_API gfx_r_finishLayout( UPtr layout )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentFrame );

		AX_ASSERT_NOT_NULL( layout );
		SGfxLayout *const p = ( SGfxLayout * )layout;

		if( !p->cElements ) {
			return false;
		}

		if( p->stride == 0 ) {
			UPtr best = 0;
			for( UPtr i = 0; i < p->cElements; ++i ) {
				const UPtr test = p->elements[ i ].uOffset + p->elements[ i ].cBytes;
				if( best < test ) {
					best = test;
				}
			}

			if( !( p->stride = best ) ) {
				return false;
			}
		}

		if( !( p->pAPIObj = g_pCurrentFrame->compileLayout( *p ) ) ) {
			return false;
		}

		return true;
	}

	DOLL_FUNC Void DOLL_API gfx_r_setLayout( UPtr layout )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentFrame );
		AX_ASSERT_NOT_NULL( layout );
		g_pCurrentFrame->setLayout( ( SGfxLayout * )layout );
	}
	DOLL_FUNC UPtr DOLL_API gfx_r_getLayout()
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentFrame );
		return (UPtr)g_pCurrentFrame->getLayout();
	}

	DOLL_FUNC UPtr DOLL_API gfx_r_createVBuffer( UPtr size, const void *pData, EBufferPerformance perf, EBufferPurpose purpose )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		return (UPtr)g_pCurrentAPI->createVBuffer( size, pData, perf, purpose );
	}
	DOLL_FUNC UPtr DOLL_API gfx_r_createIBuffer( UPtr size, const void *pData, EBufferPerformance perf, EBufferPurpose purpose )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		return (UPtr)g_pCurrentAPI->createIBuffer( size, pData, perf, purpose );
	}

	DOLL_FUNC Bool DOLL_API gfx_r_writeVBuffer( UPtr vbuffer, UPtr offset, UPtr size, const void *pData )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		AX_ASSERT_NOT_NULL( vbuffer );
		g_pCurrentAPI->cmdWriteVBuffer( (IGfxAPIVBuffer*)vbuffer, offset, size, pData );
		return true;
	}
	DOLL_FUNC Bool DOLL_API gfx_r_writeIBuffer( UPtr ibuffer, UPtr offset, UPtr size, const void *pData )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		AX_ASSERT_NOT_NULL( ibuffer );
		g_pCurrentAPI->cmdWriteIBuffer( (IGfxAPIIBuffer*)ibuffer, offset, size, pData );
		return true;
	}

	DOLL_FUNC Bool DOLL_API gfx_r_readVBuffer( UPtr vbuffer, UPtr offset, UPtr size, void *pData )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		AX_ASSERT_NOT_NULL( vbuffer );
		g_pCurrentAPI->cmdReadVBuffer( (IGfxAPIVBuffer*)vbuffer, offset, size, pData );
		return true;
	}
	DOLL_FUNC Bool DOLL_API gfx_r_readIBuffer( UPtr ibuffer, UPtr offset, UPtr size, void *pData )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		AX_ASSERT_NOT_NULL( ibuffer );
		g_pCurrentAPI->cmdReadIBuffer( (IGfxAPIIBuffer*)ibuffer, offset, size, pData );
		return true;
	}

	DOLL_FUNC Void DOLL_API gfx_r_destroyVBuffer( UPtr vbuffer )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->destroyVBuffer( (IGfxAPIVBuffer*)vbuffer );
	}
	DOLL_FUNC Void DOLL_API gfx_r_destroyIBuffer( UPtr ibuffer )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->destroyIBuffer( (IGfxAPIIBuffer*)ibuffer );
	}

	DOLL_FUNC Void DOLL_API gfx_r_setVBuffer( UPtr vbuffer )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->iaBindVBuffer( (IGfxAPIVBuffer*)vbuffer );
	}
	DOLL_FUNC Void DOLL_API gfx_r_setIBuffer( UPtr ibuffer )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->iaBindIBuffer( (IGfxAPIIBuffer*)ibuffer );
	}

	DOLL_FUNC Void DOLL_API gfx_r_draw( ETopology mode, U32 cVerts, U32 uOffset )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->cmdDraw( mode, cVerts, uOffset );
	}
	DOLL_FUNC Void DOLL_API gfx_r_drawIndexed( ETopology mode, U32 cIndices, U32 uOffset, U32 uBias )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentAPI );
		g_pCurrentAPI->cmdDrawIndexed( mode, cIndices, uOffset, uBias );
	}
	DOLL_FUNC Void DOLL_API gfx_r_drawMem( ETopology mode, U32 cVerts, UPtr cStrideBytes, const void *pMem )
	{
		DOLL_GFXAPI_ASSERT_CURFRAME( g_pCurrentFrame );

		const SGfxLayout *const pLayout = g_pCurrentFrame->getLayout();
		AX_ASSERT_NOT_NULL( pLayout );

		const UPtr cBytes = pLayout->stride*cVerts;

		AX_ASSERT( !cStrideBytes || cStrideBytes == pLayout->stride );
		( void )cStrideBytes;

		IGfxAPIVBuffer *const vbuf = g_pCurrentFrame->getMemVBuf( cBytes );
		if( !vbuf ) {
			return;
		}

		gfx_r_writeVBuffer( (UPtr)vbuf, 0, cBytes, pMem );

		gfx_r_setVBuffer( (UPtr)vbuf );
		gfx_r_draw( mode, cVerts, 0 );
	}

}
