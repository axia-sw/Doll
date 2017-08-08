# Graphics Commands

Files:

- [Action.hpp](../include/doll/Gfx/Action.hpp)
- [API.hpp](../include/doll/Gfx/API.hpp)
- [APIs.def.hpp](../include/doll/Gfx/APIs.def.hpp)
- [Layer.hpp](../include/doll/Gfx/Layer.hpp)
- [LayerEffect.hpp](../include/doll/Gfx/LayerEffect.hpp)
- [OSText.hpp](../include/doll/Gfx/OSText.hpp)
- [RenderCommands.hpp](../include/doll/Gfx/RenderCommands.hpp)
- [Sprite.hpp](../include/doll/Gfx/Sprite.hpp)
- [Texture.hpp](../include/doll/Gfx/Texture.hpp)
- [Vertex.hpp](../include/doll/Gfx/Vertex.hpp)


## Action

Actions run on sprites for a specified duration of time. They are called
automatically by the sprite for updates.

```cpp
class CallbackAction: public Action
{
public:
	typedef Void( DOLL_API *FnStart )( Void *pCallbackData, RSprite *pSprite );
	typedef Void( DOLL_API *FnComplete )( Void *pCallbackData, RSprite *pSprite );
	typedef Void( DOLL_API *FnUpdate )( Void *pCallbackData, RSprite *pSprite, F64 fProgress, F64 fDelta );

	// ... snip ... //
};

DOLL_FUNC CallbackAction *DOLL_API gfx_newCallbackAction();
DOLL_FUNC Action *DOLL_API gfx_deleteAction( Action *pAction );
DOLL_FUNC Void DOLL_API gfx_setActionCounter( Action *pAction, CFrameCounter *pCounter );
DOLL_FUNC CFrameCounter *DOLL_API gfx_getActionCounter( const Action *pAction );
DOLL_FUNC F64 DOLL_API gfx_getActionDuration( const Action *pAction );
DOLL_FUNC Void DOLL_API gfx_setActionDuration( Action *pAction, F64 fDuration );
DOLL_FUNC Void DOLL_API gfx_setCallbackActionStartFunction( CallbackAction *pCBAction, CallbackAction::FnStart pfnStart );
DOLL_FUNC Void DOLL_API gfx_setCallbackActionCompleteFunction( CallbackAction *pCBAction, CallbackAction::FnComplete pfnComplete );
DOLL_FUNC Void DOLL_API gfx_setCallbackActionFunction( CallbackAction *pCBAction, CallbackAction::FnUpdate pfnUpdate );
DOLL_FUNC Void DOLL_API gfx_setCallbackActionData( CallbackAction *pCBAction, Void *pCallbackData );
DOLL_FUNC CallbackAction::FnStart DOLL_API gfx_getCallbackActionStartFunction( const CallbackAction *pCBAction );
DOLL_FUNC CallbackAction::FnComplete DOLL_API gfx_getCallbackActionCompleteFunction( const CallbackAction *pCBAction );
DOLL_FUNC CallbackAction::FnUpdate DOLL_API gfx_getCallbackActionFunction( const CallbackAction *pCBAction );
DOLL_FUNC Void *DOLL_API gfx_getCallbackActionData( const CallbackAction *pCBAction );
```

## API

Low-level graphics API abstraction interface.

In general, you shouldn't be using this API except in instances where a
higher-level API does not fit, or you're implementing a low level renderer for
Doll to funnel its commands through.

```cpp
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

/*

	... snip ...

	`class CGfxFrame` here.

	... snip ...

*/

class IGfxAPI
{
	IGfxAPIProvider &m_gfxAPIProvider;

public:
	IGfxAPI( IGfxAPIProvider &provider ): m_gfxAPIProvider( provider ) {}
	virtual ~IGfxAPI() {}

	IGfxAPIProvider &getAPIProvider();

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

inline UPtr gfx_r_calcSize( EVectorSize n, EVectorType t );

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
```

## Layer

Layers are similar to viewports, except they can be arranged in a hierarchy.
They can be used to logically group graphics operations, as you would in an
image editing program for example, and are well suited for building your own
user interfaces.

```cpp
enum ELayout : U32
{
	kLayoutF_AlignX1  = 1,
	kLayoutF_AlignY1  = 2,
	kLayoutF_AlignX2  = 4,
	kLayoutF_AlignY2  = 8,
	kLayoutF_KeepResX = 16,
	kLayoutF_KeepResY = 32,
	
	kLayoutF_AlignX   = kLayoutF_AlignX1  | kLayoutF_AlignX2,
	kLayoutF_AlignY   = kLayoutF_AlignY1  | kLayoutF_AlignY2,
	kLayoutF_Align    = kLayoutF_AlignX   | kLayoutF_AlignY,
	kLayoutF_KeepRes  = kLayoutF_KeepResX | kLayoutF_KeepResY
};

DOLL_FUNC RLayer *DOLL_API gfx_newLayer();
DOLL_FUNC RLayer *DOLL_API gfx_deleteLayer( RLayer *layer );

DOLL_FUNC RLayer *DOLL_API gfx_getDefaultLayer();

DOLL_FUNC RLayer *DOLL_API gfx_getFirstLayer();
DOLL_FUNC RLayer *DOLL_API gfx_getLastLayer();
DOLL_FUNC RLayer *DOLL_API gfx_getLayerBefore( RLayer *layer );
DOLL_FUNC RLayer *DOLL_API gfx_getLayerAfter( RLayer *layer );
DOLL_FUNC Void DOLL_API gfx_moveLayerUp( RLayer *layer );
DOLL_FUNC Void DOLL_API gfx_moveLayerDown( RLayer *layer );

DOLL_FUNC Void DOLL_API gfx_addLayerPrerenderSpriteGroup( RLayer *layer, RSpriteGroup *group );
DOLL_FUNC Void DOLL_API gfx_addLayerPostrenderSpriteGroup( RLayer *layer, RSpriteGroup *group );

DOLL_FUNC Void DOLL_API gfx_removeLayerPrerenderSpriteGroup( RLayer *layer, RSpriteGroup *group );
DOLL_FUNC Void DOLL_API gfx_removeLayerPostrenderSpriteGroup( RLayer *layer, RSpriteGroup *group );
DOLL_FUNC Void DOLL_API gfx_removeAllLayerPrerenderSpriteGroups( RLayer *layer );
DOLL_FUNC Void DOLL_API gfx_removeAllLayerPostrenderSpriteGroups( RLayer *layer );

DOLL_FUNC UPtr DOLL_API gfx_getLayerPrerenderSpriteGroupCount( const RLayer *layer );
DOLL_FUNC UPtr DOLL_API gfx_getLayerPostrenderSpriteGroupCount( const RLayer *layer );
DOLL_FUNC RSpriteGroup *DOLL_API gfx_getLayerPrerenderSpriteGroup( RLayer *layer, UPtr index );
DOLL_FUNC RSpriteGroup *DOLL_API gfx_getLayerPostrenderSpriteGroup( RLayer *layer, UPtr index );

DOLL_FUNC Bool DOLL_API gfx_isLayerDescendantOf( const RLayer *layer, const RLayer *ancestor );
DOLL_FUNC Void DOLL_API gfx_setLayerParent( RLayer *layer, RLayer *parent );
DOLL_FUNC RLayer *DOLL_API gfx_getLayerParent( RLayer *layer );

DOLL_FUNC Void DOLL_API gfx_setLayerVisible( RLayer *layer, Bool visible );
DOLL_FUNC Bool DOLL_API gfx_getLayerVisible( const RLayer *layer );
DOLL_FUNC Void DOLL_API gfx_setLayerLayout( RLayer *layer, U32 layoutFlags );
DOLL_FUNC U32 DOLL_API gfx_getLayerLayout( const RLayer *layer );
DOLL_FUNC Void DOLL_API gfx_setLayerLayoutDistance( RLayer *layer, S32 left, S32 top, S32 right, S32 bottom );
DOLL_FUNC Void DOLL_API gfx_setLayerLayoutDistanceRect( RLayer *layer, const SRect &dist );
DOLL_FUNC Bool DOLL_API gfx_getLayerLayoutDistance( SRect &dst, const RLayer *layer );
inline SRect DOLL_API gfx_getLayerLayoutDistance( const RLayer *layer );

DOLL_FUNC Void DOLL_API gfx_setLayerPosition( RLayer *layer, S32 x, S32 y );
DOLL_FUNC Void DOLL_API gfx_setLayerPositionVec( RLayer *layer, const SIntVector2 &pos );
DOLL_FUNC Bool DOLL_API gfx_getLayerPosition( SIntVector2 &dst, const RLayer *layer );
inline SIntVector2 DOLL_API gfx_getLayerPosition( const RLayer *layer );
DOLL_FUNC S32 DOLL_API gfx_getLayerPositionX( const RLayer *layer );
DOLL_FUNC S32 DOLL_API gfx_getLayerPositionY( const RLayer *layer );
DOLL_FUNC Void DOLL_API gfx_setLayerSize( RLayer *layer, S32 w, S32 h );
DOLL_FUNC Void DOLL_API gfx_setLayerSizeVec( RLayer *layer, const SIntVector2 &res );
DOLL_FUNC Bool DOLL_API gfx_getLayerSize( SIntVector2 &dst, const RLayer *layer );
inline SIntVector2 DOLL_API gfx_getLayerSize( const RLayer *layer );
DOLL_FUNC S32 DOLL_API gfx_getLayerSizeX( const RLayer *layer );
DOLL_FUNC S32 DOLL_API gfx_getLayerSizeY( const RLayer *layer );
DOLL_FUNC Bool DOLL_API gfx_layerClientToScreen( SIntVector2 &dst, RLayer *layer, const SIntVector2 &local );
DOLL_FUNC Bool DOLL_API gfx_layerScreenToClient( SIntVector2 &dst, RLayer *layer, const SIntVector2 &global );
inline SIntVector2 DOLL_API gfx_layerClientToScreen( RLayer *layer, const SIntVector2 &local );
inline SIntVector2 DOLL_API gfx_layerScreenToClient( RLayer *layer, const SIntVector2 &global );
DOLL_FUNC Bool DOLL_API gfx_getLayerShape( SRect &dst, const RLayer *layer );
DOLL_FUNC Bool DOLL_API gfx_getLayerScreenShape( SRect &dst, const RLayer *layer );
DOLL_FUNC Bool DOLL_API gfx_getLayerParentScreenShape( SRect &dst, const RLayer *layer );
inline SRect DOLL_API gfx_getLayerShape( const RLayer *layer );
inline SRect DOLL_API gfx_getLayerScreenShape( const RLayer *layer );
inline SRect DOLL_API gfx_getLayerParentScreenShape( const RLayer *layer );
DOLL_FUNC Void DOLL_API gfx_reflowLayer( RLayer *layer );

DOLL_FUNC RLayer *DOLL_API gfx_getFirstLayerChild( RLayer *layer );
DOLL_FUNC RLayer *DOLL_API gfx_getLastLayerChild( RLayer *layer );

DOLL_FUNC Void DOLL_API gfx_setLayerUserPointer( RLayer *layer, Void *userp );
DOLL_FUNC Void *DOLL_API gfx_getLayerUserPointer( RLayer *layer );

DOLL_FUNC Void DOLL_API gfx_enableLayerAutoclear( RLayer *layer );
DOLL_FUNC Void DOLL_API gfx_disableLayerAutoclear( RLayer *layer );
DOLL_FUNC Void DOLL_API gfx_toggleLayerAutoclear( RLayer *layer );
DOLL_FUNC Bool DOLL_API gfx_isLayerAutoclearEnabled( const RLayer *layer );

DOLL_FUNC Void DOLL_API gfx_moveLayerTop( RLayer *layer );
DOLL_FUNC Void DOLL_API gfx_moveLayerBottom( RLayer *layer );

DOLL_FUNC Void DOLL_API gfx_setLayerFrame( RLayer *layer, CGfxFrame *pFrame );
DOLL_FUNC CGfxFrame *DOLL_API gfx_getLayerFrame( RLayer *layer );

DOLL_FUNC Void DOLL_API gfx_setLayerAspect( RLayer *pLayer, F32 fRatio, EAspect mode );
DOLL_FUNC F32 DOLL_API gfx_getLayerAspectRatio( const RLayer *pLayer );
DOLL_FUNC EAspect DOLL_API gfx_getLayerAspectMode( const RLayer *pLayer );
```

## Layer Effect

Layer effects can be used to apply effects to layers each time a layer is
rendered. This can be useful for screen quaking effects and the like.

```cpp
class ILayerEffect
{
public:
	ILayerEffect() {}
	virtual ~ILayerEffect() {}

	virtual void run( RLayer &layer ) = 0;
};

DOLL_FUNC Void DOLL_API gfx_addLayerEffect( RLayer *pLayer, ILayerEffect *pLayerEffect );
DOLL_FUNC Void DOLL_API gfx_addLayerEffectToFront( RLayer *pLayer, ILayerEffect *pLayerEffect );
DOLL_FUNC Void DOLL_API gfx_removeLayerEffect( RLayer *pLayer, ILayerEffect *pLayerEffect );
DOLL_FUNC Void DOLL_API gfx_removeAllLayerEffectInstances( RLayer *pLayer, ILayerEffect *pLayerEffect );
DOLL_FUNC Void DOLL_API gfx_removeAllLayerEffects( RLayer *pLayer );
```

## OS Text

Subsystem for using the operating system's native text rendering functionality
to draw text to the screen.

On Windows this is done with GDIplus; macOS: Cocoa. There is no implementation
for other operating systems at the moment.

```cpp
#define DOLL_OSTEXT_LINE_COLOR 0xFF000000
#define DOLL_OSTEXT_FILL_COLOR 0xFFFFFFFF

struct STextItem;

DOLL_FUNC STextItem *DOLL_API gfx_newOSText( Str text, const SIntVector2 &size, U32 lineColor = DOLL_OSTEXT_LINE_COLOR, U32 fillColor = DOLL_OSTEXT_FILL_COLOR );
DOLL_FUNC STextItem *DOLL_API gfx_deleteOSText( STextItem *pText );
DOLL_FUNC Void DOLL_API gfx_ostext_fillCache( STextItem *pText );
DOLL_FUNC U32 DOLL_API gfx_ostext_resX( const STextItem *pText );
DOLL_FUNC U32 DOLL_API gfx_ostext_resY( const STextItem *pText );
DOLL_FUNC const Void *DOLL_API gfx_ostext_getBits( const STextItem *pText );
DOLL_FUNC RTexture *DOLL_API gfx_ostext_makeTexture( const STextItem *pText, CTextureAtlas *pDefAtlas = nullptr );

DOLL_FUNC RTexture *DOLL_API gfx_renderOSText( Str text, const SIntVector2 &size, U32 lineColor = DOLL_OSTEXT_LINE_COLOR, U32 fillColor = DOLL_OSTEXT_FILL_COLOR, CTextureAtlas *pDefAtlas = nullptr );
DOLL_FUNC void DOLL_API gfx_drawOSText( Str text, const SRect &area );
```

## Render Commands

Easy-to-use basic 2D rendering commands.

```cpp
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

DOLL_FUNC EResult DOLL_API gfx_ink( U32 color );

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

DOLL_FUNC EResult DOLL_API gfx_blitImage( const RTexture *img, S32 x, S32 y );
DOLL_FUNC EResult DOLL_API gfx_stretchImage( const RTexture *img, S32 x, S32 y, S32 w, S32 h );
DOLL_FUNC EResult DOLL_API gfx_blitSubimage( const RTexture *img, S32 dstX, S32 dstY, S32 srcX, S32 srcY, S32 w, S32 h );
DOLL_FUNC EResult DOLL_API gfx_stretchSubimage( const RTexture *img, S32 dstX, S32 dstY, S32 dstW, S32 dstH, S32 srcX, S32 srcY, S32 srcW, S32 srcH );

DOLL_FUNC EResult DOLL_API gfx_blitImageColored( const RTexture *img, S32 x, S32 y, U32 tlcol, U32 trcol, U32 blcol, U32 brcol );

DOLL_FUNC EResult DOLL_API gfx_light( S32 l, S32 t, S32 r, S32 b, float levelSnorm );

inline EResult DOLL_API gfx_dot( const SIntVector2 &pos );
inline EResult DOLL_API gfx_line( const SIntVector2 &s, const SIntVector2 &e );
inline EResult DOLL_API gfx_outline( const SRect &r );
inline EResult DOLL_API gfx_box( const SRect &r );
inline EResult DOLL_API gfx_hgradBox( const SRect &r, U32 lcolor, U32 rcolor );
inline EResult DOLL_API gfx_vgradBox( const SRect &r, U32 tcolor, U32 bcolor );
inline EResult DOLL_API gfx_gradBox( const SRect &r, U32 tlcolor, U32 trcolor, U32 blcolor, U32 brcolor );
inline EResult DOLL_API gfx_ellipse( const SIntVector2 &pos, const SIntVector2 &radius );
inline EResult DOLL_API gfx_circle( const SIntVector2 &pos, S32 radius );
inline EResult DOLL_API gfx_roundedBox( const SRect &r, S32 radius );
inline EResult DOLL_API gfx_blitImage( const RTexture *img, const SIntVector2 &pos );
inline EResult DOLL_API gfx_stretchImage( const RTexture *img, const SRect &r );
inline EResult DOLL_API gfx_blitSubimage( const RTexture *img, const SIntVector2 &dstPos, const SRect &srcBox );
inline EResult DOLL_API gfx_stretchSubimage( const RTexture *img, const SRect &dstBox, const SRect &srcBox );
inline EResult DOLL_API gfx_blitImageColored( const RTexture *img, const SIntVector2 &pos, U32 tlcol, U32 trcol, U32 blcol, U32 brcol );

inline EResult DOLL_API gfx_light( const SRect &box, float levelSnorm );
```

## Sprite

Sprites are images that are managed and get rendered to the screen
automatically. They can be animated, and can be separated into various "sprite
groups." These groupings can even have different virtual resolutions.

Additionally, sprites can be put into a hierarchy in which sub-sprites can
inherit the transformations (such as position, angle, or scale, or any
combination thereof) from another sprite.

```cpp
DOLL_FUNC RSprite *DOLL_API gfx_newSprite();
DOLL_FUNC RSprite *DOLL_API gfx_newSpriteInGroup( RSpriteGroup *group );
DOLL_FUNC RSprite *DOLL_API gfx_deleteSprite( RSprite *spr );
DOLL_FUNC RSprite *DOLL_API gfx_loadAnimSprite( const RTexture *img, S32 cellResX, S32 cellResY, S32 startFrame, S32 numFrames, S32 offX, S32 offY, S32 padX, S32 padY );
DOLL_FUNC RSprite *DOLL_API gfx_loadAnimSpriteInGroup( RSpriteGroup *group, const RTexture *texture, S32 cellResX, S32 cellResY, S32 startFrame, S32 numFrames, S32 offX, S32 offY, S32 padX, S32 padY );

DOLL_FUNC RSpriteGroup *DOLL_API gfx_newSpriteGroup();
DOLL_FUNC RSpriteGroup *DOLL_API gfx_deleteSpriteGroup( RSpriteGroup *group );
DOLL_FUNC RSpriteGroup *DOLL_API gfx_getDefaultSpriteGroup();
DOLL_FUNC Void DOLL_API gfx_showSpriteGroup( RSpriteGroup *group );
DOLL_FUNC Void DOLL_API gfx_hideSpriteGroup( RSpriteGroup *group );
DOLL_FUNC Bool DOLL_API gfx_isSpriteGroupVisible( const RSpriteGroup *group );
DOLL_FUNC Void DOLL_API gfx_enableSpriteGroupScissor( RSpriteGroup *group, S32 posX, S32 posY, S32 resX, S32 resY );
DOLL_FUNC Void DOLL_API gfx_disableSpriteGroupScissor( RSpriteGroup *group );
DOLL_FUNC Bool DOLL_API gfx_isSpriteGroupScissorEnabled( const RSpriteGroup *group );
DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupScissorPositionX( const RSpriteGroup *group );
DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupScissorPositionY( const RSpriteGroup *group );
DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupScissorResolutionX( const RSpriteGroup *group );
DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupScissorResolutionY( const RSpriteGroup *group );

DOLL_FUNC Void DOLL_API gfx_moveSprite( RSprite *sprite, F32 x, F32 y );
DOLL_FUNC Void DOLL_API gfx_turnSprite( RSprite *sprite, F32 theta );
DOLL_FUNC Void DOLL_API gfx_setSpritePosition( RSprite *sprite, F32 x, F32 y );
DOLL_FUNC Void DOLL_API gfx_setSpriteRotation( RSprite *sprite, F32 angle );
DOLL_FUNC Void DOLL_API gfx_setSpriteScale( RSprite *sprite, F32 x, F32 y );
DOLL_FUNC Void DOLL_API gfx_moveSpriteFrame( RSprite *sprite, F32 x, F32 y );
DOLL_FUNC Void DOLL_API gfx_turnSpriteFrame( RSprite *sprite, F32 theta );
DOLL_FUNC Void DOLL_API gfx_setSpriteFramePosition( RSprite *sprite, F32 x, F32 y );
DOLL_FUNC Void DOLL_API gfx_setSpriteFrameRotation( RSprite *sprite, F32 angle );
DOLL_FUNC Void DOLL_API gfx_setSpriteFrameScale( RSprite *sprite, F32 x, F32 y );
DOLL_FUNC Void DOLL_API gfx_setSpriteFrameSourceRectangle( RSprite *sprite, S32 posX, S32 posY, S32 resX, S32 resY );
DOLL_FUNC Void DOLL_API gfx_setSpriteFrameCornerColors( RSprite *sprite, U32 colorTL, U32 colorTR, U32 colorBL, U32 colorBR );
DOLL_FUNC Void DOLL_API gfx_setSpriteFrameCornerAlphas( RSprite *sprite, U32 alphaTL, U32 alphaTR, U32 alphaBL, U32 alphaBR );
DOLL_FUNC Void DOLL_API gfx_setSpriteFrameColor( RSprite *sprite, U32 color );
DOLL_FUNC Void DOLL_API gfx_setSpriteFrameAlpha( RSprite *sprite, U32 alpha );
DOLL_FUNC Void DOLL_API gfx_deleteAllSpriteFrames( RSprite *sprite );

DOLL_FUNC S32 DOLL_API gfx_addSpriteFrame( RSprite *sprite );
DOLL_FUNC SSpriteFrame *DOLL_API gfx_duplicateCurrentSpriteFrame( RSprite *sprite );

DOLL_FUNC Bool DOLL_API gfx_getSpritePosition( const RSprite *sprite, F32 *x, F32 *y );
DOLL_FUNC F32 DOLL_API gfx_getSpritePositionX( const RSprite *sprite );
DOLL_FUNC F32 DOLL_API gfx_getSpritePositionY( const RSprite *sprite );
DOLL_FUNC F32 DOLL_API gfx_getSpriteRotation( const RSprite *sprite );
DOLL_FUNC Bool DOLL_API gfx_getSpriteScale( const RSprite *sprite, F32 *x, F32 *y );
DOLL_FUNC F32 DOLL_API gfx_getSpriteScaleX( const RSprite *sprite );
DOLL_FUNC F32 DOLL_API gfx_getSpriteScaleY( const RSprite *sprite );
DOLL_FUNC Bool DOLL_API gfx_getSpriteFramePosition( const RSprite *sprite, F32 *x, F32 *y );
DOLL_FUNC F32 DOLL_API gfx_getSpriteFramePositionX( const RSprite *sprite );
DOLL_FUNC F32 DOLL_API gfx_getSpriteFramePositionY( const RSprite *sprite );
DOLL_FUNC F32 DOLL_API gfx_getSpriteFrameRotation( const RSprite *sprite );
DOLL_FUNC Bool DOLL_API gfx_getSpriteFrameScale( const RSprite *sprite, F32 *x, F32 *y );
DOLL_FUNC F32 DOLL_API gfx_getSpriteFrameScaleX( const RSprite *sprite );
DOLL_FUNC F32 DOLL_API gfx_getSpriteFrameScaleY( const RSprite *sprite );
DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameCornerColor( const RSprite *sprite, U32 index );
DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameCornerAlpha( const RSprite *sprite, U32 index );
DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameColor( const RSprite *sprite );
DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameAlpha( const RSprite *sprite );

DOLL_FUNC Void DOLL_API gfx_setSpriteTexture( RSprite *sprite, const RTexture *texture );
DOLL_FUNC const RTexture *DOLL_API gfx_getSpriteTexture( const RSprite *sprite );

DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameCount( const RSprite *sprite );
DOLL_FUNC Void DOLL_API gfx_setSpriteCurrentFrame( RSprite *sprite, U32 frameIndex );
DOLL_FUNC U32 DOLL_API gfx_getSpriteCurrentFrame( const RSprite *sprite );

DOLL_FUNC Void DOLL_API gfx_setSpriteFramesPerSecond( RSprite *sprite, F64 fps );
DOLL_FUNC F64 DOLL_API gfx_getSpriteFramesPerSecond( const RSprite *sprite );

DOLL_FUNC Void DOLL_API gfx_playSpriteAnimation( RSprite *sprite, U32 beginFrame, U32 endFrame );
DOLL_FUNC Void DOLL_API gfx_loopSpriteAnimation( RSprite *sprite, U32 beginFrame, U32 endFrame );
DOLL_FUNC Void DOLL_API gfx_playSpriteAnimationWithDeath( RSprite *sprite, U32 beginFrame, U32 endFrame );
DOLL_FUNC Void DOLL_API gfx_stopSpriteAnimation( RSprite *sprite );
DOLL_FUNC Bool DOLL_API gfx_isSpriteAnimationPlaying( const RSprite *sprite );
DOLL_FUNC Bool DOLL_API gfx_isSpriteAnimationLooping( const RSprite *sprite );

DOLL_FUNC Void DOLL_API gfx_setSpriteGroup( RSprite *sprite, RSpriteGroup *group );
DOLL_FUNC RSpriteGroup *DOLL_API gfx_getSpriteGroup( RSprite *sprite );

DOLL_FUNC Void DOLL_API gfx_bindSprite( RSprite *sprite, RSprite *master );
DOLL_FUNC Void DOLL_API gfx_bindSpriteTranslation( RSprite *sprite, RSprite *master );
DOLL_FUNC Void DOLL_API gfx_selectivelyBindSprite( RSprite *sprite, RSprite *master, S32 bindTranslation, S32 bindRotation, S32 bindScale );
DOLL_FUNC Void DOLL_API gfx_unbindSprite( RSprite *sprite );
DOLL_FUNC RSprite *DOLL_API gfx_getSpriteBindMaster( RSprite *sprite );
DOLL_FUNC Bool DOLL_API gfx_isSpriteBoundToTranslation( const RSprite *sprite );
DOLL_FUNC Bool DOLL_API gfx_isSpriteBoundToRotation( const RSprite *sprite );
DOLL_FUNC Bool DOLL_API gfx_isSpriteBoundToScale( const RSprite *sprite );

DOLL_FUNC Void DOLL_API gfx_setSpriteOffset( RSprite *sprite, F32 x, F32 y );
DOLL_FUNC Bool DOLL_API gfx_getSpriteOffset( const RSprite *sprite, F32 *x, F32 *y );
DOLL_FUNC F32 DOLL_API gfx_getSpriteOffsetX( const RSprite *sprite );
DOLL_FUNC F32 DOLL_API gfx_getSpriteOffsetY( const RSprite *sprite );

DOLL_FUNC Void DOLL_API gfx_enableSpriteGroupAutoupdating( RSpriteGroup *group );
DOLL_FUNC Void DOLL_API gfx_disableSpriteGroupAutoupdating( RSpriteGroup *group );
DOLL_FUNC Bool DOLL_API gfx_isSpriteGroupAutoupdatingEnabled( const RSpriteGroup *group );

DOLL_FUNC Void DOLL_API gfx_enableSpriteAutoupdating( RSprite *sprite );
DOLL_FUNC Void DOLL_API gfx_disableSpriteAutoupdating( RSprite *sprite );
DOLL_FUNC Bool DOLL_API gfx_isSpriteAutoupdatingEnabled( const RSprite *sprite );

DOLL_FUNC Void DOLL_API gfx_setSpriteGroupPosition( RSpriteGroup *group, F32 x, F32 y );
DOLL_FUNC Bool DOLL_API gfx_getSpriteGroupPosition( const RSpriteGroup *group, F32 *x, F32 *y );
DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupPositionX( const RSpriteGroup *group );
DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupPositionY( const RSpriteGroup *group );

DOLL_FUNC Void DOLL_API gfx_setSpriteGroupRotation( RSpriteGroup *group, F32 rotation );
DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupRotation( const RSpriteGroup *group );

DOLL_FUNC Void DOLL_API gfx_enableSpriteGroupFollow( RSpriteGroup *group, const RSprite *follow );
DOLL_FUNC Void DOLL_API gfx_disableSpriteGroupFollow( RSpriteGroup *group );
DOLL_FUNC Bool DOLL_API gfx_isSpriteGroupFollowEnabled( const RSpriteGroup *group );

DOLL_FUNC Void DOLL_API gfx_setSpriteGroupFollowMinimumDistance( RSpriteGroup *group, F32 x, F32 y );
DOLL_FUNC Void DOLL_API gfx_setSpriteGroupFollowMaximumDistance( RSpriteGroup *group, F32 x, F32 y );
DOLL_FUNC Bool DOLL_API gfx_getSpriteGroupFollowMinimumDistance( const RSpriteGroup *group, F32 *x, F32 *y );
DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowMinimumDistanceX( const RSpriteGroup *group );
DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowMinimumDistanceY( const RSpriteGroup *group );
DOLL_FUNC Bool DOLL_API gfx_getSpriteGroupFollowMaximumDistance( const RSpriteGroup *group, F32 *x, F32 *y );
DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowMaximumDistanceX( const RSpriteGroup *group );
DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowMaximumDistanceY( const RSpriteGroup *group );

DOLL_FUNC Void DOLL_API gfx_setSpriteGroupFollowSpeed( RSpriteGroup *group, F32 x, F32 y );
DOLL_FUNC Bool DOLL_API gfx_getSpriteGroupFollowSpeed( const RSpriteGroup *group, F32 *x, F32 *y );
DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowSpeedX( const RSpriteGroup *group );
DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowSpeedY( const RSpriteGroup *group );
DOLL_FUNC Void DOLL_API gfx_flipSpriteHorizontal( RSprite *sprite );
DOLL_FUNC Void DOLL_API gfx_flipSpriteVertical( RSprite *sprite );
DOLL_FUNC Void DOLL_API gfx_setSpriteMirrorHorizontal( RSprite *sprite, Bool mirror );
DOLL_FUNC Void DOLL_API gfx_setSpriteMirrorVertical( RSprite *sprite, Bool mirror );
DOLL_FUNC Bool DOLL_API gfx_getSpriteMirrorHorizontal( const RSprite *sprite );
DOLL_FUNC Bool DOLL_API gfx_getSpriteMirrorVertical( const RSprite *sprite );

DOLL_FUNC Bool DOLL_API gfx_getSpriteSize( const RSprite *sprite, F32 *x, F32 *y );
DOLL_FUNC F32 DOLL_API gfx_getSpriteSizeX( const RSprite *sprite );
DOLL_FUNC F32 DOLL_API gfx_getSpriteSizeY( const RSprite *sprite );

DOLL_FUNC Void DOLL_API gfx_enableSpriteGroupVirtualResolution( RSpriteGroup *group );
DOLL_FUNC Void DOLL_API gfx_disableSpriteGroupVirtualResolution( RSpriteGroup *group );
DOLL_FUNC Bool DOLL_API gfx_isSpriteGroupVirtualResolutionEnabled( const RSpriteGroup *group );
DOLL_FUNC Void DOLL_API gfx_setSpriteGroupVirtualResolution( RSpriteGroup *group, F32 x, F32 y );
DOLL_FUNC Bool DOLL_API gfx_getSpriteGroupVirtualResolution( const RSpriteGroup *group, F32 *x, F32 *y );
DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupVirtualResolutionX( const RSpriteGroup *group );
DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupVirtualResolutionY( const RSpriteGroup *group );
DOLL_FUNC Void DOLL_API gfx_setSpriteGroupVirtualOrigin( RSpriteGroup *group, F32 x, F32 y );
DOLL_FUNC Bool DOLL_API gfx_getSpriteGroupVirtualOrigin( const RSpriteGroup *group, F32 *x, F32 *y );
DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupVirtualOriginX( const RSpriteGroup *group );
DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupVirtualOriginY( const RSpriteGroup *group );

DOLL_FUNC Void DOLL_API gfx_setSpriteVisible( RSprite *sprite, S32 visible );
DOLL_FUNC Bool DOLL_API gfx_isSpriteVisible( const RSprite *sprite );
```

## Texture

Textures hold the raw image data needed to draw sprites and some other graphics
to the screen.

It is often the case that different textures need to be loaded into memory, but
creating separate GPU objects for these would cause problematic overhead since
GPU state would need to be changed, necessitating extra draw calls. To solve
this, automatic texture atlassing is used, causing each image loaded to get put
into a single GPU object. This enables fewer draw commands to get issued to the
GPU and avoids unnecessary switching of GPU state.

Also remember that Doll has its own [virtual file system](Commands-IO.md), so
you don't necessarily need to store each image as a single file on a hard drive.
You can even create a virtual file system that generates images procedurally,
and issue load commands to that.

```cpp
DOLL_FUNC Bool DOLL_API gfx_isTextureResolutionValid( U16 resX, U16 resY );

DOLL_FUNC CTextureAtlas *DOLL_API gfx_newTextureAtlas( U16 resX, U16 resY, U16 format );
DOLL_FUNC CTextureAtlas *DOLL_API gfx_deleteTextureAtlas( CTextureAtlas *atlas );

DOLL_FUNC RTexture *DOLL_API gfx_newTexture( U16 width, U16 height, const void *data, ETextureFormat format );
DOLL_FUNC RTexture *DOLL_API gfx_newTextureInAtlas( U16 width, U16 height, const void *data, ETextureFormat format, CTextureAtlas *atlas );

DOLL_FUNC RTexture *DOLL_API gfx_loadTexture( Str filename );
DOLL_FUNC RTexture *DOLL_API gfx_loadTextureInAtlas( Str filename, CTextureAtlas *atlas );

DOLL_FUNC RTexture *DOLL_API gfx_deleteTexture( RTexture *textureId );
DOLL_FUNC U32 DOLL_API gfx_getTextureResX( const RTexture *textureId );
DOLL_FUNC U32 DOLL_API gfx_getTextureResY( const RTexture *textureId );
```

## Vertex

```cpp
enum ETopology
{
	kTopologyPointList,
	kTopologyLineList,
	kTopologyLineStrip,
	kTopologyTriangleList,
	kTopologyTriangleStrip,
	kTopologyTriangleFan
};

enum
{
	kVF_XYZ        = 0x00000000,
	kVF_XYZW       = 0x00000001,
	kVF_XY         = 0x00000002,
	kVF_DiffuseBit = 0x00000004,
	kVF_Tex0       = 0x00000000,
	kVF_Tex1       = 0x10000000,
	kVF_Tex2       = 0x20000000,
	kVF_Tex3       = 0x30000000,
	kVF_Tex4       = 0x40000000,
	kVF_Tex5       = 0x50000000,
	kVF_Tex6       = 0x60000000,
	kVF_Tex7       = 0x70000000,
	kVF_Tex8       = 0x80000000,

	kVFMask_Pos    = 0x00000003,
	kVFShft_Pos    = 0,

	kVFMask_Tex    = 0xF0000000,
	kVFShft_Tex    = 28
};

struct SVertex2D
{
	static const U32 kFmtPos = kVF_XYZW;

	F32 x, y, z, w;
	U32 diffuse;
	F32 u, v;
};
struct SVertex2DSprite
{
	static const U32 kFmtPos = kVF_XY;

	F32 x, y;
	U32 diffuse;
	F32 u, v;
};
```
