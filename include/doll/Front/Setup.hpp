#pragma once

#include "../Core/Defs.hpp"
#include "../Gfx/API.hpp"

namespace doll
{

	class CConfiguration;

	struct SUserConfig
	{
		struct SVideo
		{
			U32  uResX               = 0;
			U32  uResY               = 0;
			char szScreenMode[ 128 ] = { '\0' };
			S32  iVsync              = 1;
			U32  uFrameLimit         = ~0U;
			char szAPIs      [ 128 ] = { '\0' };

			// FIXME: Add adapters

#ifdef DOLL__BUILD
			EGfxScreenMode getScreenMode() const;
#endif
		} video;

		struct SAudio
		{
			char szDevices [ 256 ] = { '\0' };
			char szChannels[  64 ] = { '\0' }; // ([1-9](\.[0-9])?) | { L R C S Lb Rb Lf Rf Cb Ls Rs }
			U32  cSamplesHz        = 0;

#ifdef DOLL__BUILD
			U32 getChannelMask() const;
#endif
		} audio;

#ifdef DOLL__BUILD
		Bool tryConfig( CConfiguration &, Str filename );
		Bool tryConfig( Str filename );
#endif

		inline SUserConfig &setResolution( U32 uResX, U32 uResY )
		{
			video.uResX = uResX;
			video.uResY = uResY;
			return *this;
		}
		inline SUserConfig &setFullscreen( Bool bFullscreen = true )
		{
			if( bFullscreen ) {
				axstr_cpy( video.szScreenMode, "fullscreen" );
			} else {
				axstr_cpy( video.szScreenMode, "windowed" );
			}

			return *this;
		}
		inline SUserConfig &setVsync( S32 iVsync )
		{
			video.iVsync = iVsync;
			return *this;
		}
		inline SUserConfig &setFrameLimit( U32 uFrameLimitFPS )
		{
			video.uFrameLimit = uFrameLimitFPS;
			return *this;
		}

		// Set the render API
		//
		// Should be one of:
		// - "" (use default set for this platform)
		// - "dx", "d3d", or "d3d11"
		// - "gl", "ogl", or "opengl"
		// - "vk", or "vulkan"
		// - "d3d12"
		inline SUserConfig &setRenderAPI( const Str &api )
		{
			axstr_cpy( video.szAPIs, api );
			return *this;
		}
		// Add another render API to try if the prior fails
		//
		// Should be one of:
		// - "dx", "d3d", or "d3d11"
		// - "gl", "ogl", or "opengl"
		// - "vk", or "vulkan"
		// - "d3d12"
		inline SUserConfig &addRenderAPI( const Str &api )
		{
			if( video.szAPIs[ 0 ] != '\0' ) {
				axstr_cat( video.szAPIs, " " );
			}

			axstr_cat( video.szAPIs, api );
			return *this;
		}

		// Set the render API
		inline SUserConfig &setRenderAPI( EGfxAPI api )
		{
			video.szAPIs[ 0 ] = '\0';
			return addRenderAPI( api );
		}
		// Add another render API if the prior fails
		inline SUserConfig &addRenderAPI( EGfxAPI api )
		{
			switch( api ) {
#define DOLL_GFX__API(Name_,BriefName_) \
	case kGfxAPI##Name_: return addRenderAPI( #BriefName_ );

#include "../Gfx/APIs.def.hpp"

#undef DOLL_GFX__API

			case kNumGfxAPIs:
				AX_UNREACHABLE();
			}

			return *this;
		}

		// FIXME: Add helper functions for sound setup
	};

	struct SCoreConfig: public SUserConfig
	{
		struct SMeta
		{
			char szStudio[ 128 ];
			char szName  [ 128 ];

			inline SMeta()
			{
				szStudio[ 0 ] = '\0';
				szName  [ 0 ] = '\0';
			}
		} meta;
		struct SBaseFS
		{
			static const UPtr kMaxPath = 256;

#define DOLL_ENGINE__DIR(RW_,LongName_,ShortName_,DefVal_) char sz##ShortName_##Dir[ kMaxPath ];
#include "../Core/EngineDirs.def.hpp"
#undef DOLL_ENGINE__DIR

			inline SBaseFS()
			{
#define DOLL_ENGINE__DIR(RW_,LongName_,ShortName_,DefVal_) sz##ShortName_##Dir[ 0 ] = '\0';
#include "../Core/EngineDirs.def.hpp"
#undef DOLL_ENGINE__DIR
			}
		} baseFS;
		struct SScript
		{
			char szTitle[ 128 ];
			U32  clearColor = 0x805020;

			inline SScript()
			{
				szTitle[ 0 ] = '\0';
			}
		} script;

		inline SCoreConfig()
		: SUserConfig()
		, baseFS()
		, script()
		{
		}

#ifdef DOLL__BUILD
		Bool tryConfig( CConfiguration &, Str filename );
		Bool tryConfig( Str filename );
#endif

		inline SCoreConfig &setStudio( const Str &studio )
		{
			axstr_cpy( meta.szStudio, studio );
			return *this;
		}
		inline SCoreConfig &setName( const Str &name )
		{
			axstr_cpy( meta.szName, name );
			return *this;
		}

#define DOLL_ENGINE__DIR(RW_,LongName_,ShortName_,DefVal_) \
	inline SCoreConfig &set##ShortName_##Dir( const Str &dirname )\
	{\
		axstr_cpy( baseFS.sz##ShortName_##Dir, dirname );\
		return *this;\
	}
#include "../Core/EngineDirs.def.hpp"
#undef DOLL_ENGINE__DIR

		inline SCoreConfig &setTitle( const Str &title )
		{
			axstr_cpy( script.szTitle, title );
			return *this;
		}
		inline SCoreConfig &setClearColor( U32 uColorRGBA )
		{
			script.clearColor = uColorRGBA;
			return *this;
		}
		inline SCoreConfig &setClearColor( F32 r, F32 g, F32 b, F32 a = 1.0f )
		{
			script.clearColor = doll_rgb( r, g, b, a );
			return *this;
		}
	};

	DOLL_FUNC Bool DOLL_API app_loadUserConfig( SUserConfig &dstConf, Str filename );
	inline SUserConfig DOLL_API app_loadUserConfig( Str filename )
	{
		SUserConfig r;
		return app_loadUserConfig( r, filename ), r;
	}
	DOLL_FUNC Bool DOLL_API app_loadConfig( SCoreConfig &dstConf, Str filename );
	inline SCoreConfig DOLL_API app_loadConfig( Str filename )
	{
		SCoreConfig r;
		return app_loadConfig( r, filename ), r;
	}

	DOLL_FUNC Bool DOLL_API app_convertArgs( TArr<Str> &dst, unsigned argc, const char *const *argv );
	DOLL_FUNC Bool DOLL_API app_convertArgsW( TArr<Str> &dst, unsigned argc, const wchar_t *const *wargv );
	inline Bool DOLL_API app_convertArgs( TArr<Str> &dst, unsigned argc, const wchar_t *const *wargv )
	{
		return app_convertArgsW( dst, argc, wargv );
	}
	inline TArr<Str> DOLL_API app_convertArgs( unsigned argc, const char *const *argv )
	{
		TArr<Str> r;
		return app_convertArgs( r, argc, argv ), r;
	}
	inline TArr<Str> DOLL_API app_convertArgs( unsigned argc, const wchar_t *const *wargv )
	{
		TArr<Str> r;
		return app_convertArgsW( r, argc, wargv ), r;
	}

}
