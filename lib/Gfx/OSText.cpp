#ifdef _MSC_VER
# pragma warning( push )
# pragma warning( disable: 4458 ) // declaration of 'nativeRegion' hides class member
#endif
#include <Windows.h>
#include <gdiplus.h>
#ifdef _MSC_VER
# pragma warning( pop )
#endif

#include "doll/Gfx/OSText.hpp"
#include "doll/Gfx/Texture.hpp"
#include "doll/Gfx/RenderCommands.hpp"
#include "doll/Core/Memory.hpp"
#include "doll/Core/MemoryTags.hpp"

#include <gl/GL.h> // ### FOR TESTING ###

namespace doll
{

	using namespace Gdiplus;

	// Collection of resources used for rendering text in a particular style
	struct STextStyle: public TPoolObject< STextStyle, kTag_Font >
	{
		// Number of references on this instance
		U32                  cRefs;
		// [GDI+] Font family used
		TTypeBuf<FontFamily> font;
		// Font size
		S32                  fontSize;

		STextStyle()
		: cRefs( 1 )
		, font()
		, fontSize( 12 )
		{
		}
		~STextStyle()
		{
		}

		inline STextStyle *grab()
		{
			++cRefs;
			return this;
		}
		inline STextStyle *drop()
		{
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

		// [GDI] Bitmap bits (directly readable)
		DWORD *          pBmpBits;
		// [GDI] Bitmap handle
		HBITMAP          hBmp;
		// [GDI] Temporary device context associated with this item
		HDC              hDC;

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
		, pBmpBits( nullptr )
		, hBmp( NULL )
		, hDC( NULL )
		, text()
		{
		}
		~STextItem()
		{
			if( hBmp != NULL ) {
				DeleteObject( hBmp );
				hBmp = NULL;

				pBmpBits = nullptr;
			}

			if( hDC != NULL ) {
				DeleteDC( hDC );
				hDC = NULL;
			}

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

	private:
		STextStyle *         m_pDefStyle;
		TMutArr<STextItem *> m_items;

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
	{
		GdiplusStartupInput gdiplusStartupInput;
		ULONG_PTR gdiplusToken;
		GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, NULL );
	}
	MOSText::~MOSText()
	{
	}

	Bool MOSText::setDefStyle( Str fontFamily, U32 fontSize )
	{
		STextStyle *const pTextStyle = new STextStyle();
		if( !AX_VERIFY_MEMORY( pTextStyle ) ) {
			return false;
		}

		wchar_t wszBuf[ 256 ] = { L'\0' };
		if( !AX_VERIFY_MSG( fontFamily.toWStr( wszBuf ), "Invalid UTF-8 encoding" ) ) {
			delete pTextStyle;
			return false;
		}

		pTextStyle->font.init( wszBuf );
		pTextStyle->fontSize = S32( fontSize );
		
		if( m_pDefStyle != nullptr ) {
			m_pDefStyle->drop();
		}

		m_pDefStyle = pTextStyle;
		return true;
	}
	STextItem *MOSText::newText( Str text, const SIntVector2 &size, U32 lineColor, U32 fillColor )
	{
		if( !m_pDefStyle && !setDefStyle( "Verdana", 18 ) ) {
			return nullptr;
		}

		AX_ASSERT_NOT_NULL( m_pDefStyle );

		STextItem *const pTextItem = new STextItem();
		if( !AX_VERIFY_MEMORY( pTextItem ) ) {
			return nullptr;
		}

		if( !AX_VERIFY_MEMORY( pTextItem->text.tryAssign( text ) ) ) {
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
		return pTextItem;
	}

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
	Void MOSText::drawText( STextItem &item )
	{
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

		item.uResX = item.drawSize.x;
		item.uResY = item.drawSize.y;
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
		return ( const Void * )pText->pBmpBits;
	}

	DOLL_FUNC U16 DOLL_API gfx_ostext_makeTexture( const STextItem *pText, CTextureAtlas *pDefAtlas )
	{
		AX_ASSERT_NOT_NULL( pText );

		RTexture *const pTex = g_textureMgr.makeTexture( pText->uResX, pText->uResY, pText->pBmpBits, kTexFmtRGBA8, pDefAtlas );
		if( !AX_VERIFY_MEMORY( pTex ) ) {
			return 0;
		}

		return pTex->getIdentifier();
	}

	DOLL_FUNC U16 DOLL_API gfx_renderOSText( Str text, const SIntVector2 &size, U32 lineColor, U32 fillColor, CTextureAtlas *pDefAtlas )
	{
		STextItem *const pTextItem = gfx_newOSText( text, size, lineColor, fillColor );
		if( !AX_VERIFY_MEMORY( pTextItem ) ) {
			return 0;
		}

		gfx_ostext_fillCache( pTextItem );

		const U16 tex = gfx_ostext_makeTexture( pTextItem, pDefAtlas );
		gfx_deleteOSText( pTextItem );

		return tex;
	}

	DOLL_FUNC void DOLL_API gfx_drawOSText( Str text, const SRect &area )
	{
		static U16 oldTex = 0;

		if( oldTex != 0 ) {
			gfx_deleteTexture( oldTex );
		}

		if( !( oldTex = gfx_renderOSText( text, area.size() ) ) ) {
			return;
		}

		gfx_queDrawImage( area.x1, area.y1, area.resX(), area.resY(), 0, 0, area.resX(), area.resY(), ~0U, ~0U, ~0U, ~0U, oldTex );
	}

}
