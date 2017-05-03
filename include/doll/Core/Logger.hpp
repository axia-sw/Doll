#pragma once

#include "Defs.hpp"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifndef DOLL_DEFAULT_REPORT_CHANNEL
# define DOLL_DEFAULT_REPORT_CHANNEL 0
#endif

// You should redefine this in your own code when using logging macros like
// DOLL_ERROR_LOG and so on
#ifndef DOLL_TRACE_FACILITY
# define DOLL_TRACE_FACILITY DOLL_DEFAULT_REPORT_CHANNEL
#endif

#define DOLL__LOG_BASE 0x7000

namespace doll
{

	enum ELogFacility:int
	{
		// Internal/generic library logs (e.g., general "assert" or AxLib stuff)
		kLog_Internal = DOLL__LOG_BASE,
		// General API usage logs (e.g., performance tips or invalid parameters)
		kLog_API,

		// <core subsystems start>
		kLog__Core_S = DOLL__LOG_BASE + 0x0100,
			// Asynchronous IO subsystem
			kLog_CoreAsyncIO,
			// Configuration subsystem (messages can come from config file issues too)
			kLog_CoreConfig,
			// Virtual File System and its implementations' subsystems
			kLog_CoreVFS,
			// Memory Management subsystem ("out of memory" and budget reports, etc)
			kLog_CoreMemory,
		// <core subsystems end>
		kLog__Core_E,

		// <frontend subsystems start>
		kLog__Frontend_S = DOLL__LOG_BASE + 0x0200,
			// Frontend engine (de)initialization and update routines
			kLog_FrontendEngine,
			// Frontend input subsystem (complains about developer errors)
			kLog_FrontendInput,
			// Frontend setup subsystem (responsible for reading engine config files)
			kLog_FrontendSetup,
		// <frontend subsystems end>
		kLog__Frontend_E,

		// <graphics subsystems start>
		kLog__Gfx_S = DOLL__LOG_BASE + 0x0300,
			// Graphics "action" component subsystem
			kLog_GfxAction,
			// Graphics API subsystem (driver agnostic)
			kLog_GfxAPIMgr,
			// Graphics API implementation subsystem (driver-specific)
			kLog_GfxAPIDrv,
			// Graphics API implementation subsystem (messages from the underlying API; e.g., GL errors)
			kLog_GfxAPIDrvInt,
			// Graphics layers and layer effects subsystem
			kLog_GfxLayer,
			// Graphics OS text subsystem (responsible for drawing text using OS calls)
			kLog_GfxOSText,
			// Graphics primitive buffer subsystem (holds transient vertex data)
			kLog_GfxPrimitiveBuffer,
			// Graphics rendering commands subsystem (converts high-level actions into API calls and vertex data)
			kLog_GfxRenderCommands,
			// Graphics sprite subsystem (manages sprites / vector graphics)
			kLog_GfxSprite,
			// Graphics textures subsystem (files and texture/atlas support)
			kLog_GfxTexture,
		// <graphics subsystems end>
		kLog__Gfx_E,

		// <OS subsystems start>
		kLog__OS_S = DOLL__LOG_BASE + 0x0400,
			// OS application subsystem
			kLog_OSApp,
			// OS monitors/displays subsystem
			kLog_OSMonitor,
			// OS windowing subsystem
			kLog_OSWindow,
		// <OS subsystems end>
		kLog__OS_E,

		// <sound subsystems start>
		kLog__Snd_S = DOLL__LOG_BASE + 0x0500,
			// Sound core/general subsystem
			kLog_SndCore,
			// Sound API/driver subsystem (e.g., XAudio2)
			kLog_SndAPI,
			// Sound file subsystem (e.g., WAV/RIFF loader)
			kLog_SndFile,
			// Sound management subsystem (handles mixers and makes several processes easier)
			kLog_SndMgr,
		// <sound subsystems end>
		kLog__Snd_E,

		// <utility subsystems start>
		kLog__Util_S = DOLL__LOG_BASE + 0x0600,
			// Utility "frame counter" (i.e., interpolator) subsystem
			kLog_UtilCounter,
			// Utility "metrics" subsystem (e.g., "13mm" to pixels)
			kLog_UtilMetrics,
			// Utility "safe DX" subsystem (DirectX safety checks)
			kLog_UtilSafeDX,
		// <utility subsystems end>
		kLog__Util_E
	};

	/*
	===========================================================================

		REPORTER

		Handle logging and reports

	===========================================================================
	*/

	// Specifies the Severity of any given report
	enum class ESeverity
	{
		// Verbose (unnecessary) output
		Verbose,
		// No Severity, just a "normal" report (status, help, etc)
		Normal,
		// Debug text for development purposes
		Debug,
		// Indication of a better way to do something
		Hint,
		// A potentially unwanted or non-optimal situation was detected
		Warning,
		// A definitely unwanted or unworkable situation has occurred
		Error
	};

	// Report details
	struct SReportDetails
	{
		// The Severity of the report
		ESeverity severity;
		// Which subsystem the report is coming From (this is arbitrary)
		int       from;
		// Name of the file which the report is affecting (this can be NULL)
		Str       file;
		// Line number within the given file (ignored if 0 or if 'file' is NULL)
		U32       uLine;
		// Column number on the given uLine within the file (ignored if 0 or 'uLine' is ignored)
		U32       uColumn;
		// Name of the function the report concerns (this can be NULL)
		Str       function;

		inline SReportDetails()
		: severity( ESeverity::Normal )
		, from( DOLL_DEFAULT_REPORT_CHANNEL )
		, file()
		, uLine( 0 )
		, uColumn( 0 )
		, function( nullptr )
		{
		}
		inline SReportDetails( ESeverity Sev, Str file = Str(), U32 uLine = 0, U32 uColumn = 0, Str function = Str() )
		: severity( Sev )
		, from( DOLL_DEFAULT_REPORT_CHANNEL )
		, file( file )
		, uLine( uLine )
		, uColumn( uColumn )
		, function( function )
		{
		}
		inline SReportDetails( ESeverity Sev, Str file, U32 uLine, Str function )
		: severity( Sev )
		, from( DOLL_DEFAULT_REPORT_CHANNEL )
		, file( file )
		, uLine( uLine )
		, uColumn( 0 )
		, function( function )
		{
		}
		inline SReportDetails( ESeverity Sev, int From, Str file = Str(), U32 uLine = 0, U32 uColumn = 0, Str function = Str() )
		: severity( Sev )
		, from( From )
		, file( file )
		, uLine( uLine )
		, uColumn( uColumn )
		, function( function )
		{
		}
		inline SReportDetails( ESeverity Sev, int From, Str file, U32 uLine, Str function )
		: severity( Sev )
		, from( From )
		, file( file )
		, uLine( uLine )
		, uColumn( 0 )
		, function( function )
		{
		}
		inline SReportDetails( const SReportDetails &x )
		: severity( x.severity )
		, from( x.from )
		, file( x.file )
		, uLine( x.uLine )
		, uColumn( x.uColumn )
		, function( x.function )
		{
		}

		inline SReportDetails &operator=( const SReportDetails &x )
		{
			severity = x.severity;
			from = x.from;
			file = x.file;
			uLine = x.uLine;
			uColumn = x.uColumn;
			function = x.function;

			return *this;
		}
	};

	// Base interface class for reporters
	class IReporter
	{
	public:
		// UNDOC: Constructor
		IReporter()
		{
		}
		// UNDOC: Destructor
		virtual ~IReporter()
		{
		}

		// Handle a report
		//
		// Sev: Severity of the report (see ESeverity above)
		// file: File the report concerns
		// uLine: Line the report concerns
		// message: Description of the report
		virtual void report( const SReportDetails &details, Str message ) = 0;
	};

	// Submit a report to all listening reporters
	DOLL_FUNC void DOLL_API core_report( const SReportDetails &details, Str message );

	inline void core_report( ESeverity Sev, Str file, int uLine, Str message )
	{
		AX_ASSERT( message.isUsed() );

		SReportDetails details;

		details.severity = Sev;
		details.file     = file;
		details.uLine    = ( U32 )uLine;

		core_report( details, message );
	}

	// Add a reporter interface for handling reports
	DOLL_FUNC void DOLL_API core_addReporter( IReporter *r );

	// Remove an added reporter interface
	DOLL_FUNC void DOLL_API core_removeReporter( IReporter *r );

	// IReporter proxy
	class ReportProxy
	{
	public:
		inline ReportProxy()
		: mDetails( ESeverity::Normal )
		{
		}
		inline ReportProxy( const ReportProxy &x )
		: mDetails( x.mDetails )
		{
		}
		inline ReportProxy( ESeverity Sev, int From = DOLL_DEFAULT_REPORT_CHANNEL )
		: mDetails( Sev, From )
		{
		}
		inline ReportProxy( ESeverity Sev, Str file )
		: mDetails( Sev, file )
		{
		}
		inline ReportProxy( ESeverity Sev, int From, Str file, U32 uLine, U32 uColumn, Str function )
		: mDetails( Sev, From, file, uLine, uColumn, function )
		{
		}
		inline ReportProxy( const SReportDetails &details )
		: mDetails( details )
		{
		}
		inline ~ReportProxy()
		{
		}

		inline ReportProxy operator[]( int From ) const
		{
			return ReportProxy( mDetails.severity, From, mDetails.file, mDetails.uLine, mDetails.uColumn, mDetails.function );
		}
		inline ReportProxy operator()( Str file ) const
		{
			return ReportProxy( mDetails.severity, mDetails.from, file, mDetails.uLine, mDetails.uColumn, mDetails.function );
		}
		inline ReportProxy operator()( Str file, U32 uLine ) const
		{
			return ReportProxy( mDetails.severity, mDetails.from, file, uLine, mDetails.uColumn, mDetails.function );
		}
		inline ReportProxy operator()( Str file, U32 uLine, U32 uColumn ) const
		{
			return ReportProxy( mDetails.severity, mDetails.from, file, uLine, uColumn, mDetails.function );
		}
		inline ReportProxy operator()( Str file, U32 uLine, Str function ) const
		{
			return ReportProxy( mDetails.severity, mDetails.from, file, uLine, mDetails.uColumn, function );
		}
		inline ReportProxy operator()( Str file, U32 uLine, U32 uColumn, Str function ) const
		{
			return ReportProxy( mDetails.severity, mDetails.from, file, uLine, uColumn, function );
		}

		inline ReportProxy &submit( Str message )
		{
			core_report( mDetails, message );
			return *this;
		}

		inline ReportProxy &operator<<( Str message )
		{
			core_report( mDetails, message );
			return *this;
		}
		inline ReportProxy &operator+=( Str message )
		{
			core_report( mDetails, message );
			return *this;
		}

	private:
		SReportDetails mDetails;
	};

	// AX_TRACE(msg) submits a debug report concerning this code IF in debug mode
#if AX_DEBUG_ENABLED != 0
# define DOLL_TRACE( msg )\
	doll::core_report( doll::ESeverity::Debug, __FILE__, __LINE__, msg )
#else
# define DOLL_TRACE( msg )\
	( ( void )0 )
#endif

#define AX_DO_REPORT_F( _Sev_, _File_, _Line_, _Fmt_ )\
	char buf[ 1024 ];\
	\
	va_list args;\
	va_start( args, _Fmt_ );\
	axspfv( buf, sizeof( buf ), _Fmt_, args );\
	buf[ sizeof( buf ) - 1 ] = '\0';\
	va_end( args );\
	\
	core_report( _Sev_, _File_, _Line_, buf )

	// Report a warning
	inline void warnf( Str file, int uLine, const char *pszFmt, ... )
	{
		AX_ASSERT( file.isUsed() );
		AX_ASSERT_NOT_NULL( pszFmt );

		AX_DO_REPORT_F( ESeverity::Warning, file, uLine, pszFmt );
	}
	// Report an error
	inline void errorf( Str file, int uLine, const char *pszFmt, ... )
	{
		AX_ASSERT( file.isUsed() );
		AX_ASSERT_NOT_NULL( pszFmt );

		AX_DO_REPORT_F( ESeverity::Error, file, uLine, pszFmt );
	}
	// Report debugging information
	inline void debugf( Str file, int uLine, const char *pszFmt, ... )
	{
		AX_ASSERT( file.isUsed() );
		AX_ASSERT_NOT_NULL( pszFmt );

		AX_DO_REPORT_F( ESeverity::Debug, file, uLine, pszFmt );
	}
	// Report status
	inline void statusf( Str file, int uLine, const char *pszFmt, ... )
	{
		AX_ASSERT( file.isUsed() );
		AX_ASSERT_NOT_NULL( pszFmt );

		AX_DO_REPORT_F( ESeverity::Normal, file, uLine, pszFmt );
	}

	// Report a warning
	inline void warnf( Str file, const char *pszFmt, ... )
	{
		AX_ASSERT( file.isUsed() );
		AX_ASSERT_NOT_NULL( pszFmt );

		AX_DO_REPORT_F( ESeverity::Warning, file, 0, pszFmt );
	}
	// Report an error
	inline void errorf( Str file, const char *pszFmt, ... )
	{
		AX_ASSERT( file.isUsed() );
		AX_ASSERT_NOT_NULL( pszFmt );

		AX_DO_REPORT_F( ESeverity::Error, file, 0, pszFmt );
	}
	// Report debugging information
	inline void debugf( Str file, const char *pszFmt, ... )
	{
		AX_ASSERT( file.isUsed() );
		AX_ASSERT_NOT_NULL( pszFmt );

		AX_DO_REPORT_F( ESeverity::Debug, file, 0, pszFmt );
	}
	// Report status
	inline void statusf( Str file, const char *pszFmt, ... )
	{
		AX_ASSERT( file.isUsed() );
		AX_ASSERT_NOT_NULL( pszFmt );

		AX_DO_REPORT_F( ESeverity::Normal, file, 0, pszFmt );
	}

	// Report a warning
	inline void basicWarnf( const char *pszFmt, ... )
	{
		AX_ASSERT_NOT_NULL( pszFmt );

		AX_DO_REPORT_F( ESeverity::Warning, nullptr, 0, pszFmt );
	}
	// Report an error
	inline void basicErrorf( const char *pszFmt, ... )
	{
		AX_ASSERT_NOT_NULL( pszFmt );

		AX_DO_REPORT_F( ESeverity::Error, nullptr, 0, pszFmt );
	}
	// Report debugging information
	inline void basicDebugf( const char *pszFmt, ... )
	{
		AX_ASSERT_NOT_NULL( pszFmt );

		AX_DO_REPORT_F( ESeverity::Debug, nullptr, 0, pszFmt );
	}
	// Report status
	inline void basicStatusf( const char *pszFmt, ... )
	{
		AX_ASSERT_NOT_NULL( pszFmt );

		AX_DO_REPORT_F( ESeverity::Normal, nullptr, 0, pszFmt );
	}
	// Same as basicStatusf (report status) -- includes newline in output
	inline void notef( const char *pszFmt, ... )
	{
		AX_ASSERT_NOT_NULL( pszFmt );

		AX_DO_REPORT_F( ESeverity::Normal, nullptr, 0, pszFmt );
	}

	static ReportProxy g_VerboseLog( ESeverity::Verbose );
	static ReportProxy g_InfoLog( ESeverity::Normal );
	static ReportProxy g_DebugLog( ESeverity::Debug );
	static ReportProxy g_HintLog( ESeverity::Hint );
	static ReportProxy g_WarningLog( ESeverity::Warning );
	static ReportProxy g_ErrorLog( ESeverity::Error );

#define DOLL_REFERENCE_LOG( Kind, From )\
	doll::g_##Kind##Log[ From ]( __FILE__, __LINE__, AX_PRETTY_FUNCTION )

#define DOLL_VERBOSE_LOG DOLL_REFERENCE_LOG(Verbose, DOLL_TRACE_FACILITY)
#define DOLL_INFO_LOG    DOLL_REFERENCE_LOG(Info   , DOLL_TRACE_FACILITY)
#define DOLL_DEBUG_LOG   DOLL_REFERENCE_LOG(Debug  , DOLL_TRACE_FACILITY)
#define DOLL_HINT_LOG    DOLL_REFERENCE_LOG(Hint   , DOLL_TRACE_FACILITY)
#define DOLL_WARNING_LOG DOLL_REFERENCE_LOG(Warning, DOLL_TRACE_FACILITY)
#define DOLL_ERROR_LOG   DOLL_REFERENCE_LOG(Error  , DOLL_TRACE_FACILITY)

#undef vsprintf_s

	// Install the debug.log reporter
	DOLL_FUNC void DOLL_API core_installDebugLogReporter();
	// Uninstall the debug.log reporter
	DOLL_FUNC void DOLL_API core_uninstallDebugLogReporter();

	// Install the colored console reporter
	DOLL_FUNC void DOLL_API core_installConsoleReporter();
	// Uninstall the colored console reporter
	DOLL_FUNC void DOLL_API core_uninstallConsoleReporter();

}
