#include "doll/Script/Token.hpp"

namespace doll { namespace script {

	static const struct STokTyStr { const Str name; ETokenType type; } g_tokenTypes[] = {
#define E__(X_) { ( #X_ ), kTT_##X_ }
		E__(None),
		E__(Keyword),
		E__(Name),
		E__(Type),
		E__(Label),
		E__(ConfigVar),
		E__(SystemVar),
		E__(TagRef),
		E__(ProgTagRef),
		E__(ResRef),
		E__(CharRef),
		E__(KoePrefix),
		E__(Punctuation),
		E__(NumericLiteral),
		E__(StringLiteral),
		E__(Koe),
		E__(Speaker),
		E__(Message),
		E__(Blank),
		E__(Comment),

		{ "CfgVar", kTT_ConfigVar },
		{ "SysRef", kTT_SystemVar },
		{ "AppRef", kTT_ProgTagRef },
		{ "Punct", kTT_Punctuation },
		{ "Number", kTT_NumericLiteral },
		{ "StrLit", kTT_StringLiteral }
#undef E__
	};

	DOLL_FUNC Bool DOLL_API scr_tokenTypeToString( Str &dst, ETokenType src )
	{
		for( const auto &x : g_tokenTypes ) {
			if( x.type == src ) {
				dst = x.name;
				return true;
			}
		}

		return false;
	}
	DOLL_FUNC Bool DOLL_API scr_stringToTokenType( ETokenType &dst, Str src )
	{
		for( const auto &x : g_tokenTypes ) {
			if( x.name.caseCmp( src ) ) {
				dst = x.type;
				return true;
			}
		}

		return false;
	}

}}
