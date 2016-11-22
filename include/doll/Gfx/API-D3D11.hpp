#pragma once

#include "../Core/Defs.hpp"

#include "Vertex.hpp"
#include "API.hpp"

namespace doll
{

	struct SD3D11Context;
	class CGfxAPI_D3D11;

	DOLL_FUNC CGfxAPI_D3D11 *DOLL_API gfx__api_init_d3d11( OSWindow wnd, const SGfxInitDesc &desc );

	class CGfxAPI_D3D11: public virtual IGfxAPI
	{
	public:
		CGfxAPI_D3D11( SD3D11Context *pCtx );
		virtual ~CGfxAPI_D3D11();

		virtual EGfxAPI getAPI() const override;

		virtual Void setDefaultState( const Mat4f &proj ) override;

		virtual Void resize( U32 uResX, U32 uResY ) override;
		virtual Void getSize( U32 &uResX, U32 &uResY ) override;

		virtual Void wsiPresent() override;

		virtual IGfxAPITexture *createTexture( ETextureFormat fmt, U16 resX, U16 resY, const U8 *pData ) override;
		virtual Void destroyTexture( IGfxAPITexture * ) override;

		virtual IGfxAPIVLayout *createLayout( const SGfxLayout &desc ) override;
		virtual Void destroyLayout( IGfxAPIVLayout * ) override;

		virtual IGfxAPIVBuffer *createVBuffer( UPtr cBytes, const Void *pData, EBufferPerformance, EBufferPurpose ) override;
		virtual IGfxAPIIBuffer *createIBuffer( UPtr cBytes, const Void *pData, EBufferPerformance, EBufferPurpose ) override;
		virtual Void destroyVBuffer( IGfxAPIVBuffer * ) override;
		virtual Void destroyIBuffer( IGfxAPIIBuffer * ) override;

		virtual Void psoSetScissorEnable( Bool enable ) override;
		virtual Void psoSetTextureEnable( Bool enable ) override;
		virtual Void psoSetBlend( EBlendOp, EBlendFactor colA, EBlendFactor colB, EBlendFactor alphaA, EBlendFactor alphaB ) override;

		virtual Void rsSetScissor( S32 posX, S32 posY, U32 resX, U32 resY ) override;
		virtual Void rsSetViewport( S32 posX, S32 posY, U32 resX, U32 resY ) override;

		virtual Void iaSetLayout( IGfxAPIVLayout * ) override;

		virtual Void tsBindTexture( IGfxAPITexture *, U32 uStage ) override;
		virtual Void iaBindVBuffer( IGfxAPIVBuffer * ) override;
		virtual Void iaBindIBuffer( IGfxAPIIBuffer * ) override;

		virtual Void cmdClearRect( S32 posX, S32 posY, U32 resX, U32 resY, U32 value ) override;
		virtual Void cmdUpdateTexture( IGfxAPITexture *, U16 posX, U16 posY, U16 resX, U16 resY, const U8 *pData ) override;
		virtual Void cmdWriteVBuffer( IGfxAPIVBuffer *, UPtr offset, UPtr size, const Void *pData ) override;
		virtual Void cmdWriteIBuffer( IGfxAPIIBuffer *, UPtr offset, UPtr size, const Void *pData ) override;
		virtual Void cmdReadVBuffer( IGfxAPIVBuffer *, UPtr offset, UPtr size, Void *pData ) override;
		virtual Void cmdReadIBuffer( IGfxAPIIBuffer *, UPtr offset, UPtr size, Void *pData ) override;

		virtual Void cmdDraw( ETopology, U32 cVerts, U32 uOffset ) override;
		virtual Void cmdDrawIndexed( ETopology, U32 cIndices, U32 uOffset, U32 uBias ) override;

		static CGfxAPI_D3D11 *init( OSWindow wnd, const SGfxInitDesc &desc )
		{
#ifdef __clang__ // warning: private field 'm_pCtx' is not used
			((void)(((CGfxAPI_D3D11*)0)->m_pCtx));
#endif
			return gfx__api_init_d3d11( wnd, desc );
		}

	private:
		SD3D11Context *const m_pCtx;
	};
}
