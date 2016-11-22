//============================================================================//
/// \file  doll-script-diagnostics_ids.def.hpp
/// \brief This file contains the definitions of all the diagnostics used by
///        Doll's scripting system.
///
/// \note This file is intentionally allowed to be included multiple times.
//============================================================================//

#if !defined(DOLL_SCRIPT__DIAG) && ( !defined(DOLL_SCRIPT_ERROR) || !defined(DOLL_SCRIPT_WARNING) )
# error Need DOLL_SCRIPT__DIAG defined, or DOLL_SCRIPT_ERROR and DOLL_SCRIPT_WARNING defined.
#endif

#ifndef DOLL_SCRIPT__DIAG
# define DOLL_SCRIPT__DIAG(Sev_,Id_,Cat_,Opt_,Msg_,ArgTys_)
#endif

#ifndef DOLL_SCRIPT_ERROR
# define DOLL_SCRIPT_ERROR(Id_,Cat_,Opt_,Msg_,ArgTys_) \
	DOLL_SCRIPT__DIAG(Error,Id_,Cat_,Opt_,Msg_,ArgTys_)
# define DOLL_SCRIPT__DEFINED_ERROR 1
#endif

#ifndef DOLL_SCRIPT_WARNING
# define DOLL_SCRIPT_WARNING(Id_,Cat_,Opt_,Msg_,ArgTys_) \
	DOLL_SCRIPT__DIAG(Warning,Id_,Cat_,Opt_,Msg_,ArgTys_)
# define DOLL_SCRIPT__DEFINED_WARNING 1
#endif

// -- Basic ----------------------------------------------------------------- //

DOLL_SCRIPT_ERROR(InternalStrLitNoMem, Basic, None,
	"[internal] Insufficient memory to remember string literal", ())
DOLL_SCRIPT_ERROR(InternalStrEscUnicodeConvertFailure, Basic, None,
	"[internal] Unexpected error converting UTF-32 codepoint %0 to UTF-8", (U32))

// -- Lexer ----------------------------------------------------------------- //

DOLL_SCRIPT_WARNING(InvalidCharacter, Lexer, None,
	"Invalid character %0", (U32))
DOLL_SCRIPT_WARNING(UnrecognizedToken, Lexer, None,
	"Unrecognized token, skipping remainder of line", ())

DOLL_SCRIPT_ERROR(UnterminatedBlockComment, Lexer, None,
	"Block comment opened by /* has no matching */", ())
DOLL_SCRIPT_ERROR(UnexpectedBlockCommentEnd, Lexer, None,
	"Unexpected block comment closer '*/' found", ())

DOLL_SCRIPT_ERROR(UnbalancedLParen, Lexer, None,
	"Unbalanced parentheses, '(' has no matching ')'", ())
DOLL_SCRIPT_ERROR(UnbalancedRParen, Lexer, None,
	"Unbalanced parentheses, '(' is missing for ')'", ())
DOLL_SCRIPT_ERROR(UnbalancedLBrack, Lexer, None,
	"Unbalanced brackets, '[' has no matching ']'", ())
DOLL_SCRIPT_ERROR(UnbalancedRBrack, Lexer, None,
	"Unbalanced brackets, '[' is missing for ']'", ())
DOLL_SCRIPT_ERROR(UnbalancedLBrace, Lexer, None,
	"Unbalanced braces, '{' has no matching '}'", ())
DOLL_SCRIPT_ERROR(UnbalancedRBrace, Lexer, None,
	"Unbalanced braces, '{' has no matching '}'", ())

DOLL_SCRIPT_ERROR(UnterminatedString, Lexer, None,
	"String literal has no matching end quote", ())

DOLL_SCRIPT_ERROR(UnexpectedSpecialIdentChar, Lexer, None,
	"Unexpected character %0 in special identifier, wanted %1", (U32, U32))

DOLL_SCRIPT_ERROR(ExpectedRadixRParen, Lexer, None,
	"Expected ')' in #()-style radix specification", ())
DOLL_SCRIPT_ERROR(UnknownRadixChar, Lexer, None,
	"Unknown radix character %0 in #()-style radix specification", (U32))
DOLL_SCRIPT_ERROR(TooManyDotsInNumber, Lexer, None,
	"Too many '.'s in number", ())
DOLL_SCRIPT_ERROR(TooManyExpsInNumber, Lexer, None,
	"Too many exponents in number", ())
DOLL_SCRIPT_ERROR(UnknownTypeSuffix, Lexer, None,
	"Unknown type suffix %0", (Str))
DOLL_SCRIPT_ERROR(FloatLitHasIntType, Lexer, None,
	"Floating-point literal has explicit integer type", ())

DOLL_SCRIPT_ERROR(StrEscExpectedHex, Lexer, None,
	"Expected hexadecimal digit for \\xHH escape sequence in string literal", ())
DOLL_SCRIPT_ERROR(StrEscUnicodeNoLBrace, Lexer, None,
	"Expected '\\u' escape to have '{' immediately following", ())
DOLL_SCRIPT_ERROR(StrEscUnicodeBadHex, Lexer, None,
	"Expected hexadecimal digit (0-9a-fA-F) or '}' for '\\u' escape", ())
DOLL_SCRIPT_ERROR(StrEscUnicodeBadCP, Lexer, None,
	"Invalid codepoint %0 from '\\u' escape sequence", (U64))
DOLL_SCRIPT_ERROR(StrEscUnicodeEarlyTerm, Lexer, None,
	"String terminated early in '\\u' escape", ())

DOLL_SCRIPT_ERROR(AlreadyInMenu, Lexer, None,
	"Already in a menu; cannot nest menus", ())
DOLL_SCRIPT_ERROR(ExpectedMenu, Lexer, None,
	"Expected '{' for menu choices", ())

// -- Testing --------------------------------------------------------------- //

DOLL_SCRIPT_ERROR(TestInvalidType, Testing, None,
	"Unknown test type %0", (Str))
DOLL_SCRIPT_ERROR(TestUnimplementedType, Testing, None,
	"Test type %0 is not yet implemented", (Str))
DOLL_SCRIPT_ERROR(TestUnexpectedEOF, Testing, None,
	"Unexpected end-of-file", ())
DOLL_SCRIPT_WARNING(TestOutstandingDirectives, Testing, None,
	"Outstanding directive: %0", (Str))
DOLL_SCRIPT_ERROR(TestUnknownDirective, Testing, None,
	"Unknown directive: %0", (Str))

DOLL_SCRIPT_ERROR(TestMissingFirstTok, Testing, None,
	"Missing first token", ())
DOLL_SCRIPT_ERROR(TestFirstTokNotTestType, Testing, None,
	"First token is not a test comment (//::TEST-<X>:://)", ())
DOLL_SCRIPT_ERROR(TestFirstTokNotTestForm, Testing, None,
	"First token is not in the form of (//::TEST-<X>:://)", ())

DOLL_SCRIPT_ERROR(TestLexUnknownFlag, Testing, None,
	"Unknown flag %0 for EXPECT-TOKEN", (Str))
DOLL_SCRIPT_ERROR(TestLexUnknownTokTy, Testing, None,
	"Unknown token type %0", (Str))
DOLL_SCRIPT_ERROR(TestLexUnexpectedStartLine, Testing, None,
	"Token is unexpectedly starting the line", ())
DOLL_SCRIPT_ERROR(TestLexUnexpectedContinueLine, Testing, None,
	"Token is unexpectedly continuing the line", ())
DOLL_SCRIPT_ERROR(TestLexTokenMismatch, Testing, None,
	"Expected token type %0, got %1 <%2>", (Str, Str, Str))
DOLL_SCRIPT_ERROR(TestLexLexanMismatch, Testing, None,
	"Expected lexan <%0>, got <%1>", (Str, Str))
DOLL_SCRIPT_ERROR(TestLexParenBalance, Testing, None,
	"Expected parenthesis () balance %0, but had %1", (U64, U64))
DOLL_SCRIPT_ERROR(TestLexBrackBalance, Testing, None,
	"Expected bracket [] balance %0, but had %1", (U64, U64))
DOLL_SCRIPT_ERROR(TestLexBraceBalance, Testing, None,
	"Expected brace {} balance %0, but had %1", (U64, U64))
DOLL_SCRIPT_ERROR(TestLexExpectedEOF, Testing, None,
	"Expected EOF, but got %0 token <%1>", (Str, Str))

// -------------------------------------------------------------------------- //

#ifdef DOLL_SCRIPT__DEFINED_WARNING
# undef DOLL_SCRIPT__DEFINED_WARNING
# undef DOLL_SCRIPT_WARNING
#endif
#ifdef DOLL_SCRIPT__DEFINED_ERROR
# undef DOLL_SCRIPT__DEFINED_ERROR
# undef DOLL_SCRIPT_ERROR
#endif
