#include "../BuildSettings.hpp"

#include "doll/Script/Diagnostics.hpp"
#include "doll/Script/Token.hpp"
#include "doll/Script/Source.hpp"
#include "doll/Core/Logger.hpp"

namespace doll { namespace script {

	namespace Diag
	{
#define DOLL_SCRIPT__DIAG(Sev_,Id_,Cat_,Opt_,Msg_,ArgTys_) \
	TDiagnosticInfo< void ArgTys_ > Id_ = { EDiagnosticId::Id_ };

#include "doll/Script/Diagnostics.def.hpp"

#undef DOLL_SCRIPT__DIAG
	}

	static const SDiagnosticDetails g_diagDetails[] = {
#define DOLL_SCRIPT__DIAG(Sev_,Id_,Cat_,Opt_,Msg_,ArgTys_) \
	{ EDiagnosticSeverity::Sev_, EDiagnosticCategory::Cat_, EDiagnosticOption::Opt_, Msg_ },

#include "doll/Script/Diagnostics.def.hpp"

#undef DOLL_SCRIPT__DIAG

		{ EDiagnosticSeverity::Error, EDiagnosticCategory::Internal, EDiagnosticOption::None, "(invalid-diag)" }
	};

	CDiagnosticEngine::CDiagnosticEngine( const TMutArr<Source *> &pSources )
	: m_pSources( pSources )
	, m_pReporters()
	, m_cWarnings( 0 )
	, m_cErrors( 0 )
	, m_cMaxErrors( 0 )
	, m_options( 0 )
	, m_states( 0 )
	, m_formatted()
	{
		static CBuiltinLogger_DiagnosticReporter defReporter;

		AX_EXPECT_MEMORY( m_pReporters.append( &defReporter ) );
	}
	CDiagnosticEngine::~CDiagnosticEngine()
	{
	}

	Void CDiagnosticEngine::diagnose( const Diagnostic &diag )
	{
		const EDiagnosticId id = diag.getId();
		AX_ASSERT( UPtr( id ) < arraySize( g_diagDetails ) );

		const SDiagnosticDetails &realDetails = g_diagDetails[ UPtr( id ) ];
		SDiagnosticDetails workingDetails = realDetails;

		if( workingDetails.sev == EDiagnosticSeverity::Warning ) {
			if( !warningsAreEnabled() ) {
				return;
			}

			if( warningsAreErrors() ) {
				workingDetails.sev = EDiagnosticSeverity::Error;
			} else {
				++m_cWarnings;
			}
		}

		if( workingDetails.sev == EDiagnosticSeverity::Error ) {
			const U32 cMaxErrors = errorsAreFatal() ? 1 : m_cMaxErrors;

			if( cMaxErrors > 0 && m_cErrors >= cMaxErrors ) {
				return;
			}

			++m_cErrors;
		}

		if( workingDetails.sev == EDiagnosticSeverity::Note && !notesAreEnabled() ) {
			return;
		}

		m_formatted.clear();
		AX_EXPECT_MEMORY( m_formatted.reserve( 1024 ) );

		if( scr_formatDiagnostic( m_formatted, workingDetails.msg, diag.getArgs() ) ) {
			workingDetails.msg = m_formatted;
		}

		for( IDiagnosticReporter *pReporter : m_pReporters ) {
			AX_ASSERT_NOT_NULL( pReporter );
			pReporter->report( diag, workingDetails );
		}
	}

	Void CDiagnosticEngine::diagnose( SourceRange range, EDiagnosticId id, TArr<DiagnosticArgument> args )
	{
		Diagnostic diag( id, args );

		diag.setLoc( range );

		if( range.cBytes > 1 ) {
			diag.addRange( range );
		}

		diagnose( diag );
	}
	Void CDiagnosticEngine::diagnose( const SToken &tok, EDiagnosticId id, TArr<DiagnosticArgument> args )
	{
		AX_ASSERT_MSG( tok.getSourceIndex() < m_pSources.num(), "Token has invalid source index" );

		const SourceRange range = {
			m_pSources[ tok.getSourceIndex() ],
			U32( tok.getOffset() ),
			U32( tok.getLength() )
		};

		diagnose( range, id, args );
	}

	CBuiltinLogger_DiagnosticReporter::CBuiltinLogger_DiagnosticReporter()
	: IDiagnosticReporter()
	{
	}
	CBuiltinLogger_DiagnosticReporter::~CBuiltinLogger_DiagnosticReporter()
	{
	}

	Void CBuiltinLogger_DiagnosticReporter::report( const Diagnostic &diag, const SDiagnosticDetails &details )
	{
		const Source *const pSrc = diag.getLoc().pSource;
		Str filename;
		U32 row = 0, col = 0;

		if( pSrc != nullptr ) {
			filename = pSrc->filename;
			scr_calcLineInfo( row, col, diag.getLoc() );
		}

		switch( details.sev ) {
		case EDiagnosticSeverity::Error:
			g_ErrorLog( filename, row, col ) += details.msg;
			break;

		case EDiagnosticSeverity::Warning:
			g_WarningLog( filename, row, col ) += details.msg;
			break;

		case EDiagnosticSeverity::Note:
			g_InfoLog( filename, row, col ) += details.msg;
			break;
		}

		// FIXME: Display source ranges
	}

}}
