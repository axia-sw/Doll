#define DOLL_TRACE_FACILITY doll::kLog_GfxOSText

#include "doll/Core/Defs.hpp"
#include "doll/Core/Logger.hpp"
#include "doll/Core/Engine.hpp"

#define DOLL_OSTEXT_GDIPLUS 0
#define DOLL_OSTEXT_DWRITE  0
#define DOLL_OSTEXT_COCOA   0

#if AX_OS_UWP
# undef  DOLL_OSTEXT_DWRITE
# define DOLL_OSTEXT_DWRITE 1
#elif AX_OS_WINDOWS
# undef  DOLL_OSTEXT_GDIPLUS
# define DOLL_OSTEXT_GDIPLUS 1
#elif AX_OS_MACOSX || AX_OS_IOS
# undef  DOLL_OSTEXT_COCOA
# define DOLL_OSTEXT_COCOA   1
#endif

#ifdef _WIN32
# ifdef _MSC_VER
#  pragma warning( push )
#  pragma warning( disable: 4458 ) // declaration of 'nativeRegion' hides class member
# endif
# include <Windows.h>
# if DOLL_OSTEXT_GDIPLUS
#  include <wtypes.h> // needed for mingw `PROPID` definition
#  include <gdiplus.h>
# endif
# ifdef _MSC_VER
#  pragma warning( pop )
# endif
#endif

#if DOLL_OSTEXT_COCOA
# include "macOS/OSText_Cocoa.hpp"
#endif

#include "doll/Gfx/OSText.hpp"
#include "doll/Gfx/Texture.hpp"
#include "doll/Gfx/RenderCommands.hpp"
#include "doll/Core/Memory.hpp"
#include "doll/Core/MemoryTags.hpp"

// ### FOR TESTING ###
#ifdef __APPLE__
# include <OpenGL/OpenGL.h>
#else
# include <GL/gl.h>
#endif

namespace doll
{

#if DOLL_OSTEXT_GDIPLUS
	using namespace Gdiplus;
#endif

	// Collection of resources used for rendering text in a particular style
	struct STextStyle: public TPoolObject< STextStyle, kTag_Font >
	{
		// Number of references on this instance
		U32                  cRefs;
#if DOLL_OSTEXT_GDIPLUS
		// [GDI+] Font family used
		TTypeBuf<FontFamily> font;
#elif DOLL_OSTEXT_COCOA
		// [Cocoa] Text style specific data
		macOS::CocoaFont     font;
#endif
		// Font size
		S32                  fontSize;

		STextStyle()
		: cRefs( 1 )
#if DOLL_OSTEXT_GDIPLUS || DOLL_OSTEXT_COCOA
		, font()
#endif
		, fontSize( 12 )
		{
			DOLL_TRACE( "STextStyle::STextStyle()" );
		}
		~STextStyle()
		{
			DOLL_TRACE( "STextStyle::~STextStyle()" );
		}

		inline STextStyle *grab()
		{
			DOLL_TRACE( "STextStyle::grab()" );
			++cRefs;
			return this;
		}
		inline STextStyle *drop()
		{
			DOLL_TRACE( "STextStyle::drop()" );
			if( --cRefs == 0 ) {
				delete this;
			}

			return nullptr;
		}
	};
	// Cached text item
	struct STextItem: public TPoolObject< STextItem, kTag_Font >
	{
		// Frame ID this item was last used on
		U32              uRenderId;

		// Width of this text item
		U32              uResX;
		// Height of this text item
		U32              uResY;

		// Area this text item will be rendered into
		//
		// NOTE: This might not match uResX/uResY, which will hold the final
		//       bitmap size (the area actually containing text)
		SIntVector2      drawSize;

		// Text style to use for this
		STextStyle *     pStyle;

		// Outline color
		U32              uLineColor;
		// Fill color
		U32              uFillColor;

#if DOLL_OSTEXT_GDIPLUS
		// [GDI] Bitmap bits (directly readable)
		DWORD *          pBmpBits;
		// [GDI] Bitmap handle
		HBITMAP          hBmp;
		// [GDI] Temporary device context associated with this item
		HDC              hDC;
#elif DOLL_OSTEXT_COCOA
		// [Cocoa] Text image
		macOS::CocoaText textImage;
#endif

		// Text string to render
		MutStr           text;

		STextItem()
		: uRenderId( ~0U )
		, uResX( 0 )
		, uResY( 0 )
		, drawSize()
		, pStyle( nullptr )
		, uLineColor( 0xFF000000 )
		, uFillColor( 0xFFFFFFFF )
#if DOLL_OSTEXT_GDIPLUS
		, pBmpBits( nullptr )
		, hBmp( NULL )
		, hDC( NULL )
#elif DOLL_OSTEXT_COOCA
		, textImage()
#endif
		, text()
		{
			DOLL_TRACE( "STextItem::STextItem()" );
		}
		~STextItem()
		{
			DOLL_TRACE( "STextItem::~STextItem()" );
#if DOLL_OSTEXT_GDIPLUS
			if( hBmp != NULL ) {
				DeleteObject( hBmp );
				hBmp = NULL;

				pBmpBits = nullptr;
			}

			if( hDC != NULL ) {
				DeleteDC( hDC );
				hDC = NULL;
			}
#endif

			if( pStyle != nullptr ) {
				pStyle = pStyle->drop();
			}
		}
	};

	class MOSText
	{
	public:
		static MOSText &get();

		Bool setDefStyle( Str fontFamily, U32 fontSize );
		STextItem *newText( Str text, const SIntVector2 &size, U32 lineColor, U32 fillColor );
		Void drawText( STextItem &item );

		Void addRenderedText( RTexture *texture );
		Void clearRenderedTexts();

	private:
		STextStyle *              m_pDefStyle;
		TSmallArr<STextItem *, 8> m_items;

		TSmallArr<RTexture *, 8>  m_renderCache;
		U32                       m_lastCacheFrameId;

		MOSText();
		~MOSText();
	};
	static TManager<MOSText> g_osTextMgr;

	MOSText &MOSText::get()
	{
		static MOSText instance;
		return instance;
	}

	MOSText::MOSText()
	: m_pDefStyle( nullptr )
	, m_items()
	, m_renderCache()
	, m_lastCacheFrameId(0)
	{
		DOLL_TRACE( "MOSText::MOSText()" );
#if DOLL_OSTEXT_GDIPLUS
		GdiplusStartupInput gdiplusStartupInput;
		ULONG_PTR gdiplusToken;
		GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, NULL );
#endif
	}
	MOSText::~MOSText()
	{
		DOLL_TRACE( "MOSText::~MOSText()" );
		clearRenderedTexts();
	}

	Bool MOSText::setDefStyle( Str fontFamily, U32 fontSize )
	{
		DOLL_TRACE( axf( "MOSText::setDefStyle(fontFamily: \"%.*s\", size: %u)",
			fontFamily.lenInt(), fontFamily.get(), fontSize ) );

		STextStyle *const pTextStyle = new STextStyle();
		if( !AX_VERIFY_MEMORY( pTextStyle ) ) {
			DOLL_TRACE( " - !pTextStyle" );
			return false;
		}

#if DOLL_OSTEXT_GDIPLUS
		wchar_t wszBuf[ 256 ] = { L'\0' };
		if( !AX_VERIFY_MSG( fontFamily.toWStr( wszBuf ), "Invalid UTF-8 encoding" ) ) {
			DOLL_TRACE( " - !fontFamily.toWStr()" );
			delete pTextStyle;
			return false;
		}

		pTextStyle->font.init( wszBuf );
#elif DOLL_OSTEXT_COCOA
		if( !pTextStyle->font.set( fontFamily, double(fontSize) ) ) {
			DOLL_TRACE( "Failed to set font (Cocoa)" );
		}
#endif
		pTextStyle->fontSize = S32( fontSize );

		if( m_pDefStyle != nullptr ) {
			m_pDefStyle->drop();
		}

		m_pDefStyle = pTextStyle;
		DOLL_TRACE( "Font set successfully" );
		return true;
	}
	STextItem *MOSText::newText( Str text, const SIntVector2 &size, U32 lineColor, U32 fillColor )
	{
		static const char *const defaultFontFamily =
#if AX_OS_WINDOWS || AX_OS_UWP
			"Verdana"
#elif AX_OS_MACOSX || AX_OS_IOS
			"Helvetica"
#else
			"sans"
#endif
			;

		DOLL_TRACE( axf( "MOSText::newText(text: \"%.*s\", size: [%i, %i], lineColor: 0x%.8X, fillColor: 0x%.8X)",
			text.lenInt(), text.get(), size.x, size.y, lineColor, fillColor ) );

		if( !m_pDefStyle && !setDefStyle( defaultFontFamily, 18 ) ) {
			DOLL_TRACE( " - !setDefStyle()" );
			return nullptr;
		}

		AX_ASSERT_NOT_NULL( m_pDefStyle );

		STextItem *const pTextItem = new STextItem();
		if( !AX_VERIFY_MEMORY( pTextItem ) ) {
			DOLL_TRACE( " - !pTextItem" );
			return nullptr;
		}

		if( !AX_VERIFY_MEMORY( pTextItem->text.tryAssign( text ) ) ) {
			DOLL_TRACE( " - !pTextItem->text.tryAssign()" );
			delete pTextItem;
			return nullptr;
		}

		pTextItem->uRenderId = 0; // ### TODO ### Get current render id

		pTextItem->drawSize = size;

		pTextItem->pStyle = m_pDefStyle->grab();
		pTextItem->uLineColor = lineColor;
		pTextItem->uFillColor = fillColor;

		// ### TODO ### Measure the text so that only the minimum size needed is
		//              used.

#if DOLL_OSTEXT_GDIPLUS
		BITMAPINFO bi;
		memset( &bi, 0, sizeof( bi ) );
		bi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
		bi.bmiHeader.biWidth = size.x;
		bi.bmiHeader.biHeight = size.y;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biCompression = BI_RGB;
		bi.bmiHeader.biBitCount = 32;

		HDC hGLDC = wglGetCurrentDC();
		if( hGLDC != NULL ) {
			pTextItem->hDC = CreateCompatibleDC( hGLDC );
		} else {
			pTextItem->hDC = CreateDCW( nullptr, nullptr, nullptr, nullptr );
		}
		if( !pTextItem->hDC ) {
			delete pTextItem;
			return nullptr;
		}

		pTextItem->hBmp =
			CreateDIBSection
			(
				pTextItem->hDC,
				&bi,
				DIB_RGB_COLORS,
				( void ** )&pTextItem->pBmpBits,
				NULL,
				0
			);
		if( !pTextItem->hBmp ) {
			delete pTextItem;
			return nullptr;
		}

		SelectObject( pTextItem->hDC, pTextItem->hBmp );
#endif

		return pTextItem;
	}

#if DOLL_OSTEXT_GDIPLUS
	static Gdiplus::Color getGdipColor( U32 uColor )
	{
		return
			Gdiplus::Color
			(
				DOLL_COLOR_A( uColor ),
				DOLL_COLOR_R( uColor ),
				DOLL_COLOR_G( uColor ),
				DOLL_COLOR_B( uColor )
			);
	}
#endif
	Void MOSText::drawText( STextItem &item )
	{
		DOLL_TRACE( "MOSText::drawText()" );
#if DOLL_OSTEXT_GDIPLUS
		AX_ASSERT_NOT_NULL( item.pStyle );
		AX_ASSERT_NOT_NULL( item.hBmp );
		AX_ASSERT_NOT_NULL( item.hDC );

		wchar_t wszText[ 4096 ];
		if( !item.text.toWStr( wszText ) ) {
			return;
		}

		Graphics gfx( item.hDC );
		gfx.SetSmoothingMode( Gdiplus::SmoothingModeAntiAlias );
		gfx.SetInterpolationMode( Gdiplus::InterpolationModeHighQualityBicubic );

		//FontFamily fontFamily( L"MS Gothic" );

		GraphicsPath path;
		StringFormat strfmt;
		path.AddString( wszText, (INT)wcslen( wszText ), item.pStyle->font.ptr(), Gdiplus::FontStyleRegular, (Gdiplus::REAL)item.pStyle->fontSize, Gdiplus::Rect( 0, 0, item.drawSize.x, item.drawSize.y ), &strfmt );

		Pen pen( getGdipColor( item.uLineColor ), 3 );
		pen.SetLineJoin(LineJoinRound);

		gfx.DrawPath( &pen, &path );

		SolidBrush brush( getGdipColor( item.uFillColor ) );
		gfx.FillPath( &brush, &path );

		gfx.Flush( Gdiplus::FlushIntentionSync );
		GdiFlush();
#elif DOLL_OSTEXT_COCOA
		AX_ASSERT_NOT_NULL( item.pStyle );

		item.pStyle->font.render( item.textImage, item.text, item.drawSize );
#endif

		item.uResX = item.drawSize.x;
		item.uResY = item.drawSize.y;
	}

	Void MOSText::addRenderedText( RTexture *texture ) {
		const U32 currentCacheFrameId = DOLL__CORESTRUC.frame.uRenderId;
		if( m_lastCacheFrameId != currentCacheFrameId ) {
			m_lastCacheFrameId = currentCacheFrameId;
			clearRenderedTexts();
		}

		AX_EXPECT_MEMORY( m_renderCache.append( texture ) );
	}
	Void MOSText::clearRenderedTexts() {
		for( SizeType n = m_renderCache.len(); n != 0; --n ) {
			const SizeType i = n - 1;
			m_renderCache[ i ] = gfx_deleteTexture( m_renderCache[ i ] );
		}
		m_renderCache.clear();
	}

	DOLL_FUNC STextItem *DOLL_API gfx_newOSText( Str text, const SIntVector2 &size, U32 lineColor, U32 fillColor )
	{
		return g_osTextMgr->newText( text, size, lineColor, fillColor );
	}
	DOLL_FUNC STextItem *DOLL_API gfx_deleteOSText( STextItem *pText )
	{
		delete pText;
		return nullptr;
	}
	DOLL_FUNC Void DOLL_API gfx_ostext_fillCache( STextItem *pText )
	{
		AX_ASSERT_NOT_NULL( pText );
		g_osTextMgr->drawText( *pText );
	}
	DOLL_FUNC U32 DOLL_API gfx_ostext_resX( const STextItem *pText )
	{
		AX_ASSERT_NOT_NULL( pText );
		return pText->uResX;
	}
	DOLL_FUNC U32 DOLL_API gfx_ostext_resY( const STextItem *pText )
	{
		AX_ASSERT_NOT_NULL( pText );
		return pText->uResY;
	}
	DOLL_FUNC const Void *DOLL_API gfx_ostext_getBits( const STextItem *pText )
	{
		AX_ASSERT_NOT_NULL( pText );
#if DOLL_OSTEXT_GDIPLUS
		return ( const Void * )pText->pBmpBits;
#elif DOLL_OSTEXT_COCOA
		return ( const Void * )pText->textImage.getBits();
#else
		return nullptr;
#endif
	}

	DOLL_FUNC RTexture *DOLL_API gfx_ostext_makeTexture( const STextItem *pText, CTextureAtlas *pDefAtlas )
	{
		AX_ASSERT_NOT_NULL( pText );

		RTexture *const pTex = g_textureMgr.makeTexture( pText->uResX, pText->uResY, gfx_ostext_getBits( pText ), kTexFmtRGBA8, pDefAtlas );
		if( !AX_VERIFY_MEMORY( pTex ) ) {
			return nullptr;
		}

		return pTex;
	}

	DOLL_FUNC RTexture *DOLL_API gfx_renderOSText( Str text, const SIntVector2 &size, U32 lineColor, U32 fillColor, CTextureAtlas *pDefAtlas )
	{
		STextItem *const pTextItem = gfx_newOSText( text, size, lineColor, fillColor );
		if( !AX_VERIFY_MEMORY( pTextItem ) ) {
			return 0;
		}

		gfx_ostext_fillCache( pTextItem );

		RTexture *const tex = gfx_ostext_makeTexture( pTextItem, pDefAtlas );
		gfx_deleteOSText( pTextItem );

		return tex;
	}

	DOLL_FUNC void DOLL_API gfx_drawOSText( Str text, const SRect &area )
	{
		RTexture *const tex = gfx_renderOSText( text, area.size() );
		if( !tex ) {
			return;
		}

		g_osTextMgr->addRenderedText( tex );

		gfx_queDrawImage( area.x1, area.y1, area.resX(), area.resY(), 0, 0, area.resX(), area.resY(), ~0U, ~0U, ~0U, ~0U, tex );
	}

}
