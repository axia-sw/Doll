#pragma once

#include "doll/Core/Defs.hpp"
#include "doll/Types/IntVector2.hpp"

namespace doll { namespace macOS {

	typedef struct {} *AppleFontManager;
	typedef struct {} *AppleFont;
	typedef struct {} *AppleMutableDictionary;
	typedef struct {} *AppleImage;
	typedef struct {} *AppleBitmap;

	class CocoaFont;
	class CocoaText;

	// Font data
	class CocoaFont {
		static AppleFontManager g_fontManager;

		AppleFont              m_font;
		AppleMutableDictionary m_attribs;

	public:
		CocoaFont();
		~CocoaFont();

		bool set( Str fontFamily, double size );
		void clear();

		bool render( CocoaText &dst, const Str &text, const SIntVector2 &size );
	};
	// Rendered text image
	class CocoaText {
	friend class CocoaFont;
		AppleBitmap m_bitmap;

	public:
		CocoaText();
		~CocoaText();

		const void *getBits() const;
		void freeBits();
	};

}}
