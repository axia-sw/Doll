#include "doll/Script/Compiler.hpp"
#include "doll/Script/LanguageVersion.hpp"
#include "doll/Script/Type.hpp"
#include "doll/Script/Ident.hpp"

#include "doll/Core/Logger.hpp"

namespace doll { namespace script {

	CCompilerContext::CCompilerContext()
	: m_pSources()
	, m_srcStack()
	, m_srcQueue()
	, m_diagEngine( m_pSources )
	, m_progData()
	, m_symDict()
	{
	}
	CCompilerContext::~CCompilerContext()
	{
	}

	Bool CCompilerContext::init( EVersion ver )
	{

		// Initialize the dictionary system (efficient symbol look-up)
		if( !initCompilerDictionary( *this, m_symDict, ver ) ) {
			return false;
		}

		Bool r = true;

		// Create the 'Void' built-in type
		r = r && addBuiltinType< BuiltinVoidType >( "Void" );

		// Create the fixed-width 'Int' and 'UInt' built-in types
		r = r && addBuiltinType< BuiltinIntType >( "Int1"  , EIntSign::Signed  ,  1 );
		r = r && addBuiltinType< BuiltinIntType >( "Int8"  , EIntSign::Signed  ,  8 );
		r = r && addBuiltinType< BuiltinIntType >( "Int16" , EIntSign::Signed  , 16 );
		r = r && addBuiltinType< BuiltinIntType >( "Int32" , EIntSign::Signed  , 32 );
		r = r && addBuiltinType< BuiltinIntType >( "Int64" , EIntSign::Signed  , 64 );
		r = r && addBuiltinType< BuiltinIntType >( "UInt1" , EIntSign::Unsigned,  1 );
		r = r && addBuiltinType< BuiltinIntType >( "UInt8" , EIntSign::Unsigned,  8 );
		r = r && addBuiltinType< BuiltinIntType >( "UInt16", EIntSign::Unsigned, 16 );
		r = r && addBuiltinType< BuiltinIntType >( "UInt32", EIntSign::Unsigned, 32 );
		r = r && addBuiltinType< BuiltinIntType >( "UInt64", EIntSign::Unsigned, 64 );

		// Create the register- and pointer-sized 'Int' and 'UInt' built-in types
		r = r && addBuiltinType< BuiltinIntType >( "IntReg" , EIntSign::Signed  , EIntMode::Register );
		r = r && addBuiltinType< BuiltinIntType >( "UIntReg", EIntSign::Unsigned, EIntMode::Register );
		r = r && addBuiltinType< BuiltinIntType >( "IntPtr" , EIntSign::Signed  , EIntMode::Pointer  );
		r = r && addBuiltinType< BuiltinIntType >( "UIntPtr", EIntSign::Unsigned, EIntMode::Pointer  );

		// Create the 'Float' built-in types
		r = r && addBuiltinType< BuiltinFloatType >( "Float32", 32 );
		r = r && addBuiltinType< BuiltinFloatType >( "Float64", 64 );

		// Create the string built-in types
		r = r && addBuiltinType< BuiltinStringType >( "Str"   , EContainerMode::View );
		r = r && addBuiltinType< BuiltinStringType >( "MutStr", EContainerMode::Dynamic );
		r = r && addBuiltinType< BuiltinStringType >( "BufStr", EContainerMode::Static );

		// Done
		return r;
	}

	Bool CCompilerContext::openSource( const Str &filename )
	{
		auto queueIt = m_srcQueue.addTail();
		if( !AX_VERIFY_MEMORY( queueIt != m_srcQueue.end() ) ) {
			return false;
		}

		if( !AX_VERIFY_MEMORY( m_srcStack.append() ) ) {
			return false;
		}

		Source *const pSrc = allocSource();
		if( !AX_VERIFY_MEMORY( pSrc ) ) {
			m_srcStack.removeLast();
			m_srcQueue.remove( queueIt );
			return false;
		}

		if( !loadSourceFile( *pSrc, filename ) ) {
			m_srcStack.removeLast();
			m_srcQueue.remove( queueIt );
			freeSource( pSrc );
			return false;
		}

		m_srcStack.last() = pSrc->index;
		*queueIt = pSrc->index;
		return true;
	}
	Bool CCompilerContext::nextSource()
	{
		AX_ASSERT_MSG( m_srcStack.isEmpty(), "Cannot go to next queued source unless stack is empty." );
		AX_ASSERT_MSG( m_srcQueue.isUsed(), "Cannot go to next queued source because queue is empty." );

		m_srcQueue.remove( m_srcQueue.begin() );

		m_srcStack.clear();
		if( m_srcQueue.isEmpty() ) {
			// no more sources
			return false;
		}

		if( !AX_VERIFY_MEMORY( m_srcStack.append( *m_srcQueue.begin() ) ) ) {
			// effectively no more sources -- not enough memory to even maintain state
			return false;
		}

		// have another source to go through
		return true;
	}

	Bool CCompilerContext::pushSource( const Str &filename )
	{
		if( !AX_VERIFY_MEMORY( m_srcStack.append() ) ) {
			return false;
		}

		Source *const pSrc = allocSource();
		if( !AX_VERIFY_MEMORY( pSrc ) ) {
			m_srcStack.removeLast();
			return false;
		}

		if( !loadSourceFile( *pSrc, filename ) ) {
			m_srcStack.removeLast();
			return false;
		}

		m_srcStack.last() = pSrc->index;
		return true;
	}
	Bool CCompilerContext::popSource()
	{
		AX_ASSERT_MSG( m_srcStack.isUsed(), "No source file to pop" );

		m_srcStack.removeLast();
		return m_srcStack.isUsed();
	}

	const Source *CCompilerContext::getActiveSource_const() const
	{
		if( m_srcStack.isEmpty() ) {
			return nullptr;
		}

		const U16 srcIdx = m_srcStack.last();
		AX_ASSERT( srcIdx < m_pSources.num() );
		AX_ASSERT_NOT_NULL( m_pSources[ srcIdx ] );

		return m_pSources[ srcIdx ];
	}
	CDiagnosticEngine &CCompilerContext::getDiagnosticEngine()
	{
		return m_diagEngine;
	}
	CProgramData &CCompilerContext::getProgramData()
	{
		return m_progData;
	}
	IdentDictionary &CCompilerContext::getSymbolMap()
	{
		return m_symDict;
	}

	Bool CCompilerContext::testCurrentSource()
	{
		class CaptureLine
		{
		public:
			CaptureLine( const CLexer &lexer, const SToken &tok )
			: m_lexer( lexer )
			, m_token( tok )
			, m_gotLoc( false )
			, m_loc()
			, m_row( 0 )
			, m_col( 0 )
			{
			}
			CaptureLine( const CaptureLine & ) = default;
			~CaptureLine() = default;

			CaptureLine &operator=( const CaptureLine & ) = delete;

			U32 getLine() const
			{
				getLoc();
				return m_row;
			}
			U32 getColumn() const
			{
				getLoc();
				return m_col;
			}

		private:
			const CLexer &m_lexer;
			const SToken &m_token;

			mutable Bool      m_gotLoc;
			mutable SourceLoc m_loc;
			mutable U32       m_row;
			mutable U32       m_col;

			const SourceLoc &getLoc() const
			{
				if( !m_gotLoc ) {
					m_gotLoc = true;

					m_loc = m_lexer.getLocFromToken( m_token );
					m_lexer.calcLineInfo( m_row, m_col, m_loc );
				}

				return m_loc;
			}
		};

		struct Directive
		{
			Str         text;
			SourceRange range;

			operator Str() const { return text; }

			operator SourceRange() const { return range; }
			operator SourceLoc() const { return range; }

			Bool cmp( Str x ) const { return text.cmp( x ); }
			Bool operator==( Str x ) const { return text.cmp( x ); }
		};

		// Verify that we've got an active source file
		if( !AX_VERIFY_NOT_NULL( getActiveSource() ) ) {
			return false;
		}

		// Create a lexer for this source file, primed for testing purposes
		CLexer lexer( *this, TestingLexerOpts() );

		// Name of the source file
		//const Str filename( lexer.getFilename() ); // not used presently

		// Current token
		SToken tok;

		// Read in the token
		if( !lexer.lex( tok ) ) {
			m_diagEngine.diagnose( lexer.getEndLoc(), Diag::TestMissingFirstTok );
			return false;
		}

		// The first token of a test file must be a test directive
		if( tok.isNot( kTT_Comment ) || tok.getFlags( kSC_StyleMask ) != kSC_TestStyle ) {
			m_diagEngine.diagnose( tok, Diag::TestFirstTokNotTestType );
			return false;
		}

		// Text of the test directive, sans the "//::"
		const Str headlexan( lexer.getLexan( tok ).skip( 4 ) );
		// Ensure the test directive is in the proper format
		if( !headlexan.startsWith( "TEST-" ) || !headlexan.endsWith( ":://" ) ) {
			m_diagEngine.diagnose( tok, Diag::TestFirstTokNotTestType );
			return false;
		}

		// Text from the token explaining the type of test this is (e.g., "LEXER")
		const Str testtypetext( headlexan.skip( 5 ).drop( 4 ) );
	
		// Various test types
		enum class ETestType
		{
			// Not a test; indicates the given text was invalid
			Invalid,

			// "//::TEST-LEXER:://" Test the lexer's ability to properly dispense tokens
			Lexer,
			// "//::TEST-PARSER:://" Test the parser's ability to do proper AST and sema construction
			Parser,
			// "//::TEST-CODEGEN:://" Test the code generator's ability to properly output instructions
			CodeGen,
			// "//::TEST-RESOURCES:://" Test the resource tracker's ability to manage resources
			Resources
		};

		// The type of test this is
		const ETestType testtype =
			testtypetext == "LEXER"     ? ETestType::Lexer     :
			testtypetext == "PARSER"    ? ETestType::Parser    :
			testtypetext == "CODEGEN"   ? ETestType::CodeGen   :
			testtypetext == "RESOURCES" ? ETestType::Resources :
			ETestType::Invalid;

		// If the test is invalid, then bail out
		if( testtype == ETestType::Invalid ) {
			m_diagEngine.diagnose( tok, Diag::TestInvalidType, testtypetext );
			return false;
		}

		// Current collection of directives
		TSmallList< Directive, 128 > directives;

		// Test the lexer
		switch( testtype ) {
		case ETestType::Lexer:
			// Main lexer loop
			for(;;) {
				// Whether the next token should be the end of the file or not
				const Bool expectingEof = directives.isUsed() && directives.begin()->cmp( "EXPECT-EOF" );

				// Grab the next token; handle EOF if that's what we got
				if( !lexer.peek( tok ) ) {
					Bool breakFromLoop = false;

					// Clean exit if this is acceptable
					if( expectingEof ) {
						// Remove the directive stating we should be expecting an EOF
						if( directives.isUsed() ) {
							directives.remove( directives.begin() );
						}

						// Don't exit immediately, just break from the loop
						breakFromLoop = true;
					} else {
						m_diagEngine.diagnose( lexer.getEndLoc(), Diag::TestUnexpectedEOF );
					}

					// Provide a note about the current directive
					if( directives.isUsed() ) {
						m_diagEngine.diagnose( directives.begin()->range, Diag::TestOutstandingDirectives, directives.begin()->text );
					}

					// If we're set to just break from the loop, then do that
					if( breakFromLoop ) {
						break;
					}

					// Failed test
					return false;
				}

				// Line information for the token, calculated on demand
				CaptureLine tokcap( lexer, tok );

				// Check for a new directive
				if( tok.is( kTT_Comment ) && tok.getFlags( kSC_StyleMask ) == kSC_TestStyle ) {
					const Str lexan( lexer.getLexan( tok ) );
					AX_ASSERT( lexan.startsWith( "//::" ) );

					auto directiveIter = directives.addTail();
					if( !AX_VERIFY_MEMORY( directiveIter != directives.end() ) ) {
						return false;
					}

					directiveIter->text  = lexan.skip( 4 );
					directiveIter->range = lexer.getRangeFromToken( tok );

					lexer.skip();
					continue;
				}

				// Working text of the current directive
				Str directive;
				SourceRange directiveRange;

				// Pull the current directive from the list
				if( directives.isUsed() ) {
					auto firstDirective = directives.begin();
					
					directive      = firstDirective->text;
					directiveRange = firstDirective->range;

					directives.remove( firstDirective );
				}

				// "EXPECT-TOKEN" directive handler
				if( directive.skipAndTrimIfStartsWith( "EXPECT-TOKEN:" ) ) {
					// Whether this token should be starting the line or not
					Bool shouldStartLine = false;

					// Read all of this directive's flags
					for(;;) {
						// Check the flag
						const Str flag( directive.startsWith( '+' ) ? directive.readToken() : Str() );
						// If we didn't get one, exit this loop
						if( flag.isEmpty() ) {
							break;
						}

						// Token should start the line?
						if( flag.caseCmp( "+L" ) ) {
							shouldStartLine = true;
						// Unknown flag for this directive
						} else {
							m_diagEngine.diagnose( directiveRange, Diag::TestLexUnknownFlag, flag );
							return false;
						}
					}

					// The directive's expected token type name
					const Str expname( directive.readToken() );
					// The directive's expected token text
					const Str exptext( directive.trim() );

					// The directive's expected token type
					ETokenType exptokty;

					// Determine the expected token's type
					if( !scr_stringToTokenType( exptokty, expname ) ) {
						m_diagEngine.diagnose( directiveRange, Diag::TestLexUnknownTokTy, expname );
						return false;
					}

					// Trimmed lexan (text) of the token
					const Str lexan( lexer.getLexan( tok ).trim() );

					// Check that the token's "starts line" flag is the expected value
					if( tok.isStartingLine() != shouldStartLine ) {
						if( shouldStartLine ) {
							m_diagEngine.diagnose( tok, Diag::TestLexUnexpectedContinueLine );
						} else {
							m_diagEngine.diagnose( tok, Diag::TestLexUnexpectedStartLine );
						}
						return false;
					}

					// Check that the token type matches what was specified
					if( tok.isNot( exptokty ) ) {
						const Str expty( scr_tokenTypeToString( exptokty ) );
						const Str gotty( scr_tokenTypeToString( tok.getType() ) );

						m_diagEngine.diagnose( tok, Diag::TestLexTokenMismatch, expty, gotty, lexan );
						return false;
					}

					// Check that the token's lexan matches the given text
					if( lexan != exptext ) {
						m_diagEngine.diagnose( tok, Diag::TestLexLexanMismatch, exptext, lexan );
						return false;
					}

					// This token was valid, so skip it
					lexer.skip();
				// "EXPECT-()BAL" directive handler: Check parenthesis balance
				} else if( directive.skipAndTrimIfStartsWith( "EXPECT-()BAL:" ) ) {
					// Numeric text
					const Str numtxt( directive.readToken() );

					// Convert to a number
					const axstr_uint_t num = numtxt.toUnsignedInteger();

					// Check that the balance matches
					if( lexer.getParenthesisBalance() != num ) {
						m_diagEngine.diagnose( tok, Diag::TestLexParenBalance, U64( num ), U64( lexer.getParenthesisBalance() ) );
						return false;
					}
				// "EXPECT-[]BAL" directive handler: Check bracket balence
				} else if( directive.skipAndTrimIfStartsWith( "EXPECT-[]BAL:" ) ) {
					// Numeric text
					const Str numtxt( directive.readToken() );

					// Convert to a number
					const axstr_uint_t num = numtxt.toUnsignedInteger();

					// Check that the balance matches
					if( lexer.getBracketBalance() != num ) {
						m_diagEngine.diagnose( tok, Diag::TestLexBrackBalance, U64( num ), U64( lexer.getBracketBalance() ) );
						return false;
					}
				// "EXPECT-{}BAL" directive handler: Check brace balance
				} else if( directive.skipAndTrimIfStartsWith( "EXPECT-{}BAL:" ) ) {
					// Numeric text
					const Str numtxt( directive.readToken() );

					// Convert to a number
					const axstr_uint_t num = numtxt.toUnsignedInteger();

					// Check that the balance matches
					if( lexer.getBraceBalance() != num ) {
						m_diagEngine.diagnose( tok, Diag::TestLexBraceBalance, U64( num ), U64( lexer.getBraceBalance() ) );
						return false;
					}
				// "EXPECT-EOF" directive handler: End-of-file expected
				} else if( directive.skipAndTrimIfStartsWith( "EXPECT-EOF" ) ) {
					// Might have a habitual ':' after the directive, but not required
					directive.skipAndTrimIfStartsWith( ":" );

					// We expect an EOF obviously, so if this isn't an EOF then error
					if( !tok.isEOF() ) {
						const Str tokty( scr_tokenTypeToString( tok.getType() ) );
						const Str lexan( lexer.getLexan( tok ) );

						m_diagEngine.diagnose( tok, Diag::TestLexExpectedEOF, tokty, lexan );
						return false;
					}
				// Unknown directive
				} else if( directive.isUsed() ) {
					const Str unkdir( directive.readToken() );

					m_diagEngine.diagnose( directiveRange, Diag::TestUnknownDirective, unkdir );
					return false;
				}
			}
			break; // �� Lexer test

		default:
			// Report unimplemented test scenarios
			m_diagEngine.diagnose( tok, Diag::TestUnimplementedType, testtypetext );
			return false;
		}

		// Done
		return true;
	}

	Source *CCompilerContext::allocSource()
	{
		Source **ppSrc = nullptr;
		if( m_pSources.num() == Source::kMaxSources || !m_pSources.append() ) {
			for( UPtr i = 0; i < Source::kMaxSources; ++i ) {
				if( m_pSources[ i ] == nullptr ) {
					ppSrc = &m_pSources[ i ];
					break;
				}
			}
		} else {
			if( !m_pSources.last() ) {
				ppSrc = &m_pSources.last();
			}
		}
		
		if( !ppSrc ) {
			return nullptr;
		}

		Source *&pSrc = *ppSrc;
		if( !AX_VERIFY_MEMORY( pSrc = DOLL__SCRIPTOBJ_NEW Source( *this ) ) ) {
			return nullptr;
		}

		pSrc->index = U16( ppSrc - m_pSources.pointer() );
		AX_ASSERT( m_pSources[ pSrc->index ] == pSrc );

		return pSrc;
	}
	NullPtr CCompilerContext::freeSource( Source *pSrc )
	{
		if( !pSrc ) {
			return nullptr;
		}

		Source &src = *pSrc;
		const UPtr srcIndex = UPtr( src.index );

		AX_ASSERT_MSG( pSrc == m_pSources[ srcIndex ], "Corrupt source" );

		delete pSrc;
		m_pSources[ srcIndex ] = nullptr;

		return nullptr;
	}

	Bool CCompilerContext::loadSourceFile( Source &src, const Str &filename )
	{
		if( !AX_VERIFY_MEMORY( src.filename.assign( filename ) ) ) {
			return false;
		}

		if( !core_readText( src.buffer, filename ) ) {
			src.filename.purge();
			return false;
		}

		if( src.buffer.len() >= UPtr( 1 )<<24 ) {
			src.buffer.purge();
			src.filename.purge();

			g_ErrorLog( filename ) += "Source file too big.";
			return false;
		}

		return true;
	}

}}
