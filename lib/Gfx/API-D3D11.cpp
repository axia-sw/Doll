#define DOLL_TRACE_FACILITY doll::kLog_GfxAPIDrv

#include "doll/Core/Defs.hpp"
#include "doll/Gfx/APIs.def.hpp"
#if DOLL_GFX_DIRECT3D11_ENABLED

#include "doll/Gfx/API-D3D11.hpp"

#include "doll/Core/Logger.hpp"

namespace doll
{

	class CGfxAPIProvider_D3D11: public IGfxAPIProvider {
	public:
		virtual Void drop() override {
		}

		virtual Bool is( const Str &name ) const override {
			return
				name.caseCmp( "dx" ) ||
				name.caseCmp( "dx11" ) ||
				name.caseCmp( "d3d" ) ||
				name.caseCmp( "d3d11" ) ||
				name.caseCmp( "directx" ) ||
				name.caseCmp( "directx11" ) ||
				name.caseCmp( "direct3d" ) ||
				name.caseCmp( "direct3d11" );
		}
		virtual Str getName() const override {
			return Str( "d3d11" );
		}
		virtual Str getDescription() const override {
			return Str( "Direct3D 11" );
		}
		virtual IGfxAPI *initAPI( OSWindow wnd, const SGfxInitDesc &desc ) override {
			return CGfxAPI_D3D11::init( wnd, desc, *this );
		}
		virtual Void finiAPI( IGfxAPI *pAPI ) override {
			delete pAPI;
		}
	};
	static CGfxAPIProvider_D3D11 direct3D11GfxAPIProvider_;
	IGfxAPIProvider &direct3D11GfxAPIProvider = direct3D11GfxAPIProvider_;

	CGfxAPI_D3D11::CGfxAPI_D3D11( IGfxAPIProvider &provider, SD3D11Context *pCtx )
	: IGfxAPI( provider )
	, m_pCtx( pCtx )
	{
		AX_ASSERT_NOT_NULL( pCtx );
	}
	CGfxAPI_D3D11::~CGfxAPI_D3D11()
	{
	}

	EGfxAPI CGfxAPI_D3D11::getAPI() const
	{
		return kGfxAPIDirect3D11;
	}

	TArr<EShaderFormat> CGfxAPI_D3D11::getSupportedShaderFormats() const
	{
		static const EShaderFormat formats[] = {
			kShaderFormatHLSL
		};
		return TArr<EShaderFormat>(formats);
	}

	Void CGfxAPI_D3D11::setDefaultState( const Mat4f &proj )
	{
		( ( Void )proj );
	}

	Void CGfxAPI_D3D11::resize( U32 uResX, U32 uResY )
	{
		( ( Void )uResX );
		( ( Void )uResY );
	}
	Void CGfxAPI_D3D11::getSize( U32 &uResX, U32 &uResY )
	{
		( ( Void )uResX );
		( ( Void )uResY );
	}

	Void CGfxAPI_D3D11::wsiPresent()
	{
	}

	IGfxAPISampler *CGfxAPI_D3D11::createSampler( const SGfxSamplerDesc &desc )
	{
		( ( Void )desc );

		return nullptr;
	}
	Void CGfxAPI_D3D11::destroySampler( IGfxAPISampler *pSampler )
	{
		( ( Void )pSampler );
	}

	IGfxAPITexture *CGfxAPI_D3D11::createTexture( ETextureFormat fmt, U16 resX, U16 resY, const U8 *pData )
	{
		( ( Void )fmt );
		( ( Void )resX );
		( ( Void )resY );
		( ( Void )pData );

		return nullptr;
	}
	Void CGfxAPI_D3D11::destroyTexture( IGfxAPITexture *pTex )
	{
		( ( Void )pTex );
	}

	IGfxAPIVLayout *CGfxAPI_D3D11::createLayout( const SGfxLayout &desc )
	{
		( ( Void )desc );
		return nullptr;
	}
	Void CGfxAPI_D3D11::destroyLayout( IGfxAPIVLayout *pVLayout )
	{
		( ( Void )pVLayout );
	}

	IGfxAPIVBuffer *CGfxAPI_D3D11::createVBuffer( UPtr cBytes, const Void *pData, EBufferPerformance performance, EBufferPurpose purpose )
	{
		( ( Void )cBytes );
		( ( Void )pData );
		( ( Void )performance );
		( ( Void )purpose );

		return nullptr;
	}
	IGfxAPIIBuffer *CGfxAPI_D3D11::createIBuffer( UPtr cBytes, const Void *pData, EBufferPerformance performance, EBufferPurpose purpose )
	{
		( ( Void )cBytes );
		( ( Void )pData );
		( ( Void )performance );
		( ( Void )purpose );

		return nullptr;
	}
	Void CGfxAPI_D3D11::destroyVBuffer( IGfxAPIVBuffer *pVBuf )
	{
		( ( Void )pVBuf );
	}
	Void CGfxAPI_D3D11::destroyIBuffer( IGfxAPIIBuffer *pIBuf )
	{
		( ( Void )pIBuf );
	}

	Void CGfxAPI_D3D11::vsSetProjectionMatrix( const F32 *matrix )
	{
		( ( Void )matrix );
	}
	Void CGfxAPI_D3D11::vsSetModelViewMatrix( const F32 *matrix )
	{
		( ( Void )matrix );
	}

	Void CGfxAPI_D3D11::psoSetScissorEnable( Bool enable )
	{
		( ( Void )enable );
	}
	Void CGfxAPI_D3D11::psoSetTextureEnable( Bool enable )
	{
		( ( Void )enable );
	}
	Void CGfxAPI_D3D11::psoSetBlend( EBlendOp op, EBlendFactor colA, EBlendFactor colB, EBlendFactor alphaA, EBlendFactor alphaB )
	{
		( ( Void )op );
		( ( Void )colA );
		( ( Void )colB );
		( ( Void )alphaA );
		( ( Void )alphaB );
	}

	Void CGfxAPI_D3D11::rsSetScissor( S32 posX, S32 posY, U32 resX, U32 resY )
	{
		( ( Void )posX );
		( ( Void )posY );
		( ( Void )resX );
		( ( Void )resY );
	}
	Void CGfxAPI_D3D11::rsSetViewport( S32 posX, S32 posY, U32 resX, U32 resY )
	{
		( ( Void )posX );
		( ( Void )posY );
		( ( Void )resX );
		( ( Void )resY );
	}

	Void CGfxAPI_D3D11::iaSetLayout( IGfxAPIVLayout *pVLayout )
	{
		( ( Void )pVLayout );
	}

	Void CGfxAPI_D3D11::tsBindTexture( IGfxAPITexture *pTex, U32 uStage )
	{
		( ( Void )pTex );
		( ( Void )uStage );
	}
	Void CGfxAPI_D3D11::tsBindSampler( IGfxAPISampler *pSampler, U32 uStage )
	{
		( ( Void )pSampler );
		( ( Void )uStage );
	}
	Void CGfxAPI_D3D11::iaBindVBuffer( IGfxAPIVBuffer *pVBuf )
	{
		( ( Void )pVBuf );
	}
	Void CGfxAPI_D3D11::iaBindIBuffer( IGfxAPIIBuffer *pIBuf )
	{
		( ( Void )pIBuf );
	}

	Void CGfxAPI_D3D11::cmdClearRect( S32 posX, S32 posY, U32 resX, U32 resY, U32 value )
	{
		( ( Void )posX );
		( ( Void )posY );
		( ( Void )resX );
		( ( Void )resY );
		( ( Void )value );
	}
	Void CGfxAPI_D3D11::cmdUpdateTexture( IGfxAPITexture *pTex, U16 posX, U16 posY, U16 resX, U16 resY, const U8 *pData )
	{
		( ( Void )pTex );
		( ( Void )posX );
		( ( Void )posY );
		( ( Void )resX );
		( ( Void )resY );
		( ( Void )pData );
	}
	Void CGfxAPI_D3D11::cmdWriteVBuffer( IGfxAPIVBuffer *pVBuf, UPtr offset, UPtr size, const Void *pData )
	{
		( ( Void )pVBuf );
		( ( Void )offset );
		( ( Void )size );
		( ( Void )pData );
	}
	Void CGfxAPI_D3D11::cmdWriteIBuffer( IGfxAPIIBuffer *pIBuf, UPtr offset, UPtr size, const Void *pData )
	{
		( ( Void )pIBuf );
		( ( Void )offset );
		( ( Void )size );
		( ( Void )pData );
	}
	Void CGfxAPI_D3D11::cmdReadVBuffer( IGfxAPIVBuffer *pVBuf, UPtr offset, UPtr size, Void *pData )
	{
		( ( Void )pVBuf );
		( ( Void )offset );
		( ( Void )size );
		( ( Void )pData );
	}
	Void CGfxAPI_D3D11::cmdReadIBuffer( IGfxAPIIBuffer *pIBuf, UPtr offset, UPtr size, Void *pData )
	{
		( ( Void )pIBuf );
		( ( Void )offset );
		( ( Void )size );
		( ( Void )pData );
	}

	Void CGfxAPI_D3D11::cmdDraw( ETopology topology, U32 cVerts, U32 uOffset )
	{
		( ( Void )topology );
		( ( Void )cVerts );
		( ( Void )uOffset );
	}
	Void CGfxAPI_D3D11::cmdDrawIndexed( ETopology topology, U32 cIndices, U32 uOffset, U32 uBias )
	{
		( ( Void )topology );
		( ( Void )cIndices );
		( ( Void )uOffset );
		( ( Void )uBias );
	}

	DOLL_FUNC CGfxAPI_D3D11 *DOLL_API gfx__api_init_d3d11( OSWindow wnd, const SGfxInitDesc &desc, IGfxAPIProvider &provider )
	{
		( ( Void )wnd );
		( ( Void )desc );

		static Bool didError = false;
		if( !didError ) {
			didError = true;
			DOLL_ERROR_LOG += "Direct3D 11 not yet implemented.";
		}

		return nullptr;
	}

}

#endif
