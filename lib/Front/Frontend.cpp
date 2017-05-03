#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN 1
# include <Windows.h>
# include <Objbase.h>
# undef min
# undef max
# include <io.h>
#endif

#define DOLL_TRACE_FACILITY doll::kLog_FrontendEngine

#include "doll/Front/Frontend.hpp"
#include "doll/Front/Input.hpp"
#include "doll/Front/Setup.hpp"

#include "doll/Core/Defs.hpp"

#include "doll/Core/Config.hpp"
#include "doll/Core/Logger.hpp"
#include "doll/Core/Version.hpp"

#include "doll/Gfx/Action.hpp"
#include "doll/Gfx/API.hpp"
#include "doll/Gfx/Layer.hpp"
#include "doll/Gfx/OSText.hpp"
#include "doll/Gfx/RenderCommands.hpp"
#include "doll/Gfx/Sprite.hpp"

#include "doll/IO/AsyncIO.hpp"
#include "doll/IO/VFS.hpp"

#include "doll/OS/App.hpp"
#include "doll/OS/Monitor.hpp"
#include "doll/OS/OpenGL.hpp"
#include "doll/OS/Window.hpp"

#include "doll/Snd/SoundCore.hpp"

#include "doll/Util/Counter.hpp"

#include "doll/UX/Widget.hpp"

#include "doll/private/gitinfo.gen.h"

#include <stdlib.h>
#include <sys/stat.h>

#ifndef _WIN32
// Needed for isatty()
# include <unistd.h>
// Used for nanosleep()
# include <time.h>
#endif

#if DOLL__USE_GLFW
# if AX_OS_WINDOWS
#  define GLFW_EXPOSE_NATIVE_WIN32 1
# elif AX_OS_LINUX
#  define GLFW_EXPOSE_NATIVE_X11   1
# elif AX_OS_MACOSX
#  define GLFW_EXPOSE_NATIVE_COCOA 1
# else
#  error "Unhandled GLFW platform"
# endif
# include <GLFW/glfw3.h>
# include <GLFW/glfw3native.h>
#endif

#if AX_OS_MACOSX
typedef void *NSWindow;
#endif

namespace doll
{

#if 0 //unused
	static char g_szFilenameBuf[ 65536*2 ] = { 0 };
	static UPtr g_uFilenameBuf = 0;

	static Bool addFilename( Str &dstfilename, const Str &filename )
	{
		if( g_uFilenameBuf >= sizeof( g_szFilenameBuf ) + filename.len() ) {
			return false;
		}

		memcpy( &g_szFilenameBuf[ g_uFilenameBuf ], ( const Void * )filename.get(), filename.len() );
		dstfilename = Str( &g_szFilenameBuf[ g_uFilenameBuf ], filename.len() );

		g_uFilenameBuf += filename.len();
		g_szFilenameBuf[ g_uFilenameBuf++ ] = '\0';

		return true;
	}
#endif

#if DOLL__USE_GLFW
	static void glfw_error_f( int error, const char *description )
	{
		axerrf( "GLFW error %i: %s\n", error, description );
		basicErrorf( "GLFW error %i: %s\n", error, description );
	}
#endif

	static EWndReply onClosed_f( OSWindow )
	{
		DOLL_DEBUG_LOG += "Window closed";
#if DOLL__USE_GLFW
		glfwSetWindowShouldClose( g_core.view.window, GLFW_TRUE );
#else
		os_submitQuitEvent();
#endif
		return EWndReply::Handled;
	}
	static EWndReply onSized_f( OSWindow, U32 clientResX, U32 clientResY )
	{
		if( g_core.view.pGfxFrame != nullptr ) {
			g_core.view.pGfxFrame->resize( clientResX, clientResY );

			gfx_setLayerSize( gfx_getDefaultLayer(), clientResX, clientResY );
		}

		return EWndReply::Handled;
	}

#if DOLL__USE_GLFW
	static void glfw_onClosed_f( GLFWwindow * )
	{
		(void)onClosed_f( OSWindow(0) );
	}
	static void glfw_onSized_f( GLFWwindow *, int w, int h )
	{
		(void)onSized_f( OSWindow(0), U32( S32( w ) ), U32( S32( h ) ) );
	}
#endif

#if DOLL__USE_GLFW
	static SIntVector2 glfw_getDesktopSize()
	{
		GLFWmonitor *const monitor = glfwGetPrimaryMonitor();
		if( !monitor ) {
			return SIntVector2();
		}

		const GLFWvidmode *const vidmode = glfwGetVideoMode( monitor );
		if( !vidmode ) {
			return SIntVector2();
		}

		return SIntVector2( S32( vidmode->width ), S32( vidmode->height ) );
	}
#endif

#if DOLL__USE_GLFW
	static U32 glfw_cvtmods( int mods )
	{
		U32 cvtmods = 0;

		if( mods & GLFW_MOD_SHIFT ) {
			cvtmods |= kMF_LShift;
		}
		if( mods & GLFW_MOD_CONTROL ) {
			cvtmods |= kMF_LControl;
		}
		if( mods & GLFW_MOD_ALT ) {
			cvtmods |= kMF_LAlt;
		}

		return cvtmods;
	}
	static void glfw_getMouse( GLFWwindow *window, S32 &dstClientPosX, S32 &dstClientPosY )
	{
		double mousePosX, mousePosY;

		glfwGetCursorPos( window, &mousePosX, &mousePosY );

		dstClientPosX = S32( mousePosX );
		dstClientPosY = S32( mousePosY );
	}
	static EMouse glfw_getMouseButton( int button )
	{
		// GLFW_MOUSE_BUTTON_1 = 0, GLFW_MOUSE_BUTTON_8 = 7
		// EMouse::Button1 = 1, EMouse::Button8 = 8

		return (EMouse)( button + 1 );
	}

	static void glfw_windowFocus_f( GLFWwindow *, int focused )
	{
		if( focused ) {
			(void)in_onAcceptKey_f( OSWindow(0) );
		} else {
			(void)in_onResignKey_f( OSWindow(0) );
		}
	}

	static void glfw_mouseButton_f( GLFWwindow *window, int inbutton, int action, int inmods )
	{
		S32 clientPosX, clientPosY;
		glfw_getMouse( window, clientPosX, clientPosY );

		const EMouse button = glfw_getMouseButton( inbutton );
		const U32 mods = glfw_cvtmods( inmods );

		if( action ) {
			(void)in_onMousePress_f( OSWindow(0), button, clientPosX, clientPosY, mods );
		} else {
			(void)in_onMouseRelease_f( OSWindow(0), button, clientPosX, clientPosY, mods );
		}
	}
	static void glfw_mousePos_f( GLFWwindow *, double x, double y )
	{
		const S32 clientPosX = S32( x );
		const S32 clientPosY = S32( y );

		// FIXME: Use current mods
		const U32 mods = 0;

		(void)in_onMouseMove_f( OSWindow(0), clientPosX, clientPosY, mods );
	}
	static void glfw_mouseEnterLeave_f( GLFWwindow *, int entered )
	{
		if( !entered ) {
			// FIXME: Use current mods
			const U32 mods = 0;

			(void)in_onMouseExit_f( OSWindow(0), mods );
		}
	}
	static void glfw_mouseScroll_f( GLFWwindow *window, double x, double y )
	{
		static const double epsilon = 1e-6;

		((void)x);

		if( y >= -epsilon && y <= epsilon ) {
			return;
		}

		const F32 delta = F32( y );

		S32 clientPosX, clientPosY;
		glfw_getMouse( window, clientPosX, clientPosY );

		// FIXME: Use current mods
		const U32 mods = 0;

		(void)in_onMouseWheel_f( OSWindow(0), delta, clientPosX, clientPosY, mods );
	}

	static void glfw_keyButton_f( GLFWwindow *, int, int scancode, int action, int inmods )
	{
		const EKey key =
#if AX_OS_LINUX
			EKey( scancode - 1 ) // X11 just maps to +1
#else
			EKey( scancode )
#endif
			;
		const U32 mods = glfw_cvtmods( inmods );
		const Bool isRepeat = action == GLFW_REPEAT;

		if( action ) {
			(void)in_onKeyPress_f( OSWindow(0), key, mods, isRepeat );
		} else {
			(void)in_onKeyRelease_f( OSWindow(0), key, mods );
		}
	}
	static void glfw_keyChar_f( GLFWwindow *, unsigned int codepoint )
	{
		(void)in_onKeyChar_f( OSWindow(0), U32( codepoint ) );
	}
#endif

	DOLL_FUNC const char *DOLL_API doll_getEngineString()
	{
		static char szBuf[ 256 ] = { '\0' };

		if( szBuf[ 0 ] == '\0' ) {
			const char *const pszDbgStr = variantToString( kVariant );

#if DOLL_GITINFO__REVNUM != 0
# define DOLL__GITINFO_FMT " %s-r%u (%s %s)"
# define DOLL__GITINFO_ARG , DOLL_GITINFO__BRANCH, DOLL_GITINFO__REVNUM \
/* */                      , DOLL_GITINFO__TSTAMP, DOLL_GITINFO__COMMIT
#else
# define DOLL__GITINFO_FMT ""
# define DOLL__GITINFO_ARG
#endif

			axspf
			(
				szBuf,
				"Doll %s %u.%u.%u" DOLL__GITINFO_FMT,
				pszDbgStr,
				kVersionMajor,
				kVersionMinor,
				kVersionPatch
				DOLL__GITINFO_ARG
			);
		}

		return szBuf;
	}

	static Void parseDir( ECoreDir dstDir, TArr<SCoreDir> coreDirs, const Str &dirfmt, const Str &defval )
	{
		AX_ASSERT( coreDirs.isUsed() );
		AX_ASSERT( UPtr( U32( dstDir ) ) < coreDirs.num() );

		MutStr result;
		Str fmt( dirfmt.isUsed() ? dirfmt : defval );

		if( fmt.startsWith( '$' ) ) {
			const UPtr cCoreDirs = coreDirs.num();
			for( UPtr i = 0; i < cCoreDirs; ++i ) {
				const SCoreDir &coreDir = coreDirs[ i ];
				if( !fmt.caseStartsWith( coreDir.prefix ) ) {
					continue;
				}

				fmt = fmt.skip( coreDir.prefix.len() );
				AX_EXPECT_MEMORY( result.tryAssign( coreDir.path ) );
				break;
			}
		}

		if( result.isEmpty() && fmt.getRoot().isEmpty() ) {
			AX_EXPECT_MEMORY( result.tryAssign( app_getDir() ) );
		}

		AX_EXPECT_MEMORY( result.tryAppendPath( fmt ) );

		const_cast<SCoreDir&>(coreDirs[ U32( dstDir ) ]).path.swap( result );
	}

	static Void prepareDir( ECoreDir dir, TArr<SCoreDir> coreDirs, Str confDir, const Str &defval )
	{
		const UPtr dirIndex = UPtr( U32( dir ) );

		AX_ASSERT( coreDirs.isUsed() );
		AX_ASSERT( dirIndex < coreDirs.num() );

		parseDir( dir, coreDirs, confDir, defval );

		const SCoreDir &coreDir = coreDirs[ dirIndex ];

		if( coreDir.isWritable() && !sysfs_mkdir( coreDir.path ) ) {
			char szBuf[ 512 ];
			g_ErrorLog( coreDir.path ) +=
				axff( szBuf, "Failed to create <%s> directory", core_getDirLongName( dir ) );
		}
	}

#ifdef _WIN32
	static Str getEnv( const char *envvarname ) {
		static TSmallStr< 1024 > buf;

		if( !envvarname ) {
			buf.purge();
			return Str();
		}

# if defined( _MSC_VER ) && defined( __STDC_WANT_SECURE_LIB__ )
		size_t requiredSize = 0;
		( void )getenv_s( &requiredSize, nullptr, 0, envvarname );
		AX_EXPECT_MEMORY( buf.reserveAndSetLen( requiredSize ) );
		const int e = getenv_s( &requiredSize, buf.data(), buf.len() + 1, envvarname );
		if( e != 0 ) {
			buf.clear();
			return Str();
		}

# else
		const char *const p = getenv( envvarname );
		if( !p ) {
			return Str();
		}
# endif

		return buf.view();
	}
	static Bool checkEnv( const char *envvarname, const char *srchval ) {
		AX_ASSERT_NOT_NULL( envvarname );
		AX_ASSERT_NOT_NULL( srchval );

		return getEnv( envvarname ).caseFind( srchval ) != -1;
	}
	static Bool checkEnv( const char *envvarname ) {
		AX_ASSERT_NOT_NULL( envvarname );

		return getEnv( envvarname ).get() != Str().get();
	}
	static Bool isMSys() {
		static const Bool r =
			checkEnv( "MSYSCON" ) ||
			checkEnv( "_", "msys" );
		return r;
	}
	static Bool isCygwin() {
#ifdef __CYGWIN__
		return true;
#else
		return false;
#endif
	}
#endif
	static Bool isANSICompatibleEnvironment() {
#ifdef _WIN32
		static const Bool isANSICompat =
			isMSys() ||
			isCygwin();
		return isANSICompat;
#else
		return true;
#endif
	}
#ifdef _WIN32
	static Bool isConsoleOwned() {
		DWORD pid = 0;

		GetWindowThreadProcessId( GetConsoleWindow(), &pid );
		return GetCurrentProcessId() == pid;
	}
#endif
	static EConsoleType getConsoleType( FILE *stdhandle ) {
		AX_ASSERT_MSG( isOneOf( stdhandle, stdout, stderr, stdin ), "Invalid standard handle" );

		const int stdhandleno =
#ifdef _WIN32
			_fileno( stdhandle )
#else
			fileno( stdhandle )
#endif
			;

#ifdef _WIN32
		static const Bool isANSICompat = isANSICompatibleEnvironment();
		static const Bool isOwned = isConsoleOwned();
		// FIXME: This isn't entirely accurate...
		const Bool isTerm = isMSys() || _isatty( stdhandleno ) != 0;
#else
		const Bool isTerm = isatty( stdhandleno ) != 0;
#endif

		if( isTerm ) {
#ifdef _WIN32
			if( isANSICompat ) {
				return kConsoleTypeANSI;
			}

			return isOwned ? kConsoleTypeWinconOwned : kConsoleTypeWinconUsing;
#else
			return kConsoleTypeANSI;
#endif
		}

		// Do we have a file at all though?
		{
			struct stat s;
			if( !stdhandle || fstat( stdhandleno, &s ) != 0 ) {
				return kConsoleTypeNone;
			}
		}

		// For our purposes, assume this is just a standard file
		return kConsoleTypeFile;
	}
	static Bool isConsoleTypeANSICompatible( EConsoleType ty ) {
		return
			isANSICompatibleEnvironment() && (
				ty != kConsoleTypeNone &&
				ty != kConsoleTypeFile
			);
	}

	static Bool doll__sys_init( SCoreConfig &conf, const SCoreConfig *pPassedConf )
	{
		static SCoreDir coreDirs[] = {
#define SETUPDIR__RW__R false
#define SETUPDIR__RW__W true
#define DOLL_ENGINE__FIXED_DIR(RW_,LongName_,ShortName_) \
	{ ECoreDir::ShortName_, false, SETUPDIR__RW__##RW_, "$" #ShortName_ ":" },
#define DOLL_ENGINE__DIR(RW_,LongName_,ShortName_,DefVal_) \
	{ ECoreDir::ShortName_, true, SETUPDIR__RW__##RW_, "$" #ShortName_ ":" },
#include "doll/Core/EngineDirs.def.hpp"
#undef DOLL_ENGINE__DIR
#undef DOLL_ENGINE__FIXED_DIR
#undef SETUPDIR__RW__W
#undef SETUPDIR__RW__R
		};

		static_assert( arraySize( coreDirs ) == SCoreFileSys::kNumDirs, "Not all directories handled" );

		char szTemp[ 4096 ];

		g_core.version.uOfficialVersion = kVersion;
#ifdef AX_GITINFO__COMMIT
		g_core.version.gitCommitHash = AX_GITINFO__COMMIT;
#endif
#ifdef AX_GITINFO__TSTAMP
		g_core.version.gitCommitTime = AX_GITINFO__TSTAMP;
#endif
		g_core.version.buildVariant = kVariant;

		g_core.tooling.stdoutType = getConsoleType( stdout );
		g_core.tooling.stderrType = getConsoleType( stderr );
		g_core.tooling.stdinType  = getConsoleType( stdin );
		g_core.tooling.useANSIOutput = isConsoleTypeANSICompatible( g_core.tooling.stdoutType );
		g_core.tooling.useANSIErrors = isConsoleTypeANSICompatible( g_core.tooling.stderrType );

#ifdef _WIN32
		// free (potentially used) temporary memory that won't be used again
		getEnv( nullptr );

		if( GetSystemMetrics( SM_CLEANBOOT ) != 0 && !g_core.tooling.isTool ) {
			fprintf( stderr, "Error. Safe mode not supported.\n" );
			fflush( stderr );

			MessageBoxW( GetDesktopWindow(), L"Safe mode not supported.", L"Error", MB_ICONERROR | MB_OK );
			return false;
		}

		if( FAILED( CoInitializeEx( NULL, COINIT_MULTITHREADED ) ) ) {
			fprintf( stderr, "Error. Could not initialize COM.\n" );
			fflush( stderr );

			MessageBoxW( GetDesktopWindow(), L"Could not initialize COM.", L"Error", MB_ICONERROR | MB_OK );
			return false;
		}
#endif

#if DOLL__USE_GLFW
		g_InfoLog += axf( "GLFW version: %s", glfwGetVersionString() );

		glfwSetErrorCallback( &glfw_error_f );
		if( !g_core.tooling.isTool && !glfwInit() ) {
			fprintf( stderr, "Error. Could not initialize GLFW.\n" );
			fflush( stderr );

# ifdef _WIN32
			MessageBoxW( GetDesktopWindow(), L"Could not initialize GLFW.",
				L"Error", MB_ICONERROR | MB_OK );
# endif

			return false;
		}
		glfwSetErrorCallback( &glfw_error_f );
#endif

		// Store the launch directory
		do {
			const Str launchDir( szTemp, axstr_size_t( sysfs_getDir( szTemp ) ) );
			coreDirs[ U32( ECoreDir::Launch ) ] = launchDir;
		} while( false );

		coreDirs[ U32( ECoreDir::App ) ]     = app_getDir();
		coreDirs[ U32( ECoreDir::AppData ) ] = sysfs_getAppDataDir();
		coreDirs[ U32( ECoreDir::AppDocs ) ] = sysfs_getMyDocsDir();
		coreDirs[ U32( ECoreDir::Docs ) ]    = coreDirs[ U32( ECoreDir::AppDocs ) ].path;

		g_core.fs.dirs = coreDirs;

		fs_init();
		async_init();

		char szDefRootDir[ 512 ] = { 0 };
		if( !pPassedConf ) {
			MutStr cfgFilename;
			const Str tryPaths[] = { core_getDir( ECoreDir::Launch ), app_getDir() };
			bool didSucceed = false;

			for( const Str &tryPath : tryPaths ) {
				cfgFilename = tryPath / "script.ini";
				g_DebugLog += cfgFilename;

				if( !fs_fileExists( cfgFilename ) ) {
					continue;
				}

				if( !conf.tryConfig( cfgFilename ) ) {
					g_InfoLog( cfgFilename ) += "Failed to load config.";
					continue;
				}

				axstr_cpy( szDefRootDir, tryPath );
				didSucceed = true;
				break;
			}

			if( didSucceed ) {
				g_InfoLog( cfgFilename ) += "Successfully loaded config";
			} else {
				g_ErrorLog( cfgFilename ) += "Did not load config";
			}

			coreDirs[ U32( ECoreDir::Cfg ) ] = szDefRootDir;
		} else {
			conf = *pPassedConf;
			coreDirs[ U32( ECoreDir::Cfg ) ] = app_getDir();
		}

		AX_EXPECT_MEMORY( g_core.meta.studio.tryAssign( conf.meta.szStudio ) );
		AX_EXPECT_MEMORY( g_core.meta.name.tryAssign( conf.meta.szName ) );

		if( g_core.meta.name.isEmpty() ) {
			AX_EXPECT_MEMORY( g_core.meta.name.tryAssign( app_getName() ) );
		}

		if( g_core.meta.studio.isUsed() ) {
			MutStr studio;

			AX_EXPECT_MEMORY( studio.tryAssign( g_core.meta.studio ) );
			studio.sanitizeFilename();

			coreDirs[ U32( ECoreDir::AppData ) ] /= studio;
			coreDirs[ U32( ECoreDir::AppDocs ) ] /= studio;
		}

		if( g_core.meta.name.isUsed() ) {
			MutStr name;

			AX_EXPECT_MEMORY( name.tryAssign( g_core.meta.name ) );
			name.sanitizeFilename();

			coreDirs[ U32( ECoreDir::AppData ) ] /= name;
			coreDirs[ U32( ECoreDir::AppDocs ) ] /= name;
		} else {
			coreDirs[ U32( ECoreDir::AppData ) ] /= "App";
			coreDirs[ U32( ECoreDir::AppDocs ) ] /= "App";
		}

#define DOLL_ENGINE__DIR(RW_,LongName_,ShortName_,DefVal_) \
	prepareDir( ECoreDir::ShortName_, coreDirs, conf.baseFS.sz##ShortName_##Dir, DefVal_ );
#include "doll/Core/EngineDirs.def.hpp"
#undef DOLL_ENGINE__DIR

		core_installDebugLogReporter();

		g_core.frame.uUpdateId = 0;
		g_core.frame.uRenderId = 0;

		g_core.view.uDefClearColor = conf.script.clearColor;
		if( conf.video.uFrameLimit != ~0U ) {
			if( conf.video.uFrameLimit > 0 ) {
				g_core.frame.uLimitMillisecs = 1000/conf.video.uFrameLimit;
			} else {
				g_core.frame.uLimitMillisecs = 0;
			}
		}

		return true;
	}
	static Void doll__sys_fini()
	{
		async_fini();
#if DOLL__USE_GLFW
		if( !g_core.tooling.isTool ) {
			glfwTerminate();
		}
#endif

#if AX_OS_WINDOWS
		CoUninitialize();
#endif
	}

	static Bool doll__wnd_init( SCoreConfig &conf )
	{
		const SIntVector2 screenRes =
#if DOLL__USE_GLFW
			glfw_getDesktopSize()
#else
			os_getDesktopSize()
#endif
			;

		if( !conf.video.uResX ) {
			conf.video.uResX = screenRes.x/2 + screenRes.x/4;
		}
		if( !conf.video.uResY ) {
			conf.video.uResY = U32( F64( conf.video.uResX )*0.5625 );
		}

		if( !conf.script.szTitle[ 0 ] ) {
			axstr_cpy( conf.script.szTitle, doll_getEngineString() );
		}

#if DOLL__USE_GLFW
# ifndef __APPLE__
		glfwWindowHint( GLFW_VISIBLE, GLFW_FALSE );
# endif
		glfwWindowHint( GLFW_RESIZABLE, GLFW_TRUE );

# ifdef __APPLE__
#  if 0 // Oops. Thought we *needed* GL 3.2 on macOS
		glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
		glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 2 );
		glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
		glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
#  else
		glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 2 );
		glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
#  endif
# else
		glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 2 );
		glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
# endif

		GLFWwindow *const wnd =
			glfwCreateWindow
			(
				int( unsigned( conf.video.uResX ) ),
				int( unsigned( conf.video.uResY ) ),
				conf.script.szTitle,
				nullptr,
				nullptr
			);
		if( !AX_VERIFY_MSG( wnd != nullptr, "Failed to create window" ) ) {
			return false;
		}

		glfwSetWindowCloseCallback( wnd, &glfw_onClosed_f );
		glfwSetFramebufferSizeCallback( wnd, &glfw_onSized_f );

		glfwSetWindowFocusCallback( wnd, &glfw_windowFocus_f );

		glfwSetMouseButtonCallback( wnd, &glfw_mouseButton_f );
		glfwSetCursorPosCallback( wnd, &glfw_mousePos_f );
		glfwSetCursorEnterCallback( wnd, &glfw_mouseEnterLeave_f );
		glfwSetScrollCallback( wnd, &glfw_mouseScroll_f );

		glfwSetKeyCallback( wnd, &glfw_keyButton_f );
		glfwSetCharCallback( wnd, &glfw_keyChar_f );

		glfwMakeContextCurrent( wnd );

		g_core.view.window = wnd;
#else
		SWndDelegate   wndDel;
		SWndCreateInfo wndInfo;

		wndDel.pfnOnClosed = &onClosed_f;
		wndDel.pfnOnSized  = &onSized_f;

		wndDel.pfnOnAcceptKey    = &in_onAcceptKey_f;
		wndDel.pfnOnResignKey    = &in_onResignKey_f;
		wndDel.pfnOnKeyPress     = &in_onKeyPress_f;
		wndDel.pfnOnKeyRelease   = &in_onKeyRelease_f;
		wndDel.pfnOnKeyChar      = &in_onKeyChar_f;
		wndDel.pfnOnMousePress   = &in_onMousePress_f;
		wndDel.pfnOnMouseRelease = &in_onMouseRelease_f;
		wndDel.pfnOnMouseWheel   = &in_onMouseWheel_f;
		wndDel.pfnOnMouseMove    = &in_onMouseMove_f;
		wndDel.pfnOnMouseExit    = &in_onMouseExit_f;

		wndInfo.pDelegate   = &wndDel;
		wndInfo.uStyleFlags = 0;

		wndInfo.title = conf.script.szTitle;

		wndInfo.shape.resizeMe( SIntVector2( ( S32 )conf.video.uResX, ( S32 )conf.video.uResY ) );
		wndInfo.shape.centerMe( SRect( SIntVector2(), screenRes ) );

		if( !AX_VERIFY_MSG( g_core.view.window = wnd_open( wndInfo ), "Failed to create window" ) ) {
			return false;
		}
#endif

		g_core.input.uFlags = kCoreInF_Enabled;

		in_enableEscapeKey();
		in_enableDevcon();
		in_enableDevDebug();
		in_enableToggleFullscreen();
		in_enableScreenshot();

		return true;
	}
	static Void doll__wnd_fini()
	{
#if DOLL__USE_GLFW
		g_core.view.window = ( glfwDestroyWindow( g_core.view.window ), nullptr );
#else
		g_core.view.window = ( wnd_close( g_core.view.window ), ( OSWindow )0 );
#endif
	}

	static Bool doll__gfx_init( SCoreConfig &conf )
	{
		SGfxInitDesc desc;

		desc.apis = TArr<EGfxAPI>(); // FIXME: Parse `conf.video.szAPIs`
		desc.windowing = conf.video.getScreenMode();
		desc.vsync = conf.video.iVsync;

		const OSWindow gfxwnd =
#if DOLL__USE_GLFW && 0 // FIXME: Why was this here?
			OSWindow(0)
#else
			(OSWindow)g_core.view.window
#endif
			;

		axpf("gfx: Trying initAPI\n");
		if( !AX_VERIFY_MSG( g_core.view.pGfxAPI = gfx_initAPI( gfxwnd, &desc ), "Failed to initialize graphics" ) ) {
			return false;
		}

		axpf("gfx: Trying newCGfxFrame\n");
		g_core.view.pGfxFrame = new CGfxFrame( *g_core.view.pGfxAPI );
		if( !AX_VERIFY_MEMORY( g_core.view.pGfxFrame ) ) {
			g_core.view.pGfxAPI = gfx_finiAPI( g_core.view.pGfxAPI );
			return false;
		}

		axpf("gfx: Trying CGfxFrame::getMemVBuf\n");
		if( !AX_VERIFY_MEMORY( g_core.view.pGfxFrame->getMemVBuf( 2*1024*1024 ) ) ) {
			delete g_core.view.pGfxFrame;
			g_core.view.pGfxFrame = nullptr;
			g_core.view.pGfxAPI = gfx_finiAPI( g_core.view.pGfxAPI );
			return false;
		}

		axpf("gfx: layerMgr->setDefaultFrame\n");
		g_layerMgr->setDefaultFrame( g_core.view.pGfxFrame );
		gfx_r_setFrame( g_core.view.pGfxFrame );

		axpf("gfx: Set aspect\n");
		gfx_setCurrentLayer( gfx_getDefaultLayer() );
		gfx_getDefaultLayer()->setGLFrame( gfx_r_getFrame() );

		gfx_setLayerAspect( gfx_getDefaultLayer(), F32( 1280.0/720.0 ), EAspect::Fit );

		gfx_setLayerSize( gfx_getDefaultLayer(), g_core.view.pGfxFrame->getResX(), g_core.view.pGfxFrame->getResY() );

		axpf("gfx: spritemgr::init_gl\n");
		if( !AX_VERIFY_MSG( g_spriteMgr.init_gl(), "Failed to initialize sprite system" ) ) {
			delete g_core.view.pGfxFrame;
			g_core.view.pGfxFrame = nullptr;

			g_core.view.pGfxAPI = gfx_finiAPI( g_core.view.pGfxAPI );
			return false;
		}

#if DOLL__USE_GLFW
		axpf("gfx: glfw: Resize properly\n");
		do {
			int w = 0, h = 0;
			glfwGetWindowSize( g_core.view.window, &w, &h );
			axpf( "gfx: glfw: got %i x %i\n", w, h);
			glfw_onSized_f( g_core.view.window, w, h );
		} while( false );
#endif

		axpf("gfx: done\n");
		return true;
	}
	static Void doll__gfx_fini()
	{
		g_spriteMgr.fini_gl();

		g_core.view.pGfxFrame = ( ( delete g_core.view.pGfxFrame ), nullptr );
		g_core.view.pGfxAPI   = gfx_finiAPI( g_core.view.pGfxAPI );
	}

	static Bool doll__snd_init( SCoreConfig &conf )
	{
		( Void )conf;

		g_core.sound.uFlags = 0;

		// FIXME: Pass initialization options to the sound system (e.g.,
		//        numchannels, sample rate)
		if( !snd_init() ) {
			return false;
		}

		g_core.sound.uFlags = kCoreSndF_Enabled | kCoreSndF_AllowBGM | kCoreSndF_AllowSFX | kCoreSndF_AllowKoe;
		return true;
	}
	static Void doll__snd_fini()
	{
		snd_fini();
	}

// Not entirely clear why this is necessary on not-Windows...
//
// Removing this snippet of preprocessor hackery will result in `Bool` suddenly
// being interpreted as an `int` here.
#ifndef _WIN32
# undef  Bool
# define Bool bool
#endif

	DOLL_FUNC Bool DOLL_API doll_init( const SCoreConfig *pConf )
	{
		AX_ASSERT( g_core.notInitialized() );

		g_DebugLog += doll_getEngineString();

		SCoreConfig conf;
		axpf("Trying sysinit\n");
		if( !doll__sys_init( conf, pConf ) ) {
			return false;
		}

		axpf("Trying wndinit\n");
		if( !doll__wnd_init( conf ) ) {
			doll__sys_fini();
			return false;
		}

		axpf("Trying gfxinit\n");
		if( !doll__gfx_init( conf ) ) {
			doll__wnd_fini();
			doll__sys_fini();
			return false;
		}

		axpf("Trying sndinit\n");
		if( !doll__snd_init( conf ) ) {
			g_WarningLog += "Not using sound.";
		}

		axpf("Making window visible\n");
#if DOLL__USE_GLFW
		glfwShowWindow( g_core.view.window );
		glfwFocusWindow( g_core.view.window );
#else
		wnd_setVisible( g_core.view.window, true );
#endif

		g_core.frame.timing.updateMe( microseconds() );
		return true;
	}
	DOLL_FUNC Bool DOLL_API doll_initConsoleApp()
	{
		AX_ASSERT( g_core.notInitialized() );

		g_DebugLog += doll_getEngineString();

		g_core.tooling.isTool = true;

		SCoreConfig passedConf;
		SCoreConfig conf;
		if( !doll__sys_init( conf, &passedConf ) ) {
			return false;
		}

		g_core.frame.timing.updateMe( microseconds() );
		return true;
	}
	DOLL_FUNC Void DOLL_API doll_fini()
	{
		if( !g_core.tooling.isTool ) {
			doll__snd_fini();
			doll__gfx_fini();
			doll__wnd_fini();
		}
		doll__sys_fini();
	}

	DOLL_FUNC Bool DOLL_API doll_sync_app()
	{
		in_resetMouseMove();

#if DOLL__USE_GLFW
		glfwPollEvents();
		return !glfwWindowShouldClose( g_core.view.window );
#else
		os_processAllQueuedEvents();
		return !os_receivedQuitEvent();
#endif
	}
	DOLL_FUNC Void DOLL_API doll_sync_update()
	{
		// Step forward the asynchronous IO system
		async_step();

		if( !g_core.tooling.isTool ) {
			// Update miscellaneous management systems
			CFrameCounter::updateAll();
			ActiveAction::runAll();
			g_spriteMgr.update();

			// Update the sound system
			snd_sync();
		}

		// Increment the general frame counter
		++g_core.frame.uUpdateId;

		// Update the memory tags
#if DOLL_TAGS_ENABLED
		Mem::updateTags();
#endif
	}
	DOLL_FUNC Void DOLL_API doll_sync_render()
	{
		// Perform an automatic clear if the color is setup
		if( g_core.view.uDefClearColor != 0 ) {
			gfx_r_clearRect( 0, 0, gfx_r_resX(), gfx_r_resY(), g_core.view.uDefClearColor );
#if 0
			const F32 r = F32( DOLL_COLOR_R( g_core.uDefClearColor ) )/255.0f;
			const F32 g = F32( DOLL_COLOR_G( g_core.uDefClearColor ) )/255.0f;
			const F32 b = F32( DOLL_COLOR_B( g_core.uDefClearColor ) )/255.0f;

			glClearColor( r, g, b, 1.0f );
			glClear( GL_COLOR_BUFFER_BIT );
#endif
		}

		// Add the widget system to the user's commands
		g_widgets->frame();

		// Update the user's commands
		g_layerMgr->renderGL( g_core.view.pGfxFrame );
		g_core.view.pGfxAPI->wsiPresent();

		// Increment the rendering frame counter
		++g_core.frame.uRenderId;
	}
	DOLL_FUNC Void DOLL_API doll_sync_timing()
	{
		// Update timing
		g_core.frame.timing.updateMe( microseconds() );

		// Apply frame limiter if necessary
#ifdef _WIN32
		const U32 cMillisecs = g_core.frame.timing.elapsedMilliseconds();
		if( cMillisecs < g_core.frame.uLimitMillisecs ) {
			Sleep( g_core.frame.uLimitMillisecs - cMillisecs );
		}
#else
		const U64 cNanosecs = g_core.frame.timing.elapsedNanoseconds();
		const U64 cMaxNanosecs = millisecondsToNanoseconds( g_core.frame.uLimitMillisecs );
		if( cNanosecs < cMaxNanosecs ) {
			const U32 cDeltaNanosecs = U32( cMaxNanosecs - cNanosecs );
			const struct timespec ts = {
				cDeltaNanosecs/AXTIME_NANOSECS,
				cDeltaNanosecs%AXTIME_NANOSECS
			};
			nanosleep( &ts, nullptr );
		}
#endif
	}

	DOLL_FUNC Bool DOLL_API doll_sync()
	{
		if( g_core.tooling.isTool ) {
			doll_sync_update();
			doll_sync_timing();
			return true;
		}

		// Update general stuff and the renderer
		doll_sync_update();
		doll_sync_render();

		// General OS interaction update
		if( !doll_sync_app() ) {
			return false;
		}
		doll_sync_timing();

		// Done
		return true;
	}

#if DOLL__USE_GLFW
	DOLL_FUNC OSWindow DOLL_API doll__getOSWindow()
	{
		GLFWwindow *const wnd = g_core.view.window;

		if( !wnd ) {
			return OSWindow( 0 );
		}

# if defined( GLFW_EXPOSE_NATIVE_WIN32 )
		return OSWindow( glfwGetWin32Window( wnd ) );
# elif defined( GLFW_EXPOSE_NATIVE_X11 )
		return OSWindow( glfwGetX11Window( wnd ) );
# elif defined( GLFW_EXPOSE_NATIVE_COCOA )
		return OSWindow( ( NSWindow * )glfwGetCocoaWindow( wnd ) );
# else
#  error "Unhandled GLFW native platform."
# endif
	}
#endif

}
