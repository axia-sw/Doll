#pragma once

#include "Defs.hpp"

#include "MemoryTags.hpp"
#include "Memory.hpp"

#include "../OS/Key.hpp"
#include "../OS/Window.hpp"
#include "../Gfx/API.hpp"

struct GLFWwindow;
struct GLFWmonitor;

namespace doll
{

	struct SCoreConfig;  // in "doll-frontend-setup.hpp"
	class IFileProvider; // in "doll-core-file.hpp"
	class CAsyncOp;      // in "doll-core-async_io.hpp"
	class CSoundMixer;   // in "doll-snd-core.hpp"

	enum:UPtr { kMaxAsyncOps     = 64 }; // FIXME: tweak this arbitrarily selected value
	enum:UPtr { kMaxInputActions = 16 };

	enum:U32  { kNumAsyncRetries = 3  };

	enum class ECoreDir: U32
	{
#define DOLL_ENGINE__FIXED_DIR(RW_,LongName_,ShortName_) ShortName_,
#include "EngineDirs.def.hpp"
#undef DOLL_ENGINE__FIXED_DIR
	};

	struct SCoreDir
	{
		enum EFlag: U32
		{
			// can be changed by the config system
			kConfigurable = 0x00000001,
			// can be created and written to
			kWritable     = 0x00000002
		};
		const ECoreDir      kind;   // which type of directory
		const U32           flags;  // settings that apply to this directory
		const Str           prefix; // e.g., "$Dat:"
		MutStr              path;   // path to the directory

		SCoreDir( ECoreDir kind, Bool isConfigurable, Bool isWritable, const Str &prefix )
		: kind( kind )
		, flags( ( isConfigurable ? kConfigurable : 0 ) | ( isWritable ? kWritable : 0 ) )
		, prefix( prefix )
		, path()
		{
		}

		Bool is( ECoreDir x ) const
		{
			return kind == x;
		}

		Bool isConfigurable() const
		{
			return ( flags & kConfigurable ) != 0;
		}
		Bool isWritable() const
		{
			return ( flags & kWritable ) != 0;
		}

		SCoreDir &operator=( const Str &x )
		{
			AX_EXPECT_MEMORY( path.tryAssign( x ) );
			return *this;
		}
		SCoreDir &operator/=( const Str &x )
		{
			AX_EXPECT_MEMORY( path.tryAppendPath( x ) );
			return *this;
		}

		SCoreDir( const SCoreDir & ) = delete;
		SCoreDir &operator=( const SCoreDir & ) = delete;
	};

	// Specifies the console type for a given input or output stream
	enum EConsoleType: U32
	{
		// There is no console; I/O will likely fail
		kConsoleTypeNone,
		// I/O is dealing with a file or a pipe
		kConsoleTypeFile,
		// A standard ANSI TTY
		kConsoleTypeANSI,
#ifdef _WIN32
		// Windows console created by this process (this process was likely
		// launched stand-alone)
		kConsoleTypeWinconOwned,
		// Windows console owned by a calling process (this process was likely
		// launched from a command-prompt)
		kConsoleTypeWinconUsing,
#endif

		// The number of console types supported
		kNumConsoleTypes
	};

	enum EFrameLimit: U32
	{
		kFrameLimit30FPS  = 33,
		kFrameLimit60FPS  = 16,
		kFrameLimit120FPS = 8,
		kFrameLimitNone   = 0
	};

	enum ECoreInputFlag: U32
	{
		kCoreInF_Enabled = 0x00000001
	};
	enum class ECoreInputAction: U32
	{
		// [Esc] Quit the application
		Quit,

		// [Alt+Enter;F11] Toggle fullscreen
		ToggleFullscreen,
		// [F12] Create a screenshot
		CaptureScreenshot,

		// [`] Opens the developer console
		OpenDevcon,
		// [F3] Toggle the developer/profiling graphs (includes active async operations)
		ToggleGraphs
	};

	enum ECoreSoundFlag: U32
	{
		kCoreSndF_Enabled  = 0x00000001,
		kCoreSndF_AllowBGM = 0x00000002,
		kCoreSndF_AllowSFX = 0x00000004,
		kCoreSndF_AllowKoe = 0x00000008
	};

	struct SCoreInputAction
	{
		EKey             key;
		U32              uMods;

		ECoreInputAction command;
	};

	struct SFrameTiming
	{
		U64 uPrevMicrosecs;
		U64 uCurrMicrosecs;

		inline SFrameTiming()
		: uPrevMicrosecs( 0 )
		, uCurrMicrosecs( 0 )
		{
		}
		inline ~SFrameTiming()
		{
		}

		inline SFrameTiming &updateMe( U64 uNewMicrosecs )
		{
			uPrevMicrosecs = uCurrMicrosecs;
			uCurrMicrosecs = uNewMicrosecs;

			return *this;
		}

		inline U64 elapsedMicroseconds() const
		{
			return uCurrMicrosecs - uPrevMicrosecs;
		}
		inline U32 elapsedMilliseconds() const
		{
			return microsecondsToMilliseconds( elapsedMicroseconds() );
		}
		inline F64 elapsedSeconds() const
		{
			return microsecondsToSeconds( elapsedMicroseconds() );
		}
	};
	
	class SFSPrefix: public TPoolObject<SFSPrefix, kTag_FileSys>
	{
	public:
		typedef TIntrList<SFSPrefix> List;

		MutStr                   prefix;
		TMutArr<IFileProvider *> providers;

		TIntrLink<SFSPrefix>     siblings;
	};

	struct SCoreView
	{
#if DOLL__USE_GLFW
		GLFWwindow *window         = nullptr;
#else
		OSWindow    window         = OSWindow(0);
#endif
		IGfxAPI *   pGfxAPI        = nullptr;
		CGfxFrame * pGfxFrame      = nullptr;
		U32         uDefClearColor = DOLL_RGB( 0x22, 0x44, 0x88 );
	};

	struct SCoreFrame
	{
		U32          uUpdateId       = 0;
		U32          uRenderId       = 0;
		U32          uLimitMillisecs = kFrameLimit60FPS;
		SFrameTiming timing          = SFrameTiming();
	};
	struct SCoreInput
	{
		U32              uFlags = 0; // see `ECoreInputFlag`
	};
	struct SCoreFileSys
	{
		static const UPtr kNumDirs = 0
#define DOLL_ENGINE__FIXED_DIR(RW_,LongName_,ShortName_) + 1
#include "EngineDirs.def.hpp"
#undef DOLL_ENGINE__FIXED_DIR
			;

		SFSPrefix::List prefixes;
		SFSPrefix *     pDefPrefixes[ 16 ] = { nullptr };
		UPtr            cDefPrefixes       = 0;
		TArr<SCoreDir>  dirs;
	};

	struct SCoreAsyncIO
	{
		axthread_t          thread          = AXTHREAD_INITIALIZER;
		axth_sem_t          worksem         = AXTHREAD_SEM_INITIALIZER;
		volatile U32        frameId         = 0;
		CQuickMutex         transferOpsLock;
		TIntrList<CAsyncOp> transferOps;
		CQuickMutex         trashedOpsLock;
		TIntrList<CAsyncOp> trashedOps;
		U32                 cRetries        = kNumAsyncRetries;
		TMutArr<CAsyncOp *> engineOps;
	};

	struct SCoreSound
	{
		U32          uFlags;

		CSoundMixer *pBGMMix;
		CSoundMixer *pSFXMix;
		CSoundMixer *pKoeMix;

		// FIXME: Improve our async op setup

		CAsyncOp *   pBGMOp;
	};

	struct SCoreVersion
	{
		U32 uOfficialVersion;
		Str gitCommitHash;
		Str gitCommitTime;
	};

	struct SCoreMeta
	{
		MutStr studio;
		MutStr name;
	};

	struct SCoreTooling
	{
		Bool         isTool        = false;
		EConsoleType stdoutType    = kConsoleTypeNone;
		EConsoleType stderrType    = kConsoleTypeNone;
		EConsoleType stdinType     = kConsoleTypeNone;
		Bool         useANSIOutput = false;
		Bool         useANSIErrors = false;
	};

	struct SCoreStruc
	{
		SCoreVersion version;
		SCoreMeta    meta;
		SCoreTooling tooling;
		SCoreView    view;
		SCoreFrame   frame;
		SCoreInput   input;
		SCoreFileSys fs;
		SCoreAsyncIO io;
		SCoreSound   sound;

		inline Bool notInitialized() const
		{
			return (
				!view.window ||
				!view.pGfxAPI ||
				!view.pGfxFrame
			) && !tooling.isTool;
		}
	};

#ifdef DOLL__BUILD
	extern SCoreStruc g_core;
# define DOLL__CORESTRUC g_core
#else
# define DOLL__CORESTRUC (*doll_getCoreStruc())
#endif

	DOLL_FUNC SCoreStruc *DOLL_API doll_getCoreStruc();

#if DOLL__USE_GLFW
	DOLL_FUNC OSWindow DOLL_API doll__getOSWindow();
#endif
	inline OSWindow DOLL_API doll_getWindow()
	{
#if DOLL__USE_GLFW
		return doll__getOSWindow();
#else
		return DOLL__CORESTRUC.view.window;
#endif
	}

	inline F64 DOLL_API core_deltaTime()
	{
		return DOLL__CORESTRUC.frame.timing.elapsedSeconds();
	}
	inline U64 DOLL_API core_deltaTimeMicro()
	{
		return DOLL__CORESTRUC.frame.timing.elapsedMicroseconds();
	}
	inline U32 DOLL_API core_deltaTimeMilli()
	{
		return DOLL__CORESTRUC.frame.timing.elapsedMilliseconds();
	}

	inline const SCoreDir &DOLL_API core_getDirData( ECoreDir dir )
	{
		AX_ASSERT( UPtr( U32( dir ) ) < SCoreFileSys::kNumDirs );
		AX_ASSERT( UPtr( U32( dir ) ) < DOLL__CORESTRUC.fs.dirs.num() );

		return DOLL__CORESTRUC.fs.dirs[ U32( dir ) ];
	}
	inline Str DOLL_API core_getDir( ECoreDir dir )
	{
		return core_getDirData( dir ).path;
	}

#define DOLL_ENGINE__FIXED_DIR(RW_,LongName_,ShortName_) \
	inline Str DOLL_API core_get##ShortName_##Dir()\
	{\
		return core_getDirData( ECoreDir::ShortName_ ).path;\
	}
#include "EngineDirs.def.hpp"
#undef DOLL_ENGINE__FIXED_DIR

	inline const char *DOLL_API core_getDirLongName( ECoreDir dir )
	{
		switch( dir ) {
#define DOLL_ENGINE__FIXED_DIR(RW_,LongName_,ShortName_) case ECoreDir::ShortName_: return #LongName_;
#include "EngineDirs.def.hpp"
#undef DOLL_ENGINE__FIXED_DIR
		}

		AX_UNREACHABLE();
		return nullptr;
	}

}
