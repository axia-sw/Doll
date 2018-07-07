#define DOLL_TRACE_FACILITY doll::kLog_FrontendSetup
#include "../BuildSettings.hpp"

#include "doll/Front/Setup.hpp"
#include "doll/Core/Config.hpp"
#include "doll/Core/Logger.hpp"
#include "doll/Gfx/API.hpp"
#include "doll/Snd/ChannelUtil.hpp"
#include "doll/IO/SysFS.hpp"

namespace doll
{
	
	static const EGfxScreenMode kDefFullscreen = kGfxScreenModeFullDesktop;
	static const EGfxScreenMode kDefScreenMode = kGfxScreenModeWindowed;

	static const U32 kDefChannels = kChannelsStereo;

	inline Str screenModeToString( EGfxScreenMode mode )
	{
		switch( mode ) {
		case kGfxScreenModeWindowed:
			return "windowed";

		case kGfxScreenModeFullDesktop:
			return "fullscreen-desktop";

		case kGfxScreenModeFullExclusive:
			return "fullscreen-exclusive";
		}

		return "(unknown)";
	}

	EGfxScreenMode SUserConfig::SVideo::getScreenMode() const
	{
		const Str s( szScreenMode );

		if( s.caseStartsWith( "full" ) ) {
			const Str x1 = s.caseStartsWith( "fullscreen" ) ? s.skip( 10 ) : s.skip( 4 );
			const Str x2 = s.startsWithAny( " +-" ) ? x1.skip( 1 ) : x1;
			const Str x = x2;

			if( x.caseCmp( "exclusive" ) ) {
				return kGfxScreenModeFullExclusive;
			}

			if( x.caseCmp( "desktop" ) || x.caseCmp( "shared" ) ) {
				return kGfxScreenModeFullDesktop;
			}

			// FIXME: Warn about unknown fullscreen mode
			return kDefFullscreen;
		}

		if( s.caseCmp( "window" ) || s.caseCmp( "windowed" ) || s.caseCmp( "wnd" ) ) {
			return kGfxScreenModeWindowed;
		}

		// FIXME: Warn about unknown screen mode
		return kDefScreenMode;
	}
	// Parses: ([1-9](\.[0-9])?) | { L R C S Lb Rb Lf Rf Cb Ls Rs }
	U32 SUserConfig::SAudio::getChannelMask() const
	{
		const U32 uMask = snd_stringToChannelMask( Str(  szChannels ) );
		return uMask ? uMask : kDefChannels;
	}

	static Bool readConfigU32( SConfigVar &sect, const Str &key, U32 &out_x )
	{
		SConfigVar *const p = core_findConfigVar( &sect, key );
		if( !p ) {
			return false;
		}

		out_x = ( U32 )core_getConfigVarValue( p ).toUnsignedInteger();

		core_removeConfigVar( p );
		return true;
	}
	static Bool readConfigS32( SConfigVar &sect, const Str &key, S32 &out_x )
	{
		SConfigVar *const p = core_findConfigVar( &sect, key );
		if( !p ) {
			return false;
		}

		out_x = ( S32 )core_getConfigVarValue( p ).toInteger();

		core_removeConfigVar( p );
		return true;
	}
	static Bool readConfigBool( SConfigVar &sect, const Str &key, Bool &out_x )
	{
		SConfigVar *const p = core_findConfigVar( &sect, key );
		if( !p ) {
			return false;
		}

		out_x = core_getConfigVarValue( p ).toBool();

		core_removeConfigVar( p );
		return true;
	}

	static Bool readConfigText( SConfigVar &sect, const Str &key, char *out_buf, UPtr bufn )
	{
		SConfigVar *const p = core_findConfigVar( &sect, key );
		if( !p ) {
			return false;
		}

		const Str v = core_getConfigVarValue( p );
		axstr_cpyn( out_buf, bufn, v.get(), v.len() );

		core_removeConfigVar( p );
		return true;
	}
	template< UPtr tN >
	static Bool readConfigText( SConfigVar &sect, const Str &key, char( &out_buf )[ tN ] )
	{
		return readConfigText( sect, key, out_buf, tN );
	}

	static Bool readConfigPath( const Str &dir, SConfigVar &sect, const Str &key, char *out_buf, UPtr bufn )
	{
		// TODO: `dir` represents the directory the file is relative to if it's
		//       not an absolute path
		((void)dir);

		return readConfigText( sect, key, out_buf, bufn );
	}
	template< UPtr tN >
	static Bool readConfigPath( const Str &dir, SConfigVar &sect, const Str &key, char( &out_buf )[ tN ] )
	{
		return readConfigPath( dir, sect, key, out_buf, tN );
	}

	static Bool readConfigColor( SConfigVar &sect, const Str &key, U32 &out_color )
	{
		SConfigVar *const p = core_findConfigVar( &sect, key );
		if( !p ) {
			return false;
		}

		Str v = core_getConfigVarValue( p ).trim();

		// FIXME: Add `rgb()` (and `argb()`�E`rgba()`) functions and float variants
		// FIXME: Add standard color names (e.g., "black," "cyan," etc)

		if( v.startsWith( '#' ) ) {
			v = v.skip();

			if( v.len() == 3 ) {
				const int r = axstr_digit( v[ 0 ], 16 );
				const int g = axstr_digit( v[ 1 ], 16 );
				const int b = axstr_digit( v[ 2 ], 16 );

				if( ( r|g|b ) >= 0 ) {
					out_color = DOLL_RGB( r<<4 | r, g<<4 | g, b<<4 | b );
				} else {
					out_color = 0xFF000000;
				}

				core_removeConfigVar( p );
				return true;
			}
		}

		const U32 c = ( U32 )v.toUnsignedInteger( 16 );

		out_color = DOLL_RGB( DOLL_COLOR_B( c ), DOLL_COLOR_G( c ), DOLL_COLOR_R( c ) );

		core_removeConfigVar( p );
		return true;
	}

	static Void warnConfigVars( Str filename, SConfigVar &sect )
	{
		const char *const sp = sect.name.get();
		const UPtr sn = sect.name.len();

		SConfigVar *p = sect.children.head();
		while( p != nullptr ) {
			char szBuf[ 512 ];

			const char *const np = p->name.get();
			const UPtr nn = p->name.len();

			axspf( szBuf, "Unknown variable: '#%.*s.%.*s'", sn, sp, nn, np );
			g_WarningLog( filename ) += szBuf;

			p = p->sibling.next();
		}
	}

	Bool SUserConfig::tryConfig( CConfiguration &conf, Str filename )
	{
		SConfigVar *pSect = nullptr;
		Bool r = false;

		// [Video]
		if( ( pSect = core_findConfigSection( &conf, "Video" ) ) != nullptr ) {
			r |= readConfigU32( *pSect, "ResX", video.uResX );
			r |= readConfigU32( *pSect, "ResY", video.uResY );
			r |= readConfigS32( *pSect, "Vsync", video.iVsync );
			r |= readConfigU32( *pSect, "FrameLimit", video.uFrameLimit );
			r |= readConfigText( *pSect, "ScreenMode", video.szScreenMode );
			r |= readConfigText( *pSect, "API", video.szAPIs );

			Bool bFullscreen = false;
			if( readConfigBool( *pSect, "Fullscreen", bFullscreen ) ) {
				const EGfxScreenMode m = bFullscreen ? kDefFullscreen : kGfxScreenModeWindowed;
				const Str s = screenModeToString( m );

				axstr_cpyn( video.szScreenMode, s.get(), s.len() );
				r |= true;
			}

			warnConfigVars( filename, *pSect );
		}

		// [Audio]
		if( ( pSect = core_findConfigSection( &conf, "Audio" ) ) != nullptr ) {
			r |= readConfigText( *pSect, "Devices", audio.szDevices );
			r |= readConfigText( *pSect, "Channels", audio.szChannels );
			r |= readConfigU32( *pSect, "SampleRate", audio.cSamplesHz ); // FIXME: Make+Use `readConfigHz()`

			warnConfigVars( filename, *pSect );
		}

		return r;
	}
	Bool SUserConfig::tryConfig( Str filename )
	{
		CConfiguration *const p = core_loadConfig( filename );
		if( !p ) {
			return false;
		}

		const Bool r = tryConfig( *p, filename );

		core_deleteConfig( p );
		return r;
	}

	Bool SCoreConfig::tryConfig( CConfiguration &conf, Str filename )
	{
		SConfigVar *pSect = nullptr;
		Bool r = false;

		// [Meta]
		if( ( pSect = core_findConfigSection( &conf, "Meta" ) ) != nullptr ) {
			r |= readConfigText( *pSect, "Studio", meta.szStudio );
			r |= readConfigText( *pSect, "Name", meta.szName );

			warnConfigVars( filename, *pSect );
		}

		// [BaseFS]
		if( ( pSect = core_findConfigSection( &conf, "BaseFS" ) ) != nullptr ) {
			const Str dir = filename.getDirectory();

#define DOLL_ENGINE__DIR(RW_,LongName_,ShortName_,DefVal_) \
	r |= readConfigPath( dir, *pSect, #ShortName_ "Dir", baseFS.sz##ShortName_##Dir );
#include "doll/Core/EngineDirs.def.hpp"
#undef DOLL_ENGINE__DIR

			warnConfigVars( filename, *pSect );
		}

		// [Script]
		if( ( pSect = core_findConfigSection( &conf, "Script" ) ) != nullptr ) {
			r |= readConfigText( *pSect, "Title", script.szTitle );
			r |= readConfigColor( *pSect, "ClearColor", script.clearColor );

			warnConfigVars( filename, *pSect );
		}

		// (user config)
		r |= SUserConfig::tryConfig( conf, filename );

		return r;
	}
	Bool SCoreConfig::tryConfig( Str filename )
	{
		CConfiguration *const p = core_loadConfig( filename );
		if( !p ) {
			return false;
		}

		const Bool r = tryConfig( *p, filename );

		core_deleteConfig( p );
		return r;
	}

	DOLL_FUNC Bool DOLL_API app_loadUserConfig( SUserConfig &dstConf, Str filename )
	{
		return dstConf.tryConfig( filename );
	}
	DOLL_FUNC Bool DOLL_API app_loadConfig( SCoreConfig &dstConf, Str filename )
	{
		return dstConf.tryConfig( filename );
	}

#define DOLL__MAXARGS 8192

	DOLL_FUNC Bool DOLL_API app_convertArgs( TArr<Str> &dst, unsigned argc, const char *const *argv )
	{
		static Str args[ DOLL__MAXARGS ];

		if( argc > ( unsigned )arraySize( args ) ) {
			argc = ( unsigned )arraySize( args );
		}

		for( unsigned i = 0; i < argc; ++i ) {
			args[ i ] = Str( argv[ i ] );
		}

		dst = TArr<Str>( args, argc );
		return true;
	}
	DOLL_FUNC Bool DOLL_API app_convertArgsW( TArr<Str> &dst, unsigned argc, const wchar_t *const *wargv )
	{
		static TMutArr< MutStr > argsBase;
		static Str args[ DOLL__MAXARGS ];

		if( argc > ( unsigned )arraySize( args ) ) {
			argc = ( unsigned )arraySize( args );
		}

		if( !AX_VERIFY_MEMORY( argsBase.reserve( argc ) ) ) {
			return false;
		}

		for( unsigned i = 0; i < argc; ++i ) {
			if( !AX_VERIFY_MEMORY( argsBase.append( MutStr::fromWStr( wargv[ i ] ) ) ) ) {
				argsBase.clear();
				return false;
			}

			args[ i ] = argsBase[ i ];
		}

		dst = TArr<Str>( args, argc );
		return true;
	}

}
