#pragma once

#include "../Core/Defs.hpp"
#include "SourceLoc.hpp"
#include "DiagnosticsFmt.hpp"

/*

	NOTE: This diagnostics system is directly inspired by Apple Swift's
	`     diagnostic system, however no code is shared.

*/

namespace doll { namespace script {

	struct SToken;
	class Ident;

	class Diagnostic;

	enum class EDiagnosticId: U32
	{
#define DOLL_SCRIPT__DIAG(Sev_,Id_,Cat_,Opt_,Msg_,ArgTys_) Id_,
#include "Diagnostics.def.hpp"
#undef DOLL_SCRIPT__DIAG
	};

	enum class EDiagnosticSeverity
	{
		Note,
		Warning,
		Error
	};
	enum class EDiagnosticCategory
	{
		Basic,
		Lexer,
		Parser,
		Codegen,
		Internal,
		Testing
	};
	enum class EDiagnosticOption
	{
		None
	};

	template< typename... TArgs >
	struct TDiagnosticInfo
	{
		EDiagnosticId id;
	};

	namespace Diag
	{
#define DOLL_SCRIPT__DIAG(Sev_,Id_,Cat_,Opt_,Msg_,ArgTys_) \
	extern TDiagnosticInfo< void ArgTys_ > Id_;

#include "Diagnostics.def.hpp"

#undef DOLL_SCRIPT__DIAG
	}

	struct SDiagnosticDetails
	{
		EDiagnosticSeverity sev;
		EDiagnosticCategory cat;
		EDiagnosticOption   opt;
		Str                 msg;
	};

	class Diagnostic
	{
	public:
		Diagnostic( EDiagnosticId id, TArr<DiagnosticArgument> args )
		: m_id( id )
		, m_loc()
		, m_ranges()
		, m_args( args )
		{
		}
		template< typename... TArgs >
		Diagnostic( TDiagnosticInfo< Void( TArgs... ) > diagInfo, TArgs... args )
		: m_id( diagInfo.id )
		, m_loc()
		, m_ranges()
		, m_args()
		{
			DiagnosticArgument diagArgs[] = {
				move( args )... , DiagnosticArgument( S64( 0 ) )
			};

			AX_EXPECT_MEMORY( m_args.append( arraySize( diagArgs ) - 1, diagArgs ) );
		}

		EDiagnosticId getId() const
		{
			return m_id;
		}
		SourceLoc getLoc() const
		{
			return m_loc;
		}
		TArr<SourceRange> getRanges() const
		{
			return TArr<SourceRange>( m_ranges );
		}
		TArr<DiagnosticArgument> getArgs() const
		{
			return TArr<DiagnosticArgument>( m_args );
		}

		template< typename... TArgs >
		Bool is( TDiagnosticInfo< TArgs... > diagInfo ) const
		{
			return m_id == diagInfo.id;
		}

		Diagnostic &setLoc( SourceLoc loc )
		{
			m_loc = loc;
			return *this;
		}
		Diagnostic &addRange( SourceRange range )
		{
			AX_EXPECT_MEMORY( m_ranges.append( range ) );
			return *this;
		}

	private:
		const EDiagnosticId         m_id;
		SourceLoc                   m_loc;
		TMutArr<SourceRange>        m_ranges;
		TMutArr<DiagnosticArgument> m_args;
	};

	class IDiagnosticReporter
	{
	public:
		IDiagnosticReporter() {}
		virtual ~IDiagnosticReporter() {}

		virtual Void report( const Diagnostic &, const SDiagnosticDetails & ) = 0;
	};

	class CBuiltinLogger_DiagnosticReporter: public IDiagnosticReporter
	{
	public:
		CBuiltinLogger_DiagnosticReporter();
		virtual ~CBuiltinLogger_DiagnosticReporter();

		virtual Void report( const Diagnostic &, const SDiagnosticDetails & ) override;
	};

	class CDiagnosticEngine
	{
	public:
		CDiagnosticEngine( const TMutArr<Source *> &pSources );
		~CDiagnosticEngine();

		Void diagnose( const Diagnostic &diag );

		Void diagnose( SourceRange range, EDiagnosticId id, TArr<DiagnosticArgument> args = TArr<DiagnosticArgument>() );
		Void diagnose( const SToken &tok, EDiagnosticId id, TArr<DiagnosticArgument> args = TArr<DiagnosticArgument>() );
		template< typename... TArgs >
		Void diagnose( SourceRange range, TDiagnosticInfo< Void( TArgs... ) > diag, TArgs... args )
		{
			DiagnosticArgument diagArgs[] = {
				move( args )... , DiagnosticArgument( S64( 0 ) )
			};

			diagnose( range, diag.id, diagArgs );
		}
		template< typename... TArgs >
		Void diagnose( const SToken &tok, TDiagnosticInfo< Void( TArgs... ) > diag, TArgs... args )
		{
			DiagnosticArgument diagArgs[] = {
				move( args )..., DiagnosticArgument( S64( 0 ) )
			};

			diagnose( tok, diag.id, diagArgs );
		}

		U32 numWarnings() const
		{
			return m_cWarnings;
		}
		U32 numErrors() const
		{
			return m_cErrors;
		}
		U32 maxErrors() const
		{
			return m_cMaxErrors;
		}

		Bool didError() const
		{
			return m_cErrors > 0;
		}
		
		Void enableWarningsAreErrors()
		{
			setOption( kOptF_WarningsAreErrors );
		}
		Void disableWarningsAreErrors()
		{
			clearOption( kOptF_WarningsAreErrors );
		}
		Bool warningsAreErrors() const
		{
			return testOption( kOptF_WarningsAreErrors );
		}

		Void enableErrorsAreFatal()
		{
			setOption( kOptF_ErrorsAreFatal );
		}
		Void disableErrorsAreFatal()
		{
			clearOption( kOptF_ErrorsAreFatal );
		}
		Bool errorsAreFatal() const
		{
			return testOption( kOptF_ErrorsAreFatal );
		}

		Void enableNotes()
		{
			clearOption( kOptF_DiscardNotes );
		}
		Void disableNotes()
		{
			setOption( kOptF_DiscardNotes );
		}
		Bool notesAreEnabled()
		{
			return !testOption( kOptF_DiscardNotes );
		}

		Void enableWarnings()
		{
			clearOption( kOptF_DiscardWarnings );
		}
		Void disableWarnings()
		{
			setOption( kOptF_DiscardWarnings );
		}
		Bool warningsAreEnabled()
		{
			return !testOption( kOptF_DiscardWarnings );
		}

	protected:
		// Added so Clang would STFU
		inline U32 getStates_() const
		{
			return m_states;
		}

	private:
		enum: U32
		{
			kOptF_WarningsAreErrors = 0x00000001,
			kOptF_ErrorsAreFatal    = 0x00000002,
			kOptF_DiscardNotes      = 0x00000004,
			kOptF_DiscardWarnings   = 0x00000008
		};
		enum: U32
		{
			kStateF_Suppress = 0x00000001
		};
		const TMutArr<Source *> &      m_pSources;
		TMutArr<IDiagnosticReporter *> m_pReporters;
		U32                            m_cWarnings;
		U32                            m_cErrors;
		U32                            m_cMaxErrors;
		U32                            m_options;
		U32                            m_states;
		MutStr                         m_formatted;

		inline Void setOption( U32 opt )
		{
			m_options |= opt;
		}
		inline Void clearOption( U32 opt )
		{
			m_options &= ~opt;
		}
		inline Bool testOption( U32 opt ) const
		{
			return ( m_options & opt ) == opt;
		}

		CDiagnosticEngine( const CDiagnosticEngine & ) = delete;
		CDiagnosticEngine &operator=( const CDiagnosticEngine & ) = delete;
	};

}}
