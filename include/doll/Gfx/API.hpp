﻿#pragma once

#include "../Core/Defs.hpp"
#include "../Core/Memory.hpp"
#include "../Core/MemoryTags.hpp"

#include "../Math/Matrix.hpp"

#include "../OS/Window.hpp"

#include "Vertex.hpp"

namespace doll
{

	class IGfxAPIProvider;

	class IGfxAPI;

	class IGfxAPITexture;
	class IGfxAPIVBuffer;
	class IGfxAPIIBuffer;
	class IGfxAPIVLayout;
	class IGfxAPISampler;

	enum EGfxAPI
	{
#define DOLL_GFX__API(Name_,BriefName_) kGfxAPI##Name_,
#include "APIs.def.hpp"
#undef DOLL_GFX__API
		kNumGfxAPIs
	};
	enum EGfxScreenMode
	{
		// Standard windowing
		kGfxScreenModeWindowed,
		// Cover the desktop but don't take exclusive control
		kGfxScreenModeFullDesktop,
		// Take full control over the screen
		kGfxScreenModeFullExclusive
	};

	enum EVectorSize
	{
		kVectorSize1 = 1,
		kVectorSize2 = 2,
		kVectorSize3 = 3,
		kVectorSize4 = 4
	};
	enum EVectorType
	{
		kVectorTypeU8,
		kVectorTypeU16,
		kVectorTypeU32,
		kVectorTypeS8,
		kVectorTypeS16,
		kVectorTypeS32,
		kVectorTypeF32,
		kVectorTypeF32_SNorm,
		kVectorTypeF32_UNorm
	};

	enum EBufferPerformance
	{
		kBufferPerfStream,
		kBufferPerfStatic,
		kBufferPerfDynamic
	};
	enum EBufferPurpose
	{
		kBufferPurposeDraw,
		kBufferPurposeRead,
		kBufferPurposeCopy
	};
	enum EAccess
	{
		kAccessNone      = 0x0,
		kAccessRead      = 0x1,
		kAccessWrite     = 0x2,
		kAccessReadWrite = 0x3
	};

	enum ETextureFormat
	{
		kTexFmtRGBA8,
		kTexFmtRGB8
	};

	enum ETextureFilter
	{
		kTexFilterNearest,
		kTexFilterLinear
	};
	enum EMipmapMode
	{
		kMipmapNearest,
		kMipmapLinear
	};

	enum ETextureWrap
	{
		kTexWrapRepeat,
		kTexWrapMirror,
		kTexWrapClamp,
		kTexWrapBorder
	};

	enum ETextureBorder
	{
		kTexBorderTransparentBlack,
		kTexBorderTransparentWhite,
		kTexBorderOpaqueBlack,
		kTexBorderOpaqueWhite
	};

	enum EGfxCompareOp
	{
		kGfxCmpNever,
		kGfxCmpLess,
		kGfxCmpEqual,
		kGfxCmpLessOrEqual,
		kGfxCmpGreater,
		kGfxCmpNotEqual,
		kGfxCmpGreaterOrEqual,
		kGfxCmpAlways
	};

	enum EGfxLayoutElement
	{
		kGfxLayoutElementVertex,
		kGfxLayoutElementNormal,
		kGfxLayoutElementColor,
		kGfxLayoutElementTexCoord
	};
	enum { kMaxLayoutElements = 16UL };

	enum EBlendFactor
	{
		kBlendZero,
		kBlendOne,
		kBlendSrcColor,
		kBlendSrcAlpha,
		kBlendDstColor,
		kBlendDstAlpha,
		kBlendInvSrcColor,
		kBlendInvSrcAlpha,
		kBlendInvDstColor,
		kBlendInvDstAlpha
	};
	enum EBlendOp
	{
		kBlendAdd,
		kBlendSub,
		kBlendRevSub,
		kBlendMin,
		kBlendMax,
		kBlendLogicalClear,
		kBlendLogicalSet,
		kBlendLogicalCopy,
		kBlendLogicalCopyInverted,
		kBlendLogicalNop,
		kBlendLogicalInvert,
		kBlendLogicalAnd,
		kBlendLogicalNand,
		kBlendLogicalOr,
		kBlendLogicalNor,
		kBlendLogicalXor,
		kBlendLogicalEquiv,
		kBlendLogicalAndReverse,
		kBlendLogicalAndInverted,
		kBlendLogicalOrReverse,
		kBlendLogicalOrInverted
	};

	enum EShaderFormat
	{
		kShaderFormatSPIRV,
		kShaderFormatGLSL,
		kShaderFormatHLSL,
		kShaderFormatMSL
	};

	struct SGfxInitDesc
	{
		TArr<IGfxAPIProvider*> apis;
		EGfxScreenMode         windowing;
		S32                    vsync;
	};

	struct SGfxSamplerDesc
	{
		ETextureFilter magFilter;
		ETextureFilter minFilter;
		EMipmapMode    mipmapMode;
		ETextureWrap   wrapU;
		ETextureWrap   wrapV;
		ETextureWrap   wrapW;
		F32            mipLodBias;
		F32            minLod;
		F32            maxLod;
		Bool           anisotropyEnable;
		F32            maxAnisotropy;
		Bool           compareEnable;
		EGfxCompareOp  compareOp;
		ETextureBorder borderColor;
	};

	struct SGfxLayoutElement
	{
		EGfxLayoutElement type;

		EVectorSize       cComps;
		EVectorType       compTy;

		UPtr              uOffset;
		UPtr              cBytes;
	};

	struct SGfxLayout: public TPoolObject< SGfxLayout, kTag_RenderMisc >
	{
		UPtr              stride;
		UPtr              cElements;
		SGfxLayoutElement elements[ kMaxLayoutElements ];

		IGfxAPIVLayout *  pAPIObj;
	};

	class CGfxFrame: public TPoolObject< CGfxFrame, kTag_RenderMisc >
	{
	public:
		CGfxFrame( IGfxAPI &ctx );
		~CGfxFrame();
		
		inline IGfxAPI &getContext() { return m_context; }

		Void setDefaultState();
		Void resize( U32 uResX, U32 uResY );
		Void wsiPresent();

		inline U32 getResX() const { return m_uResX; }
		inline U32 getResY() const { return m_uResY; }

		IGfxAPIVBuffer *getMemVBuf( UPtr cBytes );

		Void setLayout( SGfxLayout * );
		SGfxLayout *getLayout();
		Void finiLayout( SGfxLayout * );
		IGfxAPIVLayout *compileLayout( const SGfxLayout & );

	private:
		IGfxAPI &       m_context;
		U32             m_uResX, m_uResY;
		Mat4f           m_proj2D;
		UPtr            m_memVBuf;
		IGfxAPIVBuffer *m_pMemVBuf;
		UPtr            m_cVBufBytes;

		SGfxLayout *    m_pLayout;
	};

	class IGfxAPI
	{
		IGfxAPIProvider &m_gfxAPIProvider;

	public:
		IGfxAPI( IGfxAPIProvider &provider ): m_gfxAPIProvider( provider ) {}
		virtual ~IGfxAPI() {}

		IGfxAPIProvider &getAPIProvider() {
			return m_gfxAPIProvider;
		}

		virtual EGfxAPI getAPI() const = 0;

		virtual TArr<EShaderFormat> getSupportedShaderFormats() const = 0;

		virtual Void setDefaultState( const Mat4f &proj ) = 0;

		virtual Void resize( U32 uResX, U32 uResY ) = 0;
		virtual Void getSize( U32 &uResX, U32 &uResY ) = 0;

		virtual Void wsiPresent() = 0;

		virtual IGfxAPISampler *createSampler( const SGfxSamplerDesc &desc ) = 0;
		virtual Void destroySampler( IGfxAPISampler * ) = 0;

		virtual IGfxAPITexture *createTexture( ETextureFormat fmt, U16 resX, U16 resY, const U8 *pData ) = 0;
		virtual Void destroyTexture( IGfxAPITexture * ) = 0;

		virtual IGfxAPIVLayout *createLayout( const SGfxLayout &desc ) = 0;
		virtual Void destroyLayout( IGfxAPIVLayout * ) = 0;

		virtual IGfxAPIVBuffer *createVBuffer( UPtr cBytes, const Void *pData, EBufferPerformance, EBufferPurpose ) = 0;
		virtual IGfxAPIIBuffer *createIBuffer( UPtr cBytes, const Void *pData, EBufferPerformance, EBufferPurpose ) = 0;
		virtual Void destroyVBuffer( IGfxAPIVBuffer * ) = 0;
		virtual Void destroyIBuffer( IGfxAPIIBuffer * ) = 0;

		virtual Void vsSetProjectionMatrix( const F32 *matrix ) = 0;
		virtual Void vsSetModelViewMatrix( const F32 *matrix ) = 0;

		virtual Void psoSetScissorEnable( Bool enable ) = 0;
		virtual Void psoSetTextureEnable( Bool enable ) = 0;
		virtual Void psoSetBlend( EBlendOp, EBlendFactor colA, EBlendFactor colB, EBlendFactor alphaA, EBlendFactor alphaB ) = 0;

		virtual Void rsSetScissor( S32 posX, S32 posY, U32 resX, U32 resY ) = 0;
		virtual Void rsSetViewport( S32 posX, S32 posY, U32 resX, U32 resY ) = 0;

		virtual Void iaSetLayout( IGfxAPIVLayout * ) = 0;

		virtual Void tsBindTexture( IGfxAPITexture *, U32 uStage ) = 0;
		virtual Void tsBindSampler( IGfxAPISampler *, U32 uStage ) = 0;
		virtual Void iaBindVBuffer( IGfxAPIVBuffer * ) = 0;
		virtual Void iaBindIBuffer( IGfxAPIIBuffer * ) = 0;

		virtual Void cmdClearRect( S32 posX, S32 posY, U32 resX, U32 resY, U32 value ) = 0;
		virtual Void cmdUpdateTexture( IGfxAPITexture *, U16 posX, U16 posY, U16 resX, U16 resY, const U8 *pData ) = 0;
		virtual Void cmdWriteVBuffer( IGfxAPIVBuffer *, UPtr offset, UPtr size, const Void *pData ) = 0;
		virtual Void cmdWriteIBuffer( IGfxAPIIBuffer *, UPtr offset, UPtr size, const Void *pData ) = 0;
		virtual Void cmdReadVBuffer( IGfxAPIVBuffer *, UPtr offset, UPtr size, Void *pData ) = 0;
		virtual Void cmdReadIBuffer( IGfxAPIIBuffer *, UPtr offset, UPtr size, Void *pData ) = 0;

		virtual Void cmdDraw( ETopology, U32 cVerts, U32 uOffset ) = 0;
		virtual Void cmdDrawIndexed( ETopology, U32 cIndices, U32 uOffset, U32 uBias ) = 0;
	};

	class IGfxAPIProvider {
	public:
		virtual Void drop() = 0;

		virtual Bool is( const Str &name ) const = 0;
		virtual Str getName() const = 0;
		virtual Str getDescription() const = 0;
		virtual IGfxAPI *initAPI( OSWindow wnd, const SGfxInitDesc &desc ) = 0;
		virtual Void finiAPI( IGfxAPI *pAPI ) = 0;
	};

	inline UPtr gfx_r_calcSize( EVectorSize n, EVectorType t )
	{
		UPtr baseSize = 0;

		switch( t )
		{
		case kVectorTypeU8:
		case kVectorTypeS8:
			baseSize = 1;
			break;

		case kVectorTypeU16:
		case kVectorTypeS16:
			baseSize = 2;
			break;

		case kVectorTypeU32:
		case kVectorTypeS32:
		case kVectorTypeF32:
		case kVectorTypeF32_SNorm:
		case kVectorTypeF32_UNorm:
			baseSize = 4;
			break;
		}

		return baseSize*UPtr(n);
	}

	enum class EShouldOverrideExistingAPI {
		no,
		yes,

		dontCare = yes
	};

	DOLL_FUNC Bool DOLL_API doll_registerGfxAPI( IGfxAPIProvider &provider, EShouldOverrideExistingAPI = EShouldOverrideExistingAPI::dontCare );
	DOLL_FUNC Void DOLL_API doll_unregisterGfxAPI( IGfxAPIProvider &provider );
	DOLL_FUNC IGfxAPIProvider *DOLL_API doll_findGfxAPIByName( const Str &name );
	DOLL_FUNC SizeType DOLL_API doll_getGfxAPICount();
	DOLL_FUNC IGfxAPIProvider *DOLL_API doll_getGfxAPI( SizeType index );

	DOLL_FUNC IGfxAPI *DOLL_API gfx_initAPI( OSWindow wnd, const SGfxInitDesc *pDesc = nullptr );
	DOLL_FUNC NullPtr DOLL_API gfx_finiAPI( IGfxAPI * );

	DOLL_FUNC Void DOLL_API gfx_r_setFrame( CGfxFrame *pFrame );
	DOLL_FUNC CGfxFrame *DOLL_API gfx_r_getFrame();

	DOLL_FUNC U32 DOLL_API gfx_r_resX();
	DOLL_FUNC U32 DOLL_API gfx_r_resY();

	DOLL_FUNC Void DOLL_API gfx_r_loadProjection( const F32 *matrix );
	DOLL_FUNC Void DOLL_API gfx_r_loadModelView( const F32 *matrix );

	DOLL_FUNC Void DOLL_API gfx_r_enableScissor();
	DOLL_FUNC Void DOLL_API gfx_r_disableScissor();
	DOLL_FUNC Void DOLL_API gfx_r_setScissor( S32 posX, S32 posY, U32 resX, U32 resY );
	DOLL_FUNC Void DOLL_API gfx_r_clearRect( S32 posX, S32 posY, U32 resX, U32 resY, U32 value );

	DOLL_FUNC Void DOLL_API gfx_r_setViewport( S32 posX, S32 posY, U32 resX, U32 resY );

	DOLL_FUNC UPtr DOLL_API gfx_r_createTexture( ETextureFormat fmt, U16 resX, U16 resY, const U8 *data );
	DOLL_FUNC Void DOLL_API gfx_r_destroyTexture( UPtr tex );

	DOLL_FUNC Void DOLL_API gfx_r_updateTexture( UPtr tex, U16 posX, U16 posY, U16 resX, U16 resY, const U8 *data );

	DOLL_FUNC Void DOLL_API gfx_r_setBlend( EBlendOp, EBlendFactor srgb, EBlendFactor drgb, EBlendFactor sa, EBlendFactor da );

	DOLL_FUNC Void DOLL_API gfx_r_enableTexture2D();
	DOLL_FUNC Void DOLL_API gfx_r_disableTexture2D();

	DOLL_FUNC Void DOLL_API gfx_r_setTexture( UPtr tex, U32 stage = 0 );

	DOLL_FUNC UPtr DOLL_API gfx_r_createLayout( UPtr stride = 0 );
	DOLL_FUNC Void DOLL_API gfx_r_destroyLayout( UPtr layout );

	DOLL_FUNC Void DOLL_API gfx_r_layoutVertex( UPtr layout, EVectorSize size, EVectorType type = kVectorTypeF32, UPtr offset = ~0U );
	DOLL_FUNC Void DOLL_API gfx_r_layoutNormal( UPtr layout, EVectorType type = kVectorTypeF32, UPtr offset = ~0U );
	DOLL_FUNC Void DOLL_API gfx_r_layoutColor( UPtr layout, EVectorSize size = kVectorSize4, EVectorType type = kVectorTypeU8, UPtr offset = ~0U );
	DOLL_FUNC Void DOLL_API gfx_r_layoutTexCoord( UPtr layout, EVectorSize size = kVectorSize2, EVectorType type = kVectorTypeF32, UPtr offset = ~0U );
	DOLL_FUNC Bool DOLL_API gfx_r_finishLayout( UPtr layout );

	DOLL_FUNC Void DOLL_API gfx_r_setLayout( UPtr layout );
	DOLL_FUNC UPtr DOLL_API gfx_r_getLayout();


	DOLL_FUNC UPtr DOLL_API gfx_r_createVBuffer( UPtr size, const void *pData, EBufferPerformance perf, EBufferPurpose purpose );
	DOLL_FUNC UPtr DOLL_API gfx_r_createIBuffer( UPtr size, const void *pData, EBufferPerformance perf, EBufferPurpose purpose );

	DOLL_FUNC Bool DOLL_API gfx_r_writeVBuffer( UPtr vbuffer, UPtr offset, UPtr size, const void *pData );
	DOLL_FUNC Bool DOLL_API gfx_r_writeIBuffer( UPtr ibuffer, UPtr offset, UPtr size, const void *pData );

	DOLL_FUNC Bool DOLL_API gfx_r_readVBuffer( UPtr vbuffer, UPtr offset, UPtr size, void *pData );
	DOLL_FUNC Bool DOLL_API gfx_r_readIBuffer( UPtr ibuffer, UPtr offset, UPtr size, void *pData );

	DOLL_FUNC Void DOLL_API gfx_r_destroyVBuffer( UPtr vbuffer );
	DOLL_FUNC Void DOLL_API gfx_r_destroyIBuffer( UPtr ibuffer );

	DOLL_FUNC Void DOLL_API gfx_r_setVBuffer( UPtr vbuffer );
	DOLL_FUNC Void DOLL_API gfx_r_setIBuffer( UPtr ibuffer );

	DOLL_FUNC Void DOLL_API gfx_r_draw( ETopology mode, U32 cVerts, U32 uOffset = 0 );
	DOLL_FUNC Void DOLL_API gfx_r_drawIndexed( ETopology mode, U32 cIndices, U32 uOffset = 0, U32 uBias = 0 );
	DOLL_FUNC Void DOLL_API gfx_r_drawMem( ETopology mode, U32 cVerts, UPtr cStrideBytes, const void *pMem );

}
