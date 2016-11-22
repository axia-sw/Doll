#pragma once

#include "../Core/Defs.hpp"

#include "../Util/ValueStack.hpp"

#include "Ident.hpp"
#include "ProgramData.hpp"
#include "Source.hpp"
#include "SourceLoc.hpp"
#include "Token.hpp"

namespace doll { namespace script {

	class CCompilerContext;
	class CDiagnosticEngine;

	enum class ELexCommentMode
	{
		// Ignore all comment tokens completely
		Ignore,
		// Emit comment blocks
		Emit,
		// Emit testing comment blocks
		EmitTests,
		// Store the last comment token
		Keep
	};
	
	struct NormalLexerOpts {};
	struct TestingLexerOpts {};

	struct LexerOpts
	{
		ELexCommentMode comments;

		LexerOpts()
		: comments( ELexCommentMode::Ignore )
		{
		}
		LexerOpts( ELexCommentMode comments )
		: comments( comments )
		{
		}
		LexerOpts( NormalLexerOpts )
		: comments( ELexCommentMode::Ignore )
		{
		}
		LexerOpts( TestingLexerOpts )
		: comments( ELexCommentMode::EmitTests )
		{
		}

		LexerOpts( const LexerOpts & ) = default;
		LexerOpts &operator=( const LexerOpts & ) = default;
		~LexerOpts() = default;
	};

	class CLexer
	{
	public:
		CLexer( Source &src, CDiagnosticEngine &diagEngine, IdentDictionary &dict, CProgramData &progData, const LexerOpts &opts = LexerOpts() );
		CLexer( CCompilerContext &ctx, const LexerOpts &opts = LexerOpts() );
		~CLexer();

		Bool lex( SToken &dstTok );
		Bool peek( SToken &dstTok ) const;

		Bool skip();

		Str getFilename() const
		{
			return m_src.filename;
		}
		Str getLexan( const SToken &tok ) const
		{
			if( tok.getOffset() + tok.getLength() > m_src.buffer.len() ) {
				return Str();
			}

			return m_src.buffer.mid( axstr_ptrdiff_t( tok.getOffset() ), tok.getLength() );
		}

		const LexerOpts &getOpts() const
		{
			return m_opts;
		}

		SourceLoc getLocFromToken( const SToken &tok ) const
		{
			if( tok.getSourceIndex() != m_src.index || tok.getOffset() + tok.getLength() > m_src.buffer.len() ) {
				return SourceLoc();
			}

			return SourceLoc( &m_src, U32( tok.getOffset() ) );
		}

		SourceLoc getLocFromPtr( const char *p ) const
		{
			const Str buf( m_src.buffer );

			AX_ASSERT_NOT_NULL( p );
			AX_ASSERT_MSG( p >= buf.get() && p <= buf.getEnd(), "Pointer not within source" );

			return SourceLoc( &m_src, U32( p - buf.get() ) );
		}
		SourceLoc getLocFromPtr( Str s ) const
		{
			return getLocFromPtr( s.get() );
		}

		SourceRange getRangeFromToken( const SToken &tok ) const
		{
			return SourceRange( &m_src, U32( tok.getOffset() ), U32( tok.getLength() ) );
		}

		SourceLoc getEndLoc() const
		{
			return getLocFromPtr( m_src.buffer.getEnd() );
		}

		Bool calcLineInfo( U32 &dstRow, U32 &dstCol, SourceLoc loc ) const
		{
			AX_ASSERT( loc.pSource == &m_src );
			return scr_calcLineInfo( dstRow, dstCol, loc );
		}

		// Check the parenthesis balance; called through test "EXPECT-()BAL" directives
		U32 getParenthesisBalance() const
		{
			return m_nestingLevels[ kBalance_Paren ];
		}
		// Check the bracket balance; called through test "EXPECT-[]BAL" directives
		U32 getBracketBalance() const
		{
			return m_nestingLevels[ kBalance_Brack ];
		}
		// Check the brace balance; called through test "EXPECT-{}BAL" directives
		U32 getBraceBalance() const
		{
			return m_nestingLevels[ kBalance_Brace ];
		}

	private:
		enum EBalanceType: U32
		{
			kBalance_Brace = 0,
			kBalance_Paren = 1,
			kBalance_Brack = 2,

			kNumBalances = 3
		};
		enum ELexerFlags: U32
		{
			// Can lex a label token (*something)
			kStateF_AcceptLabel = 0x00000001,
			// Can lex dialogue tokens
			kStateF_AcceptDlg   = 0x00000002,
			// Waiting for menu braces
			kStateF_AwaitMenu   = 0x00000004,
			// Currently in a menu
			kStateF_InMenu      = 0x00000008,
			// The last token was blank, so the next token starts the line
			kStateF_WasBlank    = 0x00000010,

			kStateM_MenuBalance = 0xFF000000,
			kStateS_MenuBalance = 24
		};

		Source &                  m_src;
		CDiagnosticEngine &       m_diagEngine;
		CProgramData &            m_progData;
		IdentDictionary &         m_dict;
		Str                       m_buffer;
		TMutArr<SToken>           m_tokStack;
		SToken                    m_nextToken;
		SToken                    m_keepToken;
		SToken *                  m_pComment;
		U32                       m_stateFlags;
		TValueStack<kNumBalances> m_balance;
		U32                       m_nestingLevels[ 3 ];
		LexerOpts                 m_opts;

		Bool lexNextToken();

		Bool skipWhitespaceAndComments();

		Bool readIdent( SToken &dstTok, Bool acceptLabel );
		Bool readPunct( SToken &dstTok );
		Bool readNumber( SToken &dstTok );
		Bool readString( SToken &dstTok, Bool acceptDlg );

		inline U32 getMenuBalance() const {
			return
				( m_stateFlags & kStateM_MenuBalance ) >> kStateS_MenuBalance;
		}
		inline Void setMenuBalance( U32 balance ) {
			m_stateFlags =
				( m_stateFlags & ~kStateM_MenuBalance ) |
				( ( balance << kStateS_MenuBalance ) & kStateM_MenuBalance );
		}
		inline Bool isAtMenuChoices() const {
			return
				( m_stateFlags & kStateF_InMenu ) &&
				getMenuBalance() == getBraceBalance() &&
				m_balance.isTop( kBalance_Brace );
		}
	};

}}
