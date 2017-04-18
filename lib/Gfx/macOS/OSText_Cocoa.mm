#include "OSText_Cocoa.hpp"
#import <AppKit/AppKit.h>

#include "doll/Core/Logger.hpp"

namespace doll { namespace macOS {

	inline NSFontManager *toCocoa( AppleFontManager src ) {
		return reinterpret_cast< NSFontManager * >( src );
	}
	inline NSFont *toCocoa( AppleFont src ) {
		return reinterpret_cast< NSFont * >( src );
	}
	inline NSImage *toCocoa( AppleImage src ) {
		return reinterpret_cast< NSImage * >( src );
	}
	inline NSBitmapImageRep *toCocoa( AppleBitmap src ) {
		return reinterpret_cast< NSBitmapImageRep * >( src );
	}
	inline NSMutableDictionary *toCocoa( AppleMutableDictionary src ) {
		return reinterpret_cast< NSMutableDictionary * >( src );
	}

	inline AppleFontManager fromCocoa( NSFontManager *dst ) {
		return reinterpret_cast< AppleFontManager >( dst );
	}
	inline AppleFont fromCocoa( NSFont *dst ) {
		return reinterpret_cast< AppleFont >( dst );
	}
	inline AppleImage fromCocoa( NSImage *dst ) {
		return reinterpret_cast< AppleImage >( dst );
	}
	inline AppleBitmap fromCocoa( NSBitmapImageRep *dst ) {
		return reinterpret_cast< AppleBitmap >( dst );
	}
	inline AppleMutableDictionary fromCocoa( NSMutableDictionary *dst ) {
		return reinterpret_cast< AppleMutableDictionary >( dst );
	}

	inline NSString *getCocoaString( Str src ) {
		return
			[[NSString alloc]
				initWithBytes:(const void *)src.get()
				length:(NSUInteger)src.len()
				encoding:NSUTF8StringEncoding];
	}

	// ------------------------------------------------------------------ //

	AppleFontManager CocoaFont::g_fontManager = nullptr;

	CocoaFont::CocoaFont()
	: m_font( nullptr )
	, m_attribs( nullptr )
	{
		DOLL_TRACE( "CocoaFont::CocoaFont()" );

		if( !g_fontManager ) {
			g_fontManager = fromCocoa( [NSFontManager sharedFontManager] );
		}
	}
	CocoaFont::~CocoaFont() {
		DOLL_TRACE( "CocoaFont::~CocoaFont()" );
		clear();
	}

	bool CocoaFont::set( Str fontFamily, double size ) {
		DOLL_TRACE( axf( "CocoaFont::set(fontFamily: \"%.*s\", size: %.2f)",
			fontFamily.lenInt(), fontFamily.get(), size ) );
		NSString *const fontFamilyStr = getCocoaString( fontFamily );
		if( !fontFamilyStr ) {
			DOLL_TRACE( " - !fontFamily" );
			return false;
		}

		NSFont *const font =
			[toCocoa(g_fontManager)
				fontWithFamily:fontFamilyStr
				traits:0
				weight:8
				size:(CGFloat)size];
		if( !font ) {
			DOLL_TRACE( " - !font" );
			[fontFamilyStr dealloc];
			return false;
		}

		NSMutableDictionary *const attribs =
			[[NSMutableDictionary dictionaryWithCapacity: 5] retain];
		if( !attribs ) {
			DOLL_TRACE( " - !attribs" );
			[fontFamilyStr dealloc];
			[font dealloc];
			return false;
		}

		[attribs setObject:font forKey:NSFontAttributeName];
		[attribs setObject:[NSColor blackColor] forKey:NSStrokeColorAttributeName];
		[attribs setObject:[NSColor whiteColor] forKey:NSForegroundColorAttributeName];
		[attribs setObject:@-3.0 forKey:NSStrokeWidthAttributeName];

		[fontFamilyStr dealloc];

		clear();

		m_font = fromCocoa( font );
		m_attribs = fromCocoa( attribs );

		DOLL_TRACE( " + OK" );
		return true;
	}
	void CocoaFont::clear() {
		DOLL_TRACE( "CocoaFont::clear()" );

		if( m_attribs != nullptr ) {
			[toCocoa(m_attribs) dealloc];
			m_attribs = nullptr;
		}
		if( m_font != nullptr ) {
			[toCocoa(m_font) dealloc];
			m_font = nullptr;
		}
	}

	bool CocoaFont::render( CocoaText &dst, const Str &text, const SIntVector2 &size ) {
		AX_ASSERT_MSG( m_font != nullptr, "CocoaFont::set() not called or not successful" );

		DOLL_TRACE( axf( "CocoaFont::render(text: \"%.*s\", size: [%i, %i])",
			text.lenInt(), text.get(), size.x, size.y ) );

		NSString *const textStr = getCocoaString( text );
		if( !textStr ) {
			DOLL_TRACE( " - !textStr" );
			return false;
		}

		//NSSize frameSize = [textStr sizeWithAttributes: m_attribs];
		const NSSize frameSize = NSMakeSize( double(size.x), double(size.y) );

		NSImage *const image = [[NSImage alloc] initWithSize:frameSize];
		if( !image ) {
			DOLL_TRACE( " - !image" );
			[textStr dealloc];
			return false;
		}

		[image lockFocusFlipped:YES];

		NSAffineTransform *const xform = [NSAffineTransform transform];
		[xform translateXBy:0 yBy:double(size.y)];
		[xform scaleXBy:1 yBy:-1];
		[xform concat];

		[textStr drawAtPoint:NSMakePoint(0, 0) withAttributes: toCocoa(m_attribs)];

		NSBitmapImageRep *const bmp =
			[[NSBitmapImageRep alloc]
				initWithFocusedViewRect:NSMakeRect(0.0f, 0.0f, frameSize.width, frameSize.height)];

		[image unlockFocus];

		if( !bmp ) {
			DOLL_TRACE( " - !bmp" );
			[textStr dealloc];
			[image dealloc];
			return false;
		}

		dst.freeBits();
		dst.m_bitmap = fromCocoa(bmp);

		DOLL_TRACE( " + OK" );
		return true;
	}

	// ------------------------------------------------------------------ //

	CocoaText::CocoaText()
	: m_bitmap( nullptr )
	{
		DOLL_TRACE( "CocoaText::CocoaText()" );
	}
	CocoaText::~CocoaText() {
		DOLL_TRACE( "CocoaText::~CocoaText()" );
		freeBits();
	}

	const void *CocoaText::getBits() const {
		AX_ASSERT_NOT_NULL( m_bitmap );
		const void *const p = (const void *)[toCocoa(m_bitmap) bitmapData];
		DOLL_TRACE( axf( "CocoaText::getBits() -> %#p", p ) );
		return p;
	}
	void CocoaText::freeBits() {
		DOLL_TRACE( "CocoaText::freeBits()" );
		if( m_bitmap != nullptr ) {
			[toCocoa(m_bitmap) dealloc];
			m_bitmap = nullptr;
		}
	}

}}
