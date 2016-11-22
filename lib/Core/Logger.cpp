#if defined( _WIN32 )
# undef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN 1
# include <Windows.h>
# undef min
# undef max
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

#include "doll/Core/Logger.hpp"
#include "doll/Core/Engine.hpp"
#include "doll/Core/Version.hpp"

#include "doll/Front/Frontend.hpp"

namespace doll
{

	// Get the reporters list
	static TList< IReporter * > &ReportersList()
	{
		static TList< IReporter * > instance;
		return instance;
	}

	// Submit a report to all listening reporters
	DOLL_FUNC void DOLL_API core_report( const SReportDetails &details, Str message )
	{
		AX_ASSERT( message.isUsed() );

		auto &reporters = ReportersList();

		bool found = false;
		for( IReporter *&r : reporters ) {
			if( !r ) {
				continue;
			}
			
			found = true;
			r->report( details, message );
		}
		
		if( !found ) {
			const ESeverity sev  = details.severity;
			const int From       = details.from;
			const Str file       = details.file;
			const uint32 uLine   = file.isUsed() ? details.uLine : 0;
			const uint32 uColumn = uLine > 0 ? details.uColumn : 0;
			const Str func       = file.isUsed() ? details.function : Str();

			const char *const sevMsg =
				sev == ESeverity::Error   ? "Error"   :
				sev == ESeverity::Warning ? "Warning" :
				sev == ESeverity::Hint    ? "Hint"    :
				sev == ESeverity::Debug   ? "Debug"   :
				sev == ESeverity::Normal  ? "Normal"  :
				"(unknown-Severity)";
			axerrf
			(
				"%s from:%i file<%.*s> line:%u column:%u function<%.*s> :: %.*s\n"
				, sevMsg
				, From
				, file.len()
				, file.len() > 0 ? file.get() : ""
				, uLine
				, uColumn
				, func.len()
				, func.len() > 0 ? func.get() : ""
				, message.len()
				, message.get()
			);
		}
	}
	// Add a reporter interface for handling reports
	DOLL_FUNC void DOLL_API core_addReporter( IReporter *r )
	{
		if( !r ) {
			return;
		}

		auto &reporters = ReportersList();

		for( IReporter *&x : reporters ) {
			if( x == r ) {
				return;
			}
		}
		
		reporters.addTail( r );
	}
	// Remove an added reporter interface
	DOLL_FUNC void DOLL_API core_removeReporter( IReporter *r )
	{
		if( !r ) {
			return;
		}
		
		auto &reporters = ReportersList();

		auto end = reporters.end();
		auto start = reporters.begin();
		for( auto iter = start; iter != end; ++iter ) {
			if( *iter == r ) {
				reporters.remove( iter );
				return;
			}
		}
	}

	/*
	===========================================================================
	
		UTILITY :: DEBUG LOG REPORTER (REPORTER IMPLEMENTATION)

		Writes reports to "debug.log"
	
	===========================================================================
	*/
	class DebugLogReporter: public virtual IReporter
	{
	public:
		// Register this reporter with the internal reporting system
		static void install()
		{
			core_addReporter( &get() );
		}
		// Remove this reporter From the internal reporting system
		static void uninstall()
		{
			core_removeReporter( &get() );
		}

		// UNDOC: Handle a report
		virtual void report( const SReportDetails &details, Str message )
		{
			if( !m_pFile ) {
				static const char *const pszLogName = "AppDebug.log";
#ifdef _WIN32
				wchar_t wszLogName[ 2048 ];
#endif
				char szTemp[ 2048 ];

				const Str logDir( core_getLogDir() );

				if( logDir.isUsed() ) {
					axstr_cpy( szTemp, logDir );
					axstr_catpath( szTemp, pszLogName );
				} else {
					axstr_cpy( szTemp, pszLogName );
				}

#ifdef _WIN32
				Str( szTemp ).toWStr( wszLogName );

# if DOLL__SECURE_LIB
				if( _wfopen_s( &m_pFile, wszLogName, L"a+" ) != 0 ) {
					return;
				}
# else
				if( !( m_pFile = _wfopen( wszLogName, L"a+" ) ) ) {
					return;
				}
# endif
#else
				if( !( m_pFile = fopen( szTemp, "a+" ) ) ) {
					return;
				}
#endif

				fseek( m_pFile, 0, SEEK_END );
				const long x = ftell( m_pFile );

				if( !x ) {
					static const U8 bom[] = { 0xEF, 0xBB, 0xBF };
					fwrite( &bom[0], sizeof(bom), 1, m_pFile );
				} else {
					axfpf
					(
						m_pFile,
						"\n\n\n--------------------------------------------------\n\n\n\n"
					);
				}

				const Str appPath = app_getPath();

				axfpf( m_pFile, "Engine Version: %s\n", doll_getEngineString() );
				axfpf( m_pFile, "    Build Date: %u\n", unsigned(AX_BUILD_DATE) );
				axfpf( m_pFile, "      App Path: %.*s\n", appPath.lenInt(), appPath.get() );
				axfpf( m_pFile, "\n\n" );
			}

			const ESeverity sev  = details.severity;
			const int From       = details.from;
			const Str file       = details.file;
			const uint32 uLine   = file.isUsed() ? details.uLine : 0;
			const uint32 uColumn = uLine > 0 ? details.uColumn : 0;
			const Str func       = file.isUsed() ? details.function : Str();

			( void )From; //not handling where a report comes from, currently
			
			const char *sevMsg = "";
			switch( sev ) {
			case ESeverity::Verbose:
				break;
			case ESeverity::Normal:
				break;
					
			case ESeverity::Debug:
				sevMsg = "***DEBUG*** ";
				break;
			case ESeverity::Hint:
				sevMsg = "Hint. ";
				break;
			case ESeverity::Warning:
				sevMsg = "Warning. ";
				break;
			case ESeverity::Error:
				sevMsg = "Error. ";
				break;
			}

			const char *const x = func.isUsed() ? "in " : "";
			const int yn        = func.lenInt();
			const char *const y = func.get();
			const char *const z = func.isUsed() ? ": " : "";

			const int mn         = message.lenInt();
			const char *const mp = message.get();

			const int fn         = file.lenInt();
			const char *const fs = file.get();

			char szBuf[1024];
			
			axpf_ptrdiff_t sn = 0;
			if( file.isUsed() ) {
				if( uLine > 0 ) {
					if( uColumn > 0 ) {
						sn = axspf( szBuf, "[%.*s(%u:%u)]\n\t%s%s%.*s%s%.*s\n\n",
							fn,fs, uLine, uColumn, sevMsg, x, yn,y, z, mn, mp );
					} else {
						sn = axspf( szBuf, "[%.*s(%u)]\n\t%s%s%.*s%s%.*s\n\n",
							fn,fs, uLine, sevMsg, x, yn,y, z, mn, mp );
					}
				} else {
					sn = axspf( szBuf, "[%.*s]\n\t%s%s%.*s%s%.*s\n\n",
						fn,fs, sevMsg, x, yn,y, z, mn, mp );
				}
			} else {
				sn = axspf( szBuf, "%s%s%.*s%s%.*s\n", sevMsg, x, yn,y, z, mn, mp );
			}

			fwrite( &szBuf[ 0 ], size_t( sn ), 1, m_pFile );
			if( sev == ESeverity::Error ) {
				fflush( m_pFile );
			}
		}

	private:
		FILE *m_pFile;

		// UNDOC: Singleton
		static DebugLogReporter &get()
		{
			static DebugLogReporter instance;
			return instance;
		}

		// UNDOC: Constructor
		DebugLogReporter()
		: IReporter()
		, m_pFile( nullptr )
		{
		}
		// UNDOC: Destructor
		virtual ~DebugLogReporter()
		{
			core_removeReporter( this );
			if( m_pFile != nullptr ) {
				axfpf( m_pFile, "\n--log closed--\n" );

				fclose( m_pFile );
				m_pFile = nullptr;
			}
		}
	};

	DOLL_FUNC void DOLL_API core_installDebugLogReporter()
	{
		DebugLogReporter::install();
	}
	DOLL_FUNC void DOLL_API core_uninstallDebugLogReporter()
	{
		DebugLogReporter::uninstall();
	}

	/*
	===========================================================================
	
		UTILITY :: CONSOLE REPORTER (REPORTER IMPLEMENTATION)

		Implements colored console output for reports
	
	===========================================================================
	*/
	class ConsoleReporter: public virtual IReporter
	{
	public:
		// Register this reporter with the internal reporting system
		static void install()
		{
			core_addReporter( &get() );
		}
		// Remove this reporter From the internal reporting system
		static void uninstall()
		{
			core_removeReporter( &get() );
		}

#define VS_STYLE_REPORT 1

		// UNDOC: Handle a report
		virtual void report( const SReportDetails &details, Str message )
		{
			const ESeverity sev  = details.severity;
			const int From       = details.from;
			const Str file       = details.file;
			const uint32 uLine   = file.isUsed() ? details.uLine : 0;
			const uint32 uColumn = uLine > 0 ? details.uColumn : 0;
			const Str func       = file.isUsed() ? details.function : Str();

			( void )From; //not handling where a report is From, currently

			const unsigned char curCol = getCurrentColors( mStdErr );
			const unsigned char bgCol = curCol & 0xF0;

			Color sevCol = kColor_White;
			const char *sevMsg = nullptr;
			
			switch( sev ) {
			case ESeverity::Verbose:
				break;
			case ESeverity::Normal:
				break;
				
			case ESeverity::Debug:
				sevCol = kColor_LightMagenta;
				sevMsg = "***DEBUG***";
				break;
			case ESeverity::Warning:
				sevCol = kColor_LightBrown;
				sevMsg = "WARNING";
				break;
			case ESeverity::Hint:
				sevCol = kColor_LightGreen;
				sevMsg = "HINT";
				break;
			case ESeverity::Error:
				sevCol = kColor_LightRed;
				sevMsg = "ERROR";
				break;
			}
			
#if !VS_STYLE_REPORT
			if( sevMsg != nullptr ) {
				setCurrentColors( mStdErr, sevCol | bgCol );
				writeString( mStdErr, sevMsg );

				setCurrentColors( mStdErr, curCol );
				writeString( mStdErr, ": " );
			}
#endif

			if( file.isUsed() ) {
				char buf[ 1024 ];
#if !VS_STYLE_REPORT
				setCurrentColors( mStdErr, kColor_Cyan | bgCol );
				writeString( mStdErr, "[" );
#endif

				setCurrentColors( mStdErr, kColor_White ); //black bg
				axpf_ptrdiff_t bufn = axspf( buf, "%/.*s", file.lenInt(), file.get() );
				if( bufn > 0 ) {
					writeString( mStdErr, Str( buf, &buf[ bufn ] ) );
				}
				
				if( uLine > 0 ) {
					setCurrentColors( mStdErr, kColor_DarkGray | bgCol );
					writeString( mStdErr, "(" );

					char *p = &buf[ arraySize( buf ) - 1 ];
					*p = '\0';

					uint32 n = uLine;
					while( n > 0 ) {
						AX_ASSERT( p > &buf[ 0 ] );

						*--p = ( char )( '0' + n%10 );
						n /= 10;
					}
					
					setCurrentColors( mStdErr, kColor_LightGray ); //black bg
					writeString( mStdErr, p ); //uLine number

					if( uColumn > 0 ) {
						p = &buf[ arraySize( buf ) - 1 ];

						n = uColumn;
						while( n > 0 ) {
							AX_ASSERT( p > &buf[ 0 ] );

							*--p = ( char )( '0' + n%10 );
							n /= 10;
						}

						setCurrentColors( mStdErr, kColor_DarkGray );
						writeString( mStdErr, "," );

						setCurrentColors( mStdErr, kColor_LightGray );
						writeString( mStdErr, p ); //uColumn number
					}

					setCurrentColors( mStdErr, kColor_DarkGray | bgCol );
					writeString( mStdErr, ")" );
				}

#if !VS_STYLE_REPORT
				setCurrentColors( mStdErr, kColor_Cyan | bgCol );
				writeString( mStdErr, "] " );
#else
				setCurrentColors( mStdErr, curCol );
				writeString( mStdErr, ": " );
#endif
			}

#if VS_STYLE_REPORT
			if( sevMsg != nullptr ) {
				setCurrentColors( mStdErr, sevCol | bgCol );
				writeString( mStdErr, sevMsg );

				setCurrentColors( mStdErr, curCol );
				writeString( mStdErr, ": " );
			}
#endif

			if( func.isUsed() ) {
				setCurrentColors( mStdErr, curCol );
				writeString( mStdErr, "in " );

				setCurrentColors( mStdErr, kColor_White );
				writeString( mStdErr, func );

				setCurrentColors( mStdErr, curCol );
				writeString( mStdErr, ": " );
			}

			setCurrentColors( mStdErr, curCol );
			writeString( mStdErr, message );
			writeString( mStdErr, "\n" );

#if defined( _WIN32 )
			if( sev != ESeverity::Error ) {
				return;
			}

			char buf[ 1024 ];

			const UPtr mn = message.len();
			const char *const mp = message.get();

			if( file.isUsed() ) {
				if( uLine > 0 ) {
					axspf( buf, sizeof( buf ),
						"File: %/.*s\nLine: %u\n\n%.*s",
						file.len(),file.get(), uLine, mn, mp );
				} else {
					axspf( buf, sizeof( buf ), "[File: %/.*s]\n%.*s",
						file.len(),file.get(), mn, mp );
				}
			} else {
				axspf( buf, sizeof( buf ), "%.*s", mn, mp );
			}
			buf[ sizeof( buf ) - 1 ] = '\0';

			const Str appName = app_getPath().getBasename();

			char title[ 512 ];
			axspf( title, sizeof( title ), "Error - %.*s", appName.len(), appName.get() );

			const HWND wnd = NULL;
			const UINT icon = MB_ICONERROR;

			if( IsDebuggerPresent() ) {
				axstr_cat( buf, sizeof( buf ), "\n\nTrigger breakpoint?" );

				wchar_t wszBuf[ 1024 ];
				wchar_t wszTitle[ 96 ];

				Str( buf ).toWStr( wszBuf );
				//MultiByteToWideChar( CP_UTF8, 0, buf, strlen(buf)+1, wszBuf, sizeof(wszBuf)/sizeof(wszBuf[0]) );
				Str( title ).toWStr( wszTitle );

				const int r = MessageBoxW( wnd, wszBuf, wszTitle, icon | MB_YESNO );

				if( r == IDYES ) {
					DebugBreak();
				}
			}
#endif
		}

	private:
		// Color codes used internally
		//
		// NOTE: If used as-is then these represent foreground (text) colors
		//       with a black background color.
		enum Color : unsigned char
		{
			kColor_Black		= 0x0,
			kColor_Blue			= 0x1,
			kColor_Green		= 0x2,
			kColor_Cyan			= 0x3,
			kColor_Red			= 0x4,
			kColor_Magenta		= 0x5,
			kColor_Brown		= 0x6,
			kColor_LightGrey	= 0x7,
			kColor_DarkGrey		= 0x8,
			kColor_LightBlue	= 0x9,
			kColor_LightGreen	= 0xA,
			kColor_LightCyan	= 0xB,
			kColor_LightRed		= 0xC,
			kColor_LightMagenta	= 0xD,
			kColor_LightBrown	= 0xE,
			kColor_White		= 0xF,
			
			kColor_Purple		= kColor_Magenta,
			kColor_Yellow		= kColor_LightBrown,
			kColor_Violet		= kColor_LightMagenta,

			kColor_LightGray	= kColor_LightGrey,
			kColor_DarkGray		= kColor_DarkGrey
		};

#if defined( _WIN32 )
		typedef HANDLE FileHandle;
#else
		typedef FILE *FileHandle;
#endif
		FileHandle mStdOut;
		FileHandle mStdErr;

		// UNDOC: Singleton
		static ConsoleReporter &get()
		{
			static ConsoleReporter instance;
			return instance;
		}

		// UNDOC: Constructor
		ConsoleReporter()
		: IReporter()
		, mStdOut( NULL )
		, mStdErr( NULL )
		{
#if defined( _WIN32 )
			mStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
			if( mStdOut == INVALID_HANDLE_VALUE ) {
				mStdOut = ( FileHandle )0;
			}

			mStdErr = GetStdHandle( STD_ERROR_HANDLE );
			if( mStdErr == INVALID_HANDLE_VALUE ) {
				mStdErr = ( FileHandle )0;
			}
#else
			mStdOut = stdout;
			mStdErr = stderr;
#endif

			core_addReporter( this );
		}
		// UNDOC: Destructor
		virtual ~ConsoleReporter()
		{
			core_removeReporter( this );
		}
		
		// No copy constructor
		ConsoleReporter( const ConsoleReporter & );
		// No assignment operator
		ConsoleReporter &operator=( const ConsoleReporter & );

		// Retrieve the current colors of the given console handle
		static unsigned char getCurrentColors( FileHandle f )
		{
			if( !f ) {
				return 0x07;
			}
			
#if defined( _WIN32 )
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			
			if( GetConsoleScreenBufferInfo( f, &csbi ) ) {
				return csbi.wAttributes & 0xFF;
			}
#else
			//
			//	TODO: Support this on GNU/Linux...
			//
#endif

			return 0x07;
		}
		// Set the current colors for the given console handle
		static void setCurrentColors( FileHandle f, unsigned char colors )
		{
			if( !f ) {
				return;
			}

#if defined( _WIN32 )
			SetConsoleTextAttribute( f, ( WORD )colors );
#else
			//
			//	MAP ORDER (0-7):
			//	Black, Blue, Green, Cyan, Red, Magenta, Yellow, Grey
			//
			//	TERMINAL COLOR ORDER (0-7):
			//	Black, Red, Green, Yellow, Blue, Magenta, Cyan, Grey
			//
			static const char *const mapF[16] = {
				"\x1b[30;22m", "\x1b[34;22m", "\x1b[32;22m", "\x1b[36;22m",
				"\x1b[31;22m", "\x1b[35;22m", "\x1b[33;22m", "\x1b[37;22m",
				"\x1b[30;1m", "\x1b[34;1m", "\x1b[32;1m", "\x1b[36;1m",
				"\x1b[31;1m", "\x1b[35;1m", "\x1b[33;1m", "\x1b[37;1m"
			};
			
			const char *const code = mapF[ colors & 0x0F ];
			fwrite( ( const void * )code, strlen( code ), 1, f );
#endif
		}
		// Write a string of text to the given console handle
		static void writeString( FileHandle f, Str s )
		{
#if defined( _WIN32 )
			FILE *fp = stdout;
			if( f == GetStdHandle( STD_ERROR_HANDLE ) ) {
				fp = stderr;
			}

			axfpf( fp, "%.*s", s.lenInt(), s.get() );
#else
			axfpf( f, "%.*s", s.lenInt(), s.get() );
#endif
		}
	};

	DOLL_FUNC void DOLL_API core_installConsoleReporter()
	{
		ConsoleReporter::install();
	}
	DOLL_FUNC void DOLL_API core_uninstallConsoleReporter()
	{
		ConsoleReporter::uninstall();
	}

}
