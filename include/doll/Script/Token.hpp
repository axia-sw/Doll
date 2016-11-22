#pragma once

#include "../Core/Defs.hpp"

namespace doll { namespace script {

	class Ident;

	// Type of a token
	enum ETokenType: U8
	{
		// No token (EOF or error)
		kTT_None,

		// Normal identifier that is a keyword
		kTT_Keyword,
		// Normal identifier "[_]*[a-z][a-zA-Z0-9_]+"
		kTT_Name,
		// Type identifier "[_]*[A-Z][a-zA-Z0-9_]+"
		kTT_Type,
		// Label identifier "\*[a-zA-Z0-9_\.]+"
		kTT_Label,
		// Configuration variable reference "#[a-zA-Z0-9_\.]+"
		kTT_ConfigVar,
		// System variable reference "$[a-zA-Z0-9_\.]+"
		kTT_SystemVar,
		// Tag reference "$<[a-zA-Z0-9_\.]+>"
		kTT_TagRef,
		// Program tag reference "#<[a-zA-Z0-9_\.]+>"
		kTT_ProgTagRef,
		// Resource reference "%[a-zA-Z0-9_\.]+"
		kTT_ResRef,
		// Character reference "@[a-zA-Z0-9_\.]+"
		kTT_CharRef,
		// Koe prefix (e.g., 声 or '[' at the start of a line)
		kTT_KoePrefix,
		// Punctuation (e.g., `!=` or `.`)
		kTT_Punctuation,
		// Numeric literal
		kTT_NumericLiteral,
		// String literal
		kTT_StringLiteral,
		// Set the vocal track to play (e.g., [000201780] or 声000201780)
		kTT_Koe,
		// Set the speaker (e.g., @@"Protag-kun" or 【Protag-kun】)
		kTT_Speaker,
		// Message (e.g., >>"Hello." or 「Hello.」)
		kTT_Message,
		// Blank menu item (produced only inside `menu` blocks)
		kTT_Blank,

		// Comment in the code
		kTT_Comment
	};

	// Subtoken when type is `kTT_None`
	enum ESubtokenNone: U8
	{
		// End-of-File found
		kTN_End = 0,

		// Error encountered during read
		kTN_Error = 1
	};
	// Subtoken when type is `kTT_Keyword`
	enum ESubtokenKeyword: U8
	{
		// `null`
		kKW_Null,
		// `false`
		kKW_False,
		// `true`
		kKW_True,
		// `if`
		kKW_If,
		// `elseif`
		kKW_ElseIf,
		// `else`
		kKW_Else,
		// `menu`
		kKW_Menu,
		// `loop`
		kKW_Loop,
		// `while`
		kKW_While,
		// `do`
		kKW_Do,
		// `for`
		kKW_For,
		// `in`
		kKW_In,
		// `struct`
		kKW_Struct,
		// `class`
		kKW_Class,
		// `extension`
		kKW_Extension,
		// `interface`
		kKW_Interface,
		// `protocol`
		kKW_Protocol,
		// `func`
		kKW_Func,
		// `var`
		kKW_Var,
		// `let`
		kKW_Let,
		// `goto`
		kKW_Goto,
		// `return`
		kKW_Return,
		// `switch`
		kKW_Switch,
		// `case`
		kKW_Case,
		// `default`
		kKW_Default,
		// `break`
		kKW_Break,
		// `continue`
		kKW_Continue,
		// `defer`
		kKW_Defer,
		// `multimessage`
		kKW_Multimessage
	};

	// Subtoken when type is `kTT_Type`
	enum ESubtokenType: U8
	{
		// a user-specified type
		kTy_User,

		// `Void`
		kTy_Void,
		// `Null`
		kTy_Null,
		// `S1` or `Int1`
		kTy_S1,
		// `S8` or `Int8` or `Char`
		kTy_S8,
		// `S16` or `Int16` or `Short`
		kTy_S16,
		// `S32` or `Int32` or `Int`
		kTy_S32,
		// `S64` or `Int64` or `Long`
		kTy_S64,
		// `U1` or `UInt1` or `Bool`
		kTy_U1,
		// `U8` or `UInt8` or `Byte`
		kTy_U8,
		// `U16` or `UInt16` or `Word`
		kTy_U16,
		// `U32` or `UInt32` or `Dword`
		kTy_U32,
		// `U64` or `UInt64` or `Qword`
		kTy_U64,
		// `F32` or `Float`
		kTy_F32,
		// `F64` or `Double`
		kTy_F64,
		// `Str` or `StringView`
		kTy_StringView,
		// `MutStr` or `MutableString`
		kTy_MutableString
	};

	// Subtoken when type is `kTT_Punctuation`
	enum ESubtokenPunctuation: U8
	{
		// `(`
		kPn_LParen,
		// `)`
		kPn_RParen,
		// `[`
		kPn_LBracket,
		// `]`
		kPn_RBracket,
		// `{`
		kPn_LBrace,
		// `}`
		kPn_RBrace,

		// `;`
		kPn_Semicolon,
		// `:`
		kPn_Colon,
		// `,`
		kPn_Comma,
		// `.`
		kPn_Dot,
		// `...`
		kPn_Ellipsis,
		// `..<`
		kPn_HalfOpenRange,
		
		// `?`
		kPn_Conditional,
		// `??`
		kPn_NilCoalesce,
		
		// `=>`
		kPn_FuncDef,

		// `=`
		kPn_Assign,
		// `:=`
		kPn_AutoAssign,
		// `?=`
		kPn_OptionalAssign,

		// `+`
		kPn_Add,
		// `-`
		kPn_Sub,
		// `*`
		kPn_Mul,
		// `/`
		kPn_Div,
		// `%`
		kPn_Mod,
		// `~`
		kPn_BitNot,
		// `|`
		kPn_BitOr,
		// `&`
		kPn_BitAnd,
		// `^`
		kPn_BitXor,
		// `<<`
		kPn_LSh,
		// `>>`
		kPn_RSh,

		// `&+`
		kPn_AddOF,
		// `&-`
		kPn_SubOF,
		// `&*`
		kPn_MulOF,

		// `||`
		kPn_RelOr,
		// `&&`
		kPn_RelAnd,
		// `!`
		kPn_RelNot,

		// `<`
		kPn_Lt,
		// `>`
		kPn_Gt,
		// `<=`
		kPn_LE,
		// `>=`
		kPn_GE,
		// `!=`
		kPn_NE,
		// `==`
		kPn_Eq,
		// `~=`
		kPn_ApxEq,
		// `===`
		kPn_IdEq,
		// `!==`
		kPn_IdNE,

		// `++`
		kPn_Inc,
		// `--`
		kPn_Dec,

		// `<=>`
		kPn_Swap,

		kPn_F_CompoundAssign = 0x40,
		kPn_M_CompoundAssign = kPn_F_CompoundAssign - 1,

		// `+=`
		kPn_AddAssign    = kPn_F_CompoundAssign | kPn_Add,
		// `-=`
		kPn_SubAssign    = kPn_F_CompoundAssign | kPn_Sub,
		// `*=`
		kPn_MulAssign    = kPn_F_CompoundAssign | kPn_Mul,
		// `/=`
		kPn_DivAssign    = kPn_F_CompoundAssign | kPn_Div,
		// `%=`
		kPn_ModAssign    = kPn_F_CompoundAssign | kPn_Mod,
		// `|=`
		kPn_BitOrAssign  = kPn_F_CompoundAssign | kPn_BitOr,
		// `&=`
		kPn_BitAndAssign = kPn_F_CompoundAssign | kPn_BitAnd,
		// `^=`
		kPn_BitXorAssign = kPn_F_CompoundAssign | kPn_BitXor,
		// `<<=`
		kPn_LShAssign    = kPn_F_CompoundAssign | kPn_LSh,
		// `>>=`
		kPn_RShAssign    = kPn_F_CompoundAssign | kPn_RSh,
		// `&+=`
		kPn_AddOFAssign  = kPn_F_CompoundAssign | kPn_AddOF,
		// `&-=`
		kPn_SubOFAssign  = kPn_F_CompoundAssign | kPn_SubOF,
		// `&*=`
		kPn_MulOFAssign  = kPn_F_CompoundAssign | kPn_MulOF,
		// `||=`
		kPn_RelOrAssign  = kPn_F_CompoundAssign | kPn_RelOr,
		// `&&=`
		kPn_RelAndAssign = kPn_F_CompoundAssign | kPn_RelAnd
	};

	// Subtoken when type is `kTT_NumericLiteral`
	enum ESubtokenNumber: U8
	{
		kSN_TyS1  = 0x00,
		kSN_TyS8  = 0x01,
		kSN_TyS16 = 0x02,
		kSN_TyS32 = 0x03,
		kSN_TyS64 = 0x04,
		kSN_TyU1  = 0x05,
		kSN_TyU8  = 0x06,
		kSN_TyU16 = 0x07,
		kSN_TyU32 = 0x08,
		kSN_TyU64 = 0x09,
		kSN_TyF32 = 0x0A,
		kSN_TyF64 = 0x0B,

		kSN_TyMask = 0x0F,

		// no attribute
		kSN_AtNone = 0x00,

		// "mm" -- millimeters
		kSN_AtMm   = 0x10,
		// "cm" -- centimeters
		kSN_AtCm   = 0x20,
		// "in" -- inches
		kSN_AtIn   = 0x30,
		// "px" -- pixels
		kSN_AtPx   = 0x40,
		// "em" -- em
		kSN_AtEm   = 0x50,
		// "pt" -- points
		kSN_AtPt   = 0x60,

		// "ms" -- milliseconds
		kSN_AtMs   = 0x70,
		// "sec" -- seconds
		kSN_AtSec  = 0x80,

		kSN_AtMask = 0xF0
	};

	// Subtoken when type is `kTT_StringLiteral`
	enum ESubtokenString: U8
	{
		kSS_F_Localized = 0x01
	};

	// Subtoken when type is `kTT_Speaker`
	enum ESubtokenSpeaker: U8
	{
		kSpk_Normal
	};

	// Subtoken when type is `kTT_Dialogue`
	enum ESubtokenDialogue: U8
	{
		// 「text 1」
		kMsg_Text1 = 0x00, // 00000000
		// 『text 2』
		kMsg_Text2 = 0x01, // 00000001
		// （text 3）
		kMsg_Text3 = 0x02, // 00000010
		// ｛text 4 (shown as narrative)｝
		kMsg_Text4 = 0x03, // 00000011

		kMsg_TextMask = 0x03, // 00000011

		// R attribute -- pause text progression; clear text window
		kMsg_AttribR = 0x04, // 00000100
		// P attribute -- pause text progression; don't clear window
		kMsg_AttribP = 0x08, // 00001000
		// B attribute -- break text (progress), don't clear window
		kMsg_AttribB = 0x0C, // 00001100

		kMsg_AttribMask = 0x0C // 00001100
	};

	// Subtoken when type is `kTT_Comment`
	enum ESubtokenComment: U8
	{
		// Normal comment -- nothing special about it
		kSC_NormalStyle = 0x00,
		// Documentation comment (e.g., "///")
		kSC_DocStyle    = 0x01,
		// Internal testing comment (i.e., "//::" followed by a sequence of non-whitespace then ":")
		kSC_TestStyle   = 0x02,

		kSC_StyleMask   = 0x07,

		// Comment is a block comment
		kSC_F_Block = 0x80
	};

#pragma pack(push,1)
	// A single token
	struct SToken
	{
		enum:U32
		{
			AF_StartsLine  = 0x80000000,

			AF_TypeMask    = 0x7F000000,
			AF_OffsetMask  = 0x00FFFFFF,
			AF_TypeShift   = 24,
			AF_OffsetShift = 0,

			BF_FlagsMask   = 0xFF000000,
			BF_SourceMask  = 0x00FFF000,
			BF_LengthMask  = 0x00000FFF,
			BF_FlagsShift  = 24,
			BF_SourceShift = 12,
			BF_LengthShift = 0
		};

		// Encoding of the token
		//
		// A 31[LTTTTTTT OOOOOOOO OOOOOOOO OOOOOOOO]0
		// -- L: line start flag (0 = continues; 1 = starts)
		// -- T: token type
		// -- O: offset from beginning of source, in bytes
		//
		// B 31[FFFFFFFF SSSSSSSS SSSSNNNN NNNNNNNN]0
		// -- F: flags (depends on token type)
		// -- S: source index
		// -- N: length of token, in bytes
		U32 fieldA, fieldB;

		// Optional value (encoded based on `type` and `flags`)
		union
		{
			// Integer value encoded as 64-bits regardless of exact type
			U64    i;
			// Floating-point value encoded as 64-bits regardless of exact type
			F64    f;
			// Index into `stringLiterals` -- all strings are `NUL`-terminated
			UPtr   s;
			// Symbol pointer
			Ident *p;
		} value;

		// Reset everything to defaults
		inline SToken &reset()
		{
			fieldA = 0;
			fieldB = 0;
			value.i = 0;
			return *this;
		}
		// Determine whether this is in a completely reset/empty state
		inline Bool isReset() const
		{
			return fieldA == 0 && fieldB == 0 && value.i == 0;
		}

		// Set the token type
		inline SToken &setType( ETokenType t )
		{
			fieldA &= ~AF_TypeMask;
			fieldA |= ( U32( t ) << AF_TypeShift ) & AF_TypeMask;
			return *this;
		}
		// Retrieve the token type
		inline ETokenType getType() const
		{
			return ETokenType( ( fieldA & AF_TypeMask ) >> AF_TypeShift );
		}
		// Check for a given token type
		inline Bool is( ETokenType t ) const
		{
			return getType() == t;
		}
		inline Bool isNot( ETokenType t ) const
		{
			return getType() != t;
		}

		// Check for any of the given token types
		inline Bool isAny( ETokenType t ) const
		{
			return is( t );
		}
		template< class... tArgs >
		inline Bool isAny( ETokenType t, ETokenType t2, tArgs... args ) const
		{
			return is( t ) || isAny( t2, args... );
		}
		template< class... tArgs >
		inline Bool isNot( ETokenType t, tArgs... args ) const
		{
			return !isAny( t, args... );
		}

		// Set the type-specific flags for the token
		inline SToken &setFlags( U8 flags, U8 mask = 0xFF )
		{
			const U8 m = mask & ( BF_FlagsMask >> BF_FlagsShift );
			fieldB &= ( ~BF_FlagsMask ) ^ ( U32( ~m ) << BF_FlagsShift );
			fieldB |= U32( flags & m ) << BF_FlagsShift;
			return *this;
		}
		// Add type-specific flags for the token (or'd together)
		inline SToken &addFlags( U8 flags )
		{
			fieldB |= ( U32( flags ) << BF_FlagsShift ) & BF_FlagsMask;
			return *this;
		}
		// Remove some type-specific flags from the token
		inline SToken &removeFlags( U8 flags )
		{
			fieldB &= ~( ( U32( flags ) << BF_FlagsShift ) & BF_FlagsMask );
			return *this;
		}
		// Retrieve the current set of type-specific flags of the token
		inline U8 getFlags( U8 mask = 0xFF) const
		{
			return U8( fieldB >> BF_FlagsShift ) & mask;
		}
		// Check whether all of the given type-specific flags are set
		inline Bool allFlags( U8 flags ) const
		{
			return ( getFlags() & flags ) == flags;
		}
		// Check whether any of the given type-specific flags are set
		inline Bool anyFlags( U8 flags ) const
		{
			return ( getFlags() & flags ) != 0;
		}

		// Mark the token as starting a line
		inline SToken &setStartLine()
		{
			fieldA |= AF_StartsLine;
			return *this;
		}
		// Mark the token as continuing the current line
		inline SToken &setContinueLine()
		{
			fieldA &= ~AF_StartsLine;
			return *this;
		}
		// Check whether this token is the first token of the line
		inline Bool isStartingLine() const
		{
			return ( fieldA & AF_StartsLine ) != 0;
		}
		inline Bool isContinuingLine() const
		{
			return ( fieldA & AF_StartsLine ) == 0;
		}

		// Set the offset and length of this token
		inline SToken &setRange( UPtr off, UPtr len )
		{
			return setOffset( off ).setLength( len );
		}
		// Set the offset and length of this token from a given base string and slice
		inline SToken &setRange( const MutStr &base, const Str &slice )
		{
			AX_ASSERT_MSG( base.view().hasSubstring( slice ), "String slice not from given base" );

			const char *const s = base.pointer();
			const char *const p = slice.pointer();

			const UPtr off = UPtr( p - s );
			const UPtr len = slice.len();

			return setOffset( off ).setLength( len );
		}

		// Set the offset of this token (must fit within 24-bits; i.e., is < 0x01000000)
		inline SToken &setOffset( UPtr off )
		{
			AX_ASSERT_MSG( off < UPtr( AF_OffsetMask )>>AF_OffsetShift, "Offset is too large (not 24-bit)" );

			fieldA &= ~AF_OffsetMask;
			fieldA |= U32( off << AF_OffsetShift ) & AF_OffsetMask;

			return *this;
		}
		// Set the length of this token (must fit within 12-bits; i.e., is < 0x1000)
		inline SToken &setLength( UPtr len )
		{
			AX_ASSERT_MSG( len < UPtr( BF_LengthMask )>>BF_LengthShift, "Length is too large (not 12-bit)" );

			fieldB &= ~BF_LengthMask;
			fieldB |= U32( len << BF_LengthShift ) & BF_LengthMask;

			return *this;
		}
		// Set the source index of this token (must fit within 12-bits; i.e., is < 0x1000)
		inline SToken &setSourceIndex( UPtr idx )
		{
			AX_ASSERT_MSG( idx < UPtr( BF_SourceMask )>>BF_SourceShift, "Source index is too large (not 12-bit)" );

			fieldB &= ~BF_SourceMask;
			fieldB |= U32( idx << BF_SourceShift ) & BF_SourceMask;

			return *this;
		}

		// Retrieve the offset (in bytes) of this token within its source
		inline UPtr getOffset() const
		{
			return UPtr( fieldA & AF_OffsetMask ) >> AF_OffsetShift;
		}
		// Retrieve the length of this token in bytes
		inline UPtr getLength() const
		{
			return UPtr( fieldB & BF_LengthMask ) >> BF_LengthShift;
		}
		// Retrieve the source index of this token (the index number of the file it's in)
		inline UPtr getSourceIndex() const
		{
			return UPtr( fieldB & BF_SourceMask ) >> BF_SourceShift;
		}

		// Check if this is an EOF token
		inline Bool isEOF() const
		{
			return getType() == kTT_None && getFlags() == kTN_End;
		}
		// Check if this is an error token
		inline Bool isError() const
		{
			return getType() == kTT_None && getFlags() == kTN_Error;
		}
	};
#pragma pack(pop)

	DOLL_FUNC Bool DOLL_API scr_tokenTypeToString( Str &dst, ETokenType src );
	DOLL_FUNC Bool DOLL_API scr_stringToTokenType( ETokenType &dst, Str src );

	inline Str DOLL_API scr_tokenTypeToString( ETokenType src )
	{
		Str r;
		return scr_tokenTypeToString( r, src ), r;
	}
	inline ETokenType DOLL_API scr_stringToTokenType( Str src )
	{
		ETokenType t = kTT_None;
		return scr_stringToTokenType( t, src ), t;
	}

}}
