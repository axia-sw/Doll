#include "../BuildSettings.hpp"

#include "doll/Script/Lexer.hpp"
#include "doll/Script/Compiler.hpp"
#include "doll/Script/Diagnostics.hpp"

#include "doll/Math/Math.hpp"

namespace doll { namespace script {

	namespace detail
	{

		SToken *selectCommentToken( const LexerOpts &opts, SToken &nextTok, SToken &keepTok )
		{
			switch( opts.comments ) {
			case ELexCommentMode::Ignore:
				break;

			case ELexCommentMode::Emit:
			case ELexCommentMode::EmitTests:
				return &nextTok;

			case ELexCommentMode::Keep:
				return &keepTok;
			}

			return nullptr;
		}

	}

	CLexer::CLexer( Source &src, CDiagnosticEngine &diagEngine, IdentDictionary &dict, CProgramData &progData, const LexerOpts &opts )
	: m_src( src )
	, m_diagEngine( diagEngine )
	, m_progData( progData )
	, m_dict( dict )
	, m_buffer( src.buffer )
	, m_tokStack()
	, m_nextToken()
	, m_keepToken()
	, m_pComment( detail::selectCommentToken( opts, m_nextToken, m_keepToken ) )
	, m_stateFlags( 0 )
	, m_balance()
	, m_nestingLevels()
	, m_opts( opts )
	{
		// Initialize the lexer
		lexNextToken();
	}
	CLexer::CLexer( CCompilerContext &ctx, const LexerOpts &opts )
	: m_src( *ctx.getActiveSource() )
	, m_diagEngine( ctx.getDiagnosticEngine() )
	, m_progData( ctx.getProgramData() )
	, m_dict( ctx.getSymbolMap() )
	, m_buffer( ctx.getActiveSource()->buffer )
	, m_tokStack()
	, m_nextToken()
	, m_keepToken()
	, m_pComment( detail::selectCommentToken( opts, m_nextToken, m_keepToken ) )
	, m_stateFlags( 0 )
	, m_balance()
	, m_nestingLevels()
	, m_opts( opts )
	{
		AX_ASSERT_NOT_NULL( ctx.getActiveSource() );

		// Initialize the lexer
		lexNextToken();
	}
	CLexer::~CLexer()
	{
	}

#if 0 // warning: unused function
	static const SToken *getLastToken( const SSource &src )
	{
		if( !src.pTokenPool || !src.pTokenPool->cTokens ) {
			return nullptr;
		}

		return &src.pTokenPool->tokens[ src.pTokenPool->cTokens - 1 ];
	}
#endif

#if 0 // warning: unused function
	static inline Bool isEmit( ELexCommentMode mode )
	{
		return mode == ELexCommentMode::Emit || mode == ELexCommentMode::EmitTests;
	}
#endif

	Bool CLexer::lex( SToken &dstTok )
	{
		AX_ASSERT_MSG( m_nextToken.isReset() == false, "Lexer not initialized." );

		if( peek( dstTok ) ) {
			if( dstTok.is( kTT_Keyword ) && dstTok.getFlags() == kKW_Goto ) {
				m_stateFlags |= kStateF_AcceptLabel;
			} else if( dstTok.is( kTT_Koe ) || dstTok.is( kTT_Speaker ) ) {
				m_stateFlags |= kStateF_AcceptDlg;
			}

			// FIXME: Check return value
			lexNextToken();
			return true;
		}

		return false;
	}
	Bool CLexer::peek( SToken &dstTok ) const
	{
		dstTok = m_nextToken;
		return !dstTok.isEOF();
	}
	Bool CLexer::skip()
	{
		if( m_nextToken.isEOF() ) {
			return false;
		}

		lexNextToken();
		return true;
	}

	Bool CLexer::lexNextToken()
	{
		// Handle ()/[]/{} balance
		if( m_nextToken.is( kTT_Punctuation ) ) {
			unsigned which = ~0U;
			Bool isClose = false;

			switch( m_nextToken.getFlags() ) {
			case kPn_RParen:
				isClose = true;
			case kPn_LParen:
				which = kBalance_Paren;
				break;

			case kPn_RBracket:
				isClose = true;
			case kPn_LBracket:
				which = kBalance_Brack;
				break;

			case kPn_RBrace:
				isClose = true;
				if( m_stateFlags & kStateF_InMenu ) {
					if( getMenuBalance() == m_nestingLevels[ kBalance_Brace ] ) {
						m_stateFlags &= ~kStateF_InMenu;
					}
				} else if( m_stateFlags & kStateF_AwaitMenu ) {
					const SourceLoc loc( getLocFromToken( m_nextToken ) );
					m_diagEngine.diagnose( loc, Diag::ExpectedMenu );
					return false;
				}
			case kPn_LBrace:
				which = kBalance_Brace;
				if( !isClose && ( m_stateFlags & kStateF_AwaitMenu ) ) {
					m_stateFlags &= ~kStateF_AwaitMenu;
					m_stateFlags |= kStateF_InMenu;
					setMenuBalance( m_nestingLevels[ kBalance_Brace ] + 1 );
				}
				break;
			}

			if( which < 3 ) {
				if( !isClose ) {
					if( !AX_VERIFY_MEMORY( m_balance.push( which ) ) ) {
						return false;
					}

					++m_nestingLevels[ which ];
				} else {
					if( !m_balance.isTop( which ) ) {
						const SourceLoc loc( getLocFromToken( m_nextToken ) );

						switch( which ) {
						case kBalance_Paren:
							m_diagEngine.diagnose( loc, Diag::UnbalancedRParen );
							break;

						case kBalance_Brack:
							m_diagEngine.diagnose( loc, Diag::UnbalancedRBrack );
							break;

						case kBalance_Brace:
							m_diagEngine.diagnose( loc, Diag::UnbalancedRBrace );
							break;
						}

						return false;
					}

					m_balance.pop();
					--m_nestingLevels[ which ];
				}
			}
		}

		// Check the unread tokens
		if( m_tokStack.isUsed() ) {
			m_nextToken = m_tokStack.popLast();
			return true;
		}

		// Enable accepting labels if the prior token was a "goto" keyword
		if( m_nextToken.is( kTT_Keyword ) ) {
			if( m_nextToken.getFlags() == kKW_Goto ) {
				m_stateFlags |= kStateF_AcceptLabel;
			} else if( m_nextToken.getFlags() == kKW_Menu ) {
				if( m_stateFlags & ( kStateF_AwaitMenu | kStateF_InMenu ) ) {
					const SourceLoc loc( getLocFromToken( m_nextToken ) );
					m_diagEngine.diagnose( loc, Diag::AlreadyInMenu );
					return false;
				}

				m_stateFlags |= kStateF_AwaitMenu;
			}
		// Enable accepting messages if the prior token was a "speaker" token
		} else if( m_nextToken.is( kTT_Speaker ) ) {
			m_stateFlags |= kStateF_AcceptDlg;
		}

		// Prepare the next token
		m_nextToken.reset();
		m_nextToken.setSourceIndex( m_src.index );

		// Ignore all whitespace
		if( skipWhitespaceAndComments() ) {
			if( m_nextToken.is( kTT_Comment ) || m_nextToken.is( kTT_Blank ) ) {
				return true;
			}
		}

		// End of file?
		if( m_buffer.isEmpty() ) {
			// Setup the EOF token
			m_nextToken.setType( kTT_None );
			m_nextToken.setFlags( kTN_End );

			m_nextToken.setOffset( m_src.buffer.len() );

			// Done
			return false;
		}

		// Setup the token with defaults
		m_nextToken.setOffset( UPtr( m_buffer.get() - m_src.buffer.get() ) );

		// Whether the token is starting the line or not
		const Bool bCrossedLine = m_nextToken.isStartingLine();

		// Whether we're accepting labels
		const Bool bAcceptLabel = bCrossedLine || ( m_stateFlags & kStateF_AcceptLabel );
		// Whether dialog is accepted
		const Bool bAcceptDlg   = bCrossedLine || ( m_stateFlags & kStateF_AcceptDlg );

		// Reset the state flags
		m_stateFlags &= ~( kStateF_AcceptLabel | kStateF_AcceptDlg );

		// Check for a string literal or dialog
		if( readString( m_nextToken, bAcceptDlg ) ) {
			return true;
		}

		// Check for a numeric literal
		if( readNumber( m_nextToken ) ) {
			return true;
		}

		// Check for an identifier (this should be done after checking for literals!)
		if( readIdent( m_nextToken, bAcceptLabel ) ) {
			return true;
		}

		// Check for punctuation (should always happen after checking for an identifier!)
		if( readPunct( m_nextToken ) ) {
			return true;
		}

		// Error
		m_diagEngine.diagnose( m_nextToken, Diag::UnrecognizedToken );
		m_buffer = m_buffer.skip();

		// Setup the error token
		m_nextToken.setType( kTT_None );
		m_nextToken.setFlags( kTN_Error );
		m_nextToken.setLength( 1 );

		// Done
		return true;
	}

	Bool CLexer::skipWhitespaceAndComments()
	{
		const char *const pOrg = m_buffer.get();

		if( m_pComment != nullptr ) {
			m_pComment->reset();
		}

		const Bool wasBlank = ( m_stateFlags & kStateF_WasBlank ) != 0;
		m_stateFlags &= ~kStateF_WasBlank;

		Str s = m_buffer;
		for(;;) {
			s = m_buffer.skipWhitespace();

			const Str base( s );

			const SPtr lineIndex =
				( wasBlank || m_src.buffer.get() == m_buffer.get() ) ? 0
				: Str( pOrg, base.get() ).findAny( "\r\n" );
			const Bool bStartsLine = lineIndex != -1;

			if( bStartsLine ) {
				m_nextToken.setStartLine();

				if( isAtMenuChoices() && !wasBlank ) {
					const Str blankSpace = Str( m_buffer.get(), s.get() );
					Str secondBlank = blankSpace.searchAny( "\r\n" );
					secondBlank.checkByte( '\r' );
					secondBlank.checkByte( '\n' );
					const SPtr secondLineIndex = secondBlank.findAny( "\r\n" );
					const Bool hasBlankLine = secondLineIndex != -1;

					if( hasBlankLine ) {
						m_buffer = s;

						m_nextToken.setType( kTT_Blank );
						m_nextToken.setOffset( blankSpace.get() - m_src.buffer.get() );
						m_nextToken.setLength( blankSpace.len() );
						m_nextToken.setStartLine();

						m_stateFlags |= kStateF_WasBlank;
						return true;
					}
				}
			}

			if( *s != '/' || ( s[ 1 ] != '/' && s[ 1 ] != '*' ) ) {
				m_buffer = s;
				break;
			}

			U8 flags = 0;
			if( s.startsWith( "/*" ) ) {
				const char *const pOpenPtr = s.get();
				s = s.skip( 2 );

				flags |= kSC_F_Block;
				if( *s == '!' || ( *s == '*' && U8(s[1]) <= ' ' ) ) {
					flags = ( flags & ~kSC_StyleMask ) | kSC_DocStyle;
				} else {
					flags = ( flags & ~kSC_StyleMask ) | kSC_NormalStyle;
				}

				UPtr cNesting = 1;
				while( cNesting > 0 && !s.isEmpty() ) {
					if( s.startsWith( "/*" ) ) {
						++cNesting;
						s = s.skip( 2 );
					} else if( s.startsWith( "*/" ) ) {
						--cNesting;
						s = s.skip( 2 );
					} else {
						s = s.skip();
					}
				}
				if( cNesting > 0 ) {
					m_diagEngine.diagnose( getLocFromPtr( pOpenPtr ), Diag::UnterminatedBlockComment );
				}
			} else {
				s = s.skip( 2 );
				if( s.startsWith( "::" ) ) {
					flags = ( flags & ~kSC_StyleMask ) | kSC_TestStyle;
				} else if( *s == '!' || ( *s == '/' && U8(s[1]) <= ' ' ) ) {
					flags = ( flags & ~kSC_StyleMask ) | kSC_DocStyle;
				} else {
					flags = ( flags & ~kSC_StyleMask ) | kSC_NormalStyle;
				}

				s = s.searchAny( "\r\n" );
			}

			if( m_pComment != nullptr && ( m_opts.comments != ELexCommentMode::EmitTests || ( flags & kSC_StyleMask ) == kSC_TestStyle ) ) {
				if( m_pComment->isNot( kTT_Comment ) ) {
					m_pComment->setType( kTT_Comment );

					m_pComment->setSourceIndex( m_src.index );
					m_pComment->setOffset( UPtr( base.get() - m_src.buffer.get() ) );

					if( bStartsLine ) {
						m_pComment->setStartLine();
					}
				}

				m_pComment->setLength( m_pComment->getLength() + ( base.len() - s.len() ) );
				m_pComment->setFlags( flags );

				if( m_pComment == &m_nextToken ) {
					m_buffer = s;
					return true;
				}
			}

			m_buffer = s;
			continue;
		}

		return pOrg != m_buffer.get();
	}
	Bool CLexer::readIdent( SToken &dstTok, Bool acceptLabel )
	{
		enum TYPE { T_NONE, T_NM, T_TY, T_CR, T_LB, T_SY, T_CF, T_TG, T_AP };

		Str s = m_buffer;
		Str sym;

		TYPE t = T_NONE;
		Bool allowSpecial = false;
		char chExpect = '\0';

		if( s.checkByte( '@' ) ) {
			t = T_CR;
			allowSpecial = true;
		} else if( acceptLabel && s.checkByte( '*' ) ) {
			t = T_LB;
			allowSpecial = true;
		} else if( s.checkByte( '$' ) ) {
			if( s.checkByte( '<' ) ) {
				chExpect = '>';
				t = T_TG;
			} else {
				t = T_SY;
			}
			allowSpecial = true;
		} else if( s.checkByte( '#' ) ) {
			if( s.checkByte( '<' ) ) {
				chExpect = '>';
				t = T_AP;
			} else {
				t = T_CF;
			}
			allowSpecial = true;
		} else {
			sym = s;

			while( s.checkByte( '_' ) ) {
			}

			if( s.checkByteRange( 'a', 'z' ) ) {
				t = T_NM;
			} else if( s.checkByteRange( 'A', 'Z' ) ) {
				t = T_TY;
			}

			allowSpecial = false;
		}

		if( t == T_NONE ) {
			return false;
		}

		if( allowSpecial && !chExpect ) {
			if( s.checkByte( '{' ) ) {
				chExpect = '}';
			}
		}

		if( sym.isEmpty() ) {
			sym = s;
		}

		if( allowSpecial ) {
			while( s.isUsed() && !s.checkByte( chExpect ) ) {
				const Str p( s );

				axstr_utf32_t chGot = s.readChar();
				if( !axstr_isname( chGot ) ) {
					if( chExpect != '\0' ) {
						m_diagEngine.diagnose( getLocFromPtr( p ), Diag::UnexpectedSpecialIdentChar, U32( chGot ), U32( chExpect ) );
					}

					s = p;
					break;
				}
			}
		} else {
			while( s.isUsed() ) {
				const unsigned char ch = ( unsigned char )*s.get();

				if( axstr_isalnum( ch ) || ch == '_' ) {
					s = s.skip();
				} else {
					break;
				}
			}
		}

		switch( t ) {
		case T_NONE:
			AX_EXPECT_MSG( false, "Unreachable code - T_NONE" );
			break;

		case T_NM:
			dstTok.setType( kTT_Name );
			break;

		case T_TY:
			dstTok.setType( kTT_Type );
			break;

		case T_LB:
			dstTok.setType( kTT_Label );
			break;

		case T_CR:
			dstTok.setType( kTT_CharRef );
			break;

		case T_SY:
			dstTok.setType( kTT_SystemVar );
			break;

		case T_CF:
			dstTok.setType( kTT_ConfigVar );
			break;

		case T_TG:
			dstTok.setType( kTT_TagRef );
			break;

		case T_AP:
			dstTok.setType( kTT_ProgTagRef );
			break;
		}

		dstTok.setLength( UPtr( s.get() - m_buffer.get() ) );
		sym = sym.left( axstr_ptrdiff_t( s.get() - sym.get() ) );

		//axpf( ":::: <<`%.*s`>> ::::\n", sym.lenInt(), sym.get() );

		IdentDictionary::SEntry *const pEntry = m_dict.lookup( sym );
		if( !AX_VERIFY_MEMORY( pEntry ) ) {
			return false;
		}

		if( pEntry->pData != nullptr ) {
			if( t == T_NM ) {
				if( pEntry->pData->isKeyword ) {
					dstTok.setType( kTT_Keyword );
					dstTok.setFlags( pEntry->pData->keyword );
				}
			} else if( t == T_TY ) {
				// FIXME: Check for a standard type
			}
		} else {
			if( !AX_VERIFY_MEMORY( pEntry->pData = DOLL__SCRIPTOBJ_NEW Ident( m_src.getContext() ) ) ) {
				return false;
			}
		}

		if( pEntry->pData->name.isEmpty() ) {
			pEntry->pData->name = sym;
		}

		dstTok.value.p = pEntry->pData;
		m_buffer = s;

		return true;
	}
	Bool CLexer::readPunct( SToken &dstTok )
	{
		Str s( m_buffer );

		U8 subtoken;

		if( s.checkByte( '(' ) ) {
			subtoken = kPn_LParen;
		} else if( s.checkByte( ')' ) ) {
			subtoken = kPn_RParen;
		} else if( s.checkByte( '[' ) ) {
			subtoken = kPn_LBracket;
		} else if( s.checkByte( ']' ) ) {
			subtoken = kPn_RBracket;
		} else if( s.checkByte( '{' ) ) {
			subtoken = kPn_LBrace;
		} else if( s.checkByte( '}' ) ) {
			subtoken = kPn_RBrace;
		} else if( s.checkByte( ',' ) ) {
			subtoken = kPn_Comma;
		} else if( s.checkByte( ';' ) ) {
			subtoken = kPn_Semicolon;
		} else if( s.checkByte( ':' ) ) {
			// ":="
			if( s.checkByte( '=' ) ) {
				subtoken = kPn_AutoAssign;
			} else {
				subtoken = kPn_Colon;
			}
		} else if( s.checkByte( '.' ) ) {
			// "..."
			if( s.startsWith( ".." ) ) {
				s = s.skip( 2 );
				subtoken = kPn_Ellipsis;
			// "..<"
			} else if( s.startsWith( ".<" ) ) {
				s = s.skip( 2 );
				subtoken = kPn_HalfOpenRange;
			} else {
				subtoken = kPn_Dot;
			}
		} else if( s.checkByte( '?' ) ) {
			// "?="
			if( s.checkByte( '=' ) ) {
				subtoken = kPn_OptionalAssign;
			// "??"
			} else if( s.checkByte( '?' ) ) {
				subtoken = kPn_NilCoalesce;
			} else {
				subtoken = kPn_Conditional;
			}
		} else if( s.checkByte( '=' ) ) {
			// "=>"
			if( s.checkByte( '>' ) ) {
				subtoken = kPn_FuncDef;
			// "=="
			} else if( s.checkByte( '=' ) ) {
				// "==="
				if( s.checkByte( '=' ) ) {
					subtoken = kPn_IdEq;
				} else {
					subtoken = kPn_Eq;
				}
			} else {
				subtoken = kPn_Assign;
			}
		} else if( s.checkByte( '+' ) ) {
			// "+="
			if( s.checkByte( '=' ) ) {
				subtoken = kPn_AddAssign;
			// "++"
			} else if( s.checkByte( '+' ) ) {
				subtoken = kPn_Inc;
			} else {
				subtoken = kPn_Add;
			}
		} else if( s.checkByte( '-' ) ) {
			// "-="
			if( s.checkByte( '=' ) ) {
				subtoken = kPn_SubAssign;
			// "--"
			} else if( s.checkByte( '-' ) ) {
				subtoken = kPn_Dec;
			} else {
				subtoken = kPn_Sub;
			}
		} else if( s.checkByte( '*' ) ) {
			// "*="
			if( s.checkByte( '=' ) ) {
				subtoken = kPn_MulAssign;
			} else {
				subtoken = kPn_Mul;
			}
		} else if( s.checkByte( '/' ) ) {
			// "/="
			if( s.checkByte( '=' ) ) {
				subtoken = kPn_DivAssign;
			} else {
				subtoken = kPn_Div;
			}
		} else if( s.checkByte( '%' ) ) {
			// "%="
			if( s.checkByte( '=' ) ) {
				subtoken = kPn_ModAssign;
			} else {
				subtoken = kPn_Mod;
			}
		} else if( s.checkByte( '~' ) ) {
			// "~="
			if( s.checkByte( '=' ) ) {
				subtoken = kPn_ApxEq;
			} else {
				subtoken = kPn_BitNot;
			}
		} else if( s.checkByte( '|' ) ) {
			// "||"
			if( s.checkByte( '|' ) ) {
				// "||="
				if( s.checkByte( '=' ) ) {
					subtoken = kPn_RelOrAssign;
				} else {
					subtoken = kPn_RelOr;
				}
			// "|="
			} else if( s.checkByte( '=' ) ) {
				subtoken = kPn_BitOrAssign;
			} else {
				subtoken = kPn_BitOr;
			}
		} else if( s.checkByte( '&' ) ) {
			// "&&"
			if( s.checkByte( '&' ) ) {
				// "&&="
				if( s.checkByte( '=' ) ) {
					subtoken = kPn_RelAndAssign;
				} else {
					subtoken = kPn_RelAnd;
				}
			// "&="
			} else if( s.checkByte( '=' ) ) {
				subtoken = kPn_BitAndAssign;
			// "&+"
			} else if( s.checkByte( '+' ) ) {
				// "&+="
				if( s.checkByte( '=' ) ) {
					subtoken = kPn_AddOFAssign;
				} else {
					subtoken = kPn_AddOF;
				}
			// "&-"
			} else if( s.checkByte( '-' ) ) {
				// "&-="
				if( s.checkByte( '=' ) ) {
					subtoken = kPn_SubOFAssign;
				} else {
					subtoken = kPn_SubOF;
				}
			// "&*"
			} else if( s.checkByte( '*' ) ) {
				// "&*="
				if( s.checkByte( '=' ) ) {
					subtoken = kPn_MulOFAssign;
				} else {
					subtoken = kPn_MulOF;
				}
			} else {
				subtoken = kPn_BitAnd;
			}
		} else if( s.checkByte( '^' ) ) {
			// "^="
			if( s.checkByte( '=' ) ) {
				subtoken = kPn_BitXorAssign;
			} else {
				subtoken = kPn_BitXor;
			}
		} else if( s.checkByte( '<' ) ) {
			// "<="
			if( s.checkByte( '=' ) ) {
				// "<=>"
				if( s.checkByte( '>' ) ) {
					subtoken = kPn_Swap;
				} else {
					subtoken = kPn_LE;
				}
			// "<<"
			} else if( s.checkByte( '<' ) ) {
				// "<<="
				if( s.checkByte( '=' ) ) {
					subtoken = kPn_LShAssign;
				} else {
					subtoken = kPn_LSh;
				}
			} else {
				subtoken = kPn_Lt;
			}
		} else if( s.checkByte( '>' ) ) {
			// ">="
			if( s.checkByte( '=' ) ) {
				subtoken = kPn_GE;
			// ">>"
			} else if( s.checkByte( '>' ) ) {
				// ">>="
				if( s.checkByte( '=' ) ) {
					subtoken = kPn_RShAssign;
				} else {
					subtoken = kPn_RSh;
				}
			} else {
				subtoken = kPn_Gt;
			}
		} else if( s.checkByte( '!' ) ) {
			// "!="
			if( s.checkByte( '=' ) ) {
				// "!=="
				if( s.checkByte( '=' ) ) {
					subtoken = kPn_IdNE;
				} else {
					subtoken = kPn_NE;
				}
			} else {
				subtoken = kPn_RelNot;
			}
		} else {
			return false;
		}

		dstTok.setType( kTT_Punctuation );
		dstTok.setFlags( subtoken );

		dstTok.setLength( UPtr( s.get() - m_buffer.get() ) );
		m_buffer = s;

		return true;
	}
	Bool CLexer::readNumber( SToken &dstTok )
	{
		Str s( m_buffer );

		const char *pRadixCharLoc = nullptr;
		char chRadixChar = '\0';
		U32 uRadix = 10;

		if( s.startsWith( "#(" ) || s.startsWith( "0(" ) ) {
			const char *const pOpenRadix = s.get();

			s.skip( 2 );
			pRadixCharLoc = s.get();
			chRadixChar = ( char )axstr_tolower( s.readByte() );
			if( !s.checkByte( ')' ) ) {
				m_diagEngine.diagnose( getLocFromPtr( pOpenRadix ), Diag::ExpectedRadixRParen );
				return false;
			}
		} else if( s.startsWith( '0' ) ) {
			pRadixCharLoc = s.get() + 1;
			const char c = ( char )axstr_tolower( s[ 1 ] );
			if( c >= 'a' && c <= 'z' ) {
				chRadixChar = c;
				s = s.skip( 2 );
			}
		}

		if( chRadixChar != '\0' ) {
			if( chRadixChar == 'x' || chRadixChar == 'h' ) {
				uRadix = 16;
			} else if( chRadixChar == 'c' || chRadixChar == 'o' ) {
				uRadix = 8;
			} else if( chRadixChar == 'b' ) {
				uRadix = 2;
			} else {
				m_diagEngine.diagnose( getLocFromPtr( pRadixCharLoc ), Diag::UnknownRadixChar, U32( chRadixChar ) );
				return false;
			}
		}

		const char chA[ 2 ] = { '0', char('0' + ( ( uRadix < 10 ? uRadix : 10 ) - 1 ) ) };
		const char chB[ 2 ] = { 'a', char('a' + ( uRadix > 10 ? S32( uRadix - 10 - 1 ) : -1 ) ) };
		const char chC[ 2 ] = { 'A', char('A' + ( uRadix > 10 ? S32( uRadix - 10 - 1 ) : -1 ) ) };

		const char ch = s[ 0 ];
		if( !( ( ch >= chA[ 0 ] && ch <= chA[ 1 ] ) || ( ch >= chB[ 0 ] && ch <= chB[ 1 ] ) || ( ch >= chC[ 0 ] && ch <= chC[ 1 ] ) ) ) {
			return false;
		}

		enum STAGE { WHOLE, FRACT, EXP };
		enum ESIGN { POS, NEG, UNKNOWN };

		STAGE stage = WHOLE;
		ESIGN esign = UNKNOWN;

		U64 parts[ 3 ] = { 0, 0, 0 };
		while( s.isUsed() ) {
			const char c = s[ 0 ];

			S32 digit = -1;
			if( c >= chA[ 0 ] && c <= chA[ 1 ] ) {
				digit = S32( c - chA[ 0 ] );
			} else if( c >= chB[ 0 ] && c <= chB[ 1 ] ) {
				digit = S32( c - chB[ 0 ] ) + 10;
			} else if( c >= chC[ 0 ] && c <= chC[ 1 ] ) {
				digit = S32( c - chC[ 0 ] ) + 10;
			} else if( c == '.' ) {
				if( stage == WHOLE ) {
					stage = FRACT;
				} else {
					m_diagEngine.diagnose( getLocFromPtr( s ), Diag::TooManyDotsInNumber );
				}
			} else if( c == 'e' || c == 'E' || c == 'p' || c == 'P' ) {
				if( stage != EXP ) {
					stage = EXP;
				} else {
					m_diagEngine.diagnose( getLocFromPtr( s ), Diag::TooManyExpsInNumber );
				}
			} else if( stage == EXP && esign == UNKNOWN && ( c == '-' || c == '+' ) ) {
				if( c == '-' ) {
					esign = NEG;
				} else /* c == '+' */ {
					esign = POS;
				}
			} else {
				break;
			}

			s = s.skip();

			if( digit != -1 ) {
				while( s.checkByte( '_' ) ) {
				}

				if( stage == FRACT ) {
					// Encode fractional part with '+ 1' so 0s are retained
					if( parts[ stage ]*( uRadix + 1 ) + uRadix + 1 < parts[ stage ] ) {
						// FIXME: WARNING: OVERFLOW
					} else {
						parts[ stage ] *= uRadix + 1;
						parts[ stage ] += digit + 1;
					}
				} else {
					if( parts[ stage ]*uRadix + uRadix < parts[ stage ] ) {
						// FIXME: WARNING: OVERFLOW
					} else {
						parts[ stage ] *= uRadix;
						parts[ stage ] += U64( digit );
					}
				}
			}
		}

		if( stage >= FRACT ) {
			U64 t = parts[ FRACT ];
			U64 f = 0;

			while( t > 0 ) {
				f *= uRadix;
				f += t%( uRadix + 1 ) - 1;
				t /= uRadix + 1;
			}

			parts[ FRACT ] = f;
		}

		s.checkByte( '\'' );
		Str lit( s );
		while( s.checkByteRange( 'a', 'z' ) || s.checkByteRange( 'A', 'Z' ) ) {
		}
		lit = lit.left( s.get() - lit.get() );

		U8 flags = 0;

		Bool isFloat = stage != WHOLE;
		if( isFloat ) {
			flags = kSN_TyF64;
		} else {
			if( parts[ WHOLE ] > 0xFFFFFFFF ) {
				flags = kSN_TyS64;
			} else if( parts[ WHOLE ] > 0xFFFF ) {
				flags = kSN_TyS32;
			} else if( parts[ WHOLE ] > 0xFF ) {
				flags = kSN_TyS16;
			} else if( parts[ WHOLE ] > 1 ) {
				flags = kSN_TyS8;
			} else {
				flags = kSN_TyS1;
			}
		}

		lit.checkByte( '\'' );

		// Check the type suffix
		if( lit.isUsed() ) {
			Bool wantInt = true;

			if( lit.caseCmp( "f" ) ) {
				flags = kSN_TyF32;
				isFloat = true;
				wantInt = false;
			} else if( lit.caseCmp( "u" ) ) {
				if( parts[ WHOLE ] > 0xFFFFFFFF ) {
					flags = kSN_TyU64;
				} else if( parts[ WHOLE ] > 0xFFFF ) {
					flags = kSN_TyU32;
				} else if( parts[ WHOLE ] > 0xFF ) {
					flags = kSN_TyU16;
				} else if( parts[ WHOLE ] > 1 ) {
					flags = kSN_TyU8;
				} else {
					flags = kSN_TyU1;
				}
			} else if( lit.caseCmp( "s" ) ) {
				// This is the default
			} else if( lit.caseCmp( "s64" ) ) {
				flags = kSN_TyS64;
			} else if( lit.caseCmp( "s32" ) ) {
				flags = kSN_TyS32;
			} else if( lit.caseCmp( "s16" ) ) {
				flags = kSN_TyS16;
			} else if( lit.caseCmp( "s8" ) ) {
				flags = kSN_TyS8;
			} else if( lit.caseCmp( "s1" ) ) {
				flags = kSN_TyS1;
			} else if( lit.caseCmp( "u64" ) ) {
				flags = kSN_TyU64;
			} else if( lit.caseCmp( "u32" ) ) {
				flags = kSN_TyU32;
			} else if( lit.caseCmp( "u16" ) ) {
				flags = kSN_TyU16;
			} else if( lit.caseCmp( "u8" ) ) {
				flags = kSN_TyU8;
			} else if( lit.caseCmp( "u1" ) ) {
				flags = kSN_TyU1;
			} else if( lit.caseCmp( "mm" ) ) {
				flags |= kSN_AtMm;
				wantInt = false;
			} else if( lit.caseCmp( "cm" ) ) {
				flags |= kSN_AtCm;
				wantInt = false;
			} else if( lit.caseCmp( "in" ) ) {
				flags |= kSN_AtIn;
				wantInt = false;
			} else if( lit.caseCmp( "px" ) ) {
				flags |= kSN_AtPx;
				wantInt = false;
			} else if( lit.caseCmp( "em" ) ) {
				flags |= kSN_AtEm;
				wantInt = false;
			} else if( lit.caseCmp( "pt" ) ) {
				flags |= kSN_AtPt;
				wantInt = false;
			} else if( lit.caseCmp( "ms" ) ) {
				flags |= kSN_AtMs;
				wantInt = false;
			} else if( lit.caseCmp( "sec" ) ) {
				flags |= kSN_AtSec;
				wantInt = false;
			} else {
				m_diagEngine.diagnose( getLocFromPtr( lit ), Diag::UnknownTypeSuffix, lit );
			}

			if( wantInt && isFloat ) {
				m_diagEngine.diagnose( getLocFromPtr( m_buffer ), Diag::FloatLitHasIntType );
				isFloat = false;
			}
		}

		if( isFloat ) {
			const double x = double( parts[ WHOLE ] );
			double y = 0.0;

			U64 f = parts[ FRACT ];
			while( f > 0 ) {
				y *= 0.1;
				y += ( f%10 )*0.1;
				f /= 10;
			}

			double z = x + y;

			U64 e = parts[ EXP ];
			if( esign == NEG ) {
				while( e > 0 ) {
					z /= 10;
					--e;
				}
			} else {
				while( e > 0 ) {
					z *= 10;
					--e;
				}
			}

			dstTok.value.f = z;
		} else {
			dstTok.value.i = parts[ WHOLE ];
		}

		dstTok.setType( kTT_NumericLiteral );
		dstTok.setFlags( flags );

		dstTok.setLength( UPtr( s.get() - m_buffer.get() ) );
		m_buffer = s;

		return true;
	}
	Bool CLexer::readString( SToken &dstTok, Bool acceptDlg )
	{
		static MutStr blob;
		Str s( m_buffer );
		Str t;

		blob.clear();
		Bool nomem = false;
		Bool acceptNL = false;

		axstr_utf32_t expectChar = 0;

		enum TYPE { TXT, SPK, MSG1, MSG2, MSG3, MSG4 };
		TYPE type = TXT;

		if( acceptDlg ) {
			if( s.skipIfStartsWith( "@\"" ) ) {
				expectChar = '\"';
				acceptNL = true;
				type = SPK;
			} else if( s.checkByte( '>' ) ) {
				type = MSG1;
				while( s.checkByte( ' ' ) ) {
				}
			} else if( s.checkByte( '<' ) ) {
				type = MSG2;
				while( s.checkByte( ' ' ) ) {
				}
			} else if( s.checkByte( '~' ) ) {
				type = MSG3;
				while( s.checkByte( ' ' ) ) {
				}
			} else if( s.checkByte( '!' ) ) {
				type = MSG4;
				while( s.checkByte( ' ' ) ) {
				}
			} else {
				Str chk( s );
				switch( chk.readChar() ) {
				// 【】
				case 0x3010:
					expectChar = 0x3011;
					acceptNL = true;
					type = SPK;
					s = chk;
					break;
				// 「」
				case 0x300C:
					expectChar = 0x300D;
					acceptNL = true;
					type = MSG1;
					s = chk;
					break;
				// 『』
				case 0x300E:
					expectChar = 0x300F;
					acceptNL = true;
					type = MSG2;
					s = chk;
					break;
				// （）
				case 0xFF08:
					expectChar = 0xFF09;
					acceptNL = true;
					type = MSG3;
					s = chk;
					break;
				// ｛｝
				case 0xFF5B:
					expectChar = 0xFF5D;
					acceptNL = true;
					type = MSG4;
					s = chk;
					break;
				}
			}
		}

		if( !expectChar ) {
			if( s.checkByte( '\"' ) ) {
				expectChar = '\"';
				acceptNL = true;
			} else if( type == TXT && s.checkByteRange( 0x00, 0x7F ) ) {
				return false;
			}
		}

		t = s;
		Bool unmatched = false;
		Str ps;
		for(;;) {
			ps = s;
			if( s.checkChar( expectChar ) ) {
				break;
			}

			if( s.isEmpty() ) {
				unmatched = true;
				break;
			}

			if( s.startsWith( '\r' ) || s.startsWith( '\n' ) ) {
				if( !acceptNL ) {
					unmatched = expectChar != '\0';
					break;
				}

				t = t.left( s.get() - t.get() );
				nomem = nomem || !AX_VERIFY_MEMORY( blob.tryAppend( t ) );

				// FIXME: Don't append space if last character doesn't need a
				//        trailing space. (e.g., any Japanese or Chinese
				//        character, or any opening brackets.)
				if( t.lastByte() > 0x20 ) {
					nomem = nomem || !AX_VERIFY_MEMORY( blob.tryAppend( Str( ' ' ) ) );
				}

				while( s.checkByteRange( 0x01, 0x20 ) ) {
				}

				t = s;
				continue;
			}

			if( U8( *s ) <= ' ' && type == TXT && !expectChar ) {
				break;
			}

			if( *s != '\\' ) {
				s.readChar();
				continue;
			}

			t = t.left( s.get() - t.get() );
			nomem = nomem || !AX_VERIFY_MEMORY( blob.tryAppend( t ) );

			const char *const pBase = s.get();
			s = s.skip();

			const char esc = s.readByte();
			char szAppend[ 128 ] = { '\0', '\0' };

			if( esc == '\\' ) {
				szAppend[ 0 ] = '\\';
			} else if( esc == '?' ) {
				szAppend[ 0 ] = '?';
			} else if( esc == '\'' ) {
				szAppend[ 0 ] = '\'';
			} else if( esc == '\"' ) {
				szAppend[ 0 ] = '\"';
			} else if( esc == ' ' ) {
				szAppend[ 0 ] = ' ';
			} else if( esc == 'a' ) {
				szAppend[ 0 ] = '\a';
			} else if( esc == 'b' ) {
				szAppend[ 0 ] = '\b';
			} else if( esc == 'f' ) {
				szAppend[ 0 ] = '\f';
			} else if( esc == 'n' ) {
				szAppend[ 0 ] = '\n';
			} else if( esc == 'r' ) {
				szAppend[ 0 ] = '\r';
			} else if( esc == 't' ) {
				szAppend[ 0 ] = '\t';
			} else if( esc == 'v' ) {
				szAppend[ 0 ] = '\v';
			} else if( esc == '0' ) {
				nomem = nomem || !AX_VERIFY_MEMORY( blob.tryAppend( Str( '\0' ) ) );
			} else if( esc == 'x' ) {
				const char *pBadPos = nullptr;

				const char c[ 2 ] = {
					s.readByte(),
					s.readByte()
				};

				const int n[ 2 ] = {
					axstr_digit( c[ 0 ], 16 ),
					axstr_digit( c[ 1 ], 16 )
				};

				for( UPtr i = 0; i < 2; ++i ) {
					if( c[ i ] == -1 ) {
						pBadPos = pBase + i;
						break;
					}
				}

				if( pBadPos != nullptr ) {
					m_diagEngine.diagnose( getLocFromPtr( pBadPos ), Diag::StrEscExpectedHex );
				}

				const char xx = !pBadPos ? char( n[ 0 ]*16 + n[ 1 ] ) : '\?';
				nomem = nomem || !AX_VERIFY_MEMORY( blob.tryAppend( Str( xx ) ) );
			} else if( esc == 'u' ) {
				enum class ECodepointStatus
				{
					Good,
					BadHex,
					BadNumber,
					Terminated
				} status = ECodepointStatus::Good;
				const char *pBadPos = nullptr;

				do {
					if( !s.checkByte( '{' ) ) {
						m_diagEngine.diagnose( getLocFromPtr( s ), Diag::StrEscUnicodeNoLBrace );
						break;
					}

					axstr_utf32_t cp = 0;
					Bool didTerm = false;
					while( s.isUsed() && !( didTerm = s.checkByte( '}' ) ) ) {
						const char *const p = s.get();
						const int n = axstr_digit( s.readByte(), 16 );
						if( n < 0 ) {
							if( s.startsWith( '\"' ) ) {
								status = ECodepointStatus::Terminated;
							} else {
								status = ECodepointStatus::BadHex;
							}
							pBadPos = p;
							break;
						}

						cp *= 16;
						cp += axstr_utf32_t( n );

						if( cp > 0x10FFFF ) {
							status = ECodepointStatus::BadNumber;
							pBadPos = p;
							break;
						}
					}

					if( !didTerm && status == ECodepointStatus::Good ) {
						status = ECodepointStatus::Terminated;
					}

					if( pBadPos != nullptr ) {
						AX_ASSERT_MSG( status != ECodepointStatus::Good, "Status is not an error" );

						const SourceLoc loc = getLocFromPtr( pBadPos );
						switch( status ) {
						case ECodepointStatus::BadHex:
							m_diagEngine.diagnose( loc, Diag::StrEscUnicodeBadHex );
							break;

						case ECodepointStatus::BadNumber:
							m_diagEngine.diagnose( loc, Diag::StrEscUnicodeBadCP, U64( cp ) );
							break;

						case ECodepointStatus::Terminated:
							m_diagEngine.diagnose( loc, Diag::StrEscUnicodeEarlyTerm );
							break;

						case ECodepointStatus::Good:
							// FIXME: *UNREACHABLE*
							break;
						}

						break;
					}
					
					axstr_utf8_t utf8cp[ 5 ] = { 0, 0, 0, 0, 0 };

					const axstr_utf32_t utf32cp[] = { cp, 0 };
					const Bool result = !!axstr_utf32_to_utf8( utf8cp, sizeof( utf8cp ), utf32cp );

					if( !result ) {
						m_diagEngine.diagnose( getLocFromPtr( s ), Diag::InternalStrEscUnicodeConvertFailure, U32( cp ) );
						break;
					}

					nomem = nomem || !AX_VERIFY_MEMORY( blob.tryAppend( ( const char * )utf8cp ) );
				} while( false );
			} else if( esc == ',' ) {
				nomem = nomem || !AX_VERIFY_MEMORY( blob.tryAppend( "\xD1\x01" ) );
			}

			// FIXME: Add '\#{<COLOR>}' support.
			// FIXME: Add '\{ruby:<TEXT>:<OVER>}' support.
			// FIXME: Add '\{type[(<DELAY>)]:<TEXT>}' support.
			// FIXME: Add '\[+^]{CC}' support.
			// FIXME: Add '\&<HTMLSEQ>;' support.

			if( szAppend[ 0 ] != '\0' ) {
				nomem = nomem || !AX_VERIFY_MEMORY( blob.tryAppend( szAppend ) );
			}

			t = s;
		}

		if( unmatched ) {
			m_diagEngine.diagnose( getLocFromPtr( m_buffer ), Diag::UnterminatedString );
		}

		t = t.left( ps.get() - t.get() );
		if( t.isUsed() ) {
			nomem = nomem || !AX_VERIFY_MEMORY( blob.tryAppend( t ) );
		}

		nomem = nomem || !AX_VERIFY_MEMORY( m_progData.addString( t ) );

		if( nomem ) {
			m_diagEngine.diagnose( getLocFromPtr( m_buffer ), Diag::InternalStrLitNoMem );
			return false;
		}

#if 0
		if( !nomem ) {
			wchar_t wszBuf[ 4096 ] = { L'\0' };
			blob.toWStr( wszBuf );

			MessageBoxW( NULL, wszBuf, L"Strlit", 0 );
		}
#endif

		// Special attribute handling for messages
		if( type >= MSG1 && type <= MSG4 && expectChar != 0 ) {
			if( s.checkByte( 'P' ) ) {
				dstTok.setFlags( kMsg_AttribP, kMsg_AttribMask );
			} else if( s.checkByte( 'B' ) ) {
				dstTok.setFlags( kMsg_AttribB, kMsg_AttribMask );
			} else /* assume 'R' */ {
				s.checkByte( 'R' );
				dstTok.setFlags( kMsg_AttribR, kMsg_AttribMask );
			}
		}

		switch( type ) {
		case TXT:
			dstTok.setType( kTT_StringLiteral );
			break;

		case SPK:
			dstTok.setType( kTT_Speaker );
			break;

		case MSG1:
			dstTok.setType( kTT_Message );
			dstTok.setFlags( kMsg_Text1, kMsg_TextMask );
			break;
		case MSG2:
			dstTok.setType( kTT_Message );
			dstTok.setFlags( kMsg_Text2, kMsg_TextMask );
			break;
		case MSG3:
			dstTok.setType( kTT_Message );
			dstTok.setFlags( kMsg_Text3, kMsg_TextMask );
			break;
		case MSG4:
			dstTok.setType( kTT_Message );
			dstTok.setFlags( kMsg_Text4, kMsg_TextMask );
			break;
		}

		dstTok.setLength( UPtr( m_buffer.len() - s.len() ) );
		m_buffer = s;

		return true;
	}

}}
