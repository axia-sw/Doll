# Core Commands

Files:

- [Config.hpp](../include/doll/Core/Config.hpp)
- [Defs.hpp](../include/doll/Core/Defs.hpp)
- [Engine.hpp](../include/doll/Core/Engine.hpp)
- [EngineDirs.def.hpp](../include/doll/Core/EngineDirs.def.hpp)
- [Logger.hpp](../include/doll/Core/Logger.hpp)
- [Memory.hpp](../include/doll/Core/Memory.hpp)
- [MemoryTags.hpp](../include/doll/Core/MemoryTags.hpp)
- [Version.hpp](../include/doll/Core/Version.hpp)

## Config

Configuration subsystem.

```cpp
DOLL_FUNC CConfiguration *DOLL_API core_newConfig();
DOLL_FUNC CConfiguration *DOLL_API core_deleteConfig( CConfiguration *config );

DOLL_FUNC CConfiguration *DOLL_API core_loadConfig( Str filename );
DOLL_FUNC Bool DOLL_API core_appendConfig( CConfiguration *config, Str filename );
DOLL_FUNC Bool DOLL_API core_appendConfigString( CConfiguration *config, Str filename, Str source );

DOLL_FUNC Bool DOLL_API core_queryConfigValue( CConfiguration *config, Str &out_value, Str section, Str key );
inline Str DOLL_API core_getConfigValue( CConfiguration *config, Str section, Str key );

DOLL_FUNC Void DOLL_API core_clearConfig( CConfiguration *config );

DOLL_FUNC SConfigVar *DOLL_API core_findConfigSection( CConfiguration *config, Str name );
DOLL_FUNC SConfigVar *DOLL_API core_findConfigVar( SConfigVar *prnt, Str name );
DOLL_FUNC SConfigValue *DOLL_API core_findConfigVarValue( SConfigVar *var, Str value );

DOLL_FUNC SConfigVar *DOLL_API core_getFirstConfigSection( CConfiguration *config );
DOLL_FUNC SConfigVar *DOLL_API core_getLastConfigSection( CConfiguration *config );
DOLL_FUNC SConfigVar *DOLL_API core_getFirstConfigVar( SConfigVar *prnt );
DOLL_FUNC SConfigVar *DOLL_API core_getLastConfigVar( SConfigVar *prnt );
DOLL_FUNC SConfigVar *DOLL_API core_getConfigVarBefore( SConfigVar *var );
DOLL_FUNC SConfigVar *DOLL_API core_getConfigVarAfter( SConfigVar *var );
DOLL_FUNC SConfigVar *DOLL_API core_getConfigVarParent( SConfigVar *var );

DOLL_FUNC Bool DOLL_API core_getConfigVarName( Str &dst, SConfigVar *var );
DOLL_FUNC Bool DOLL_API core_getConfigVarValue( Str &dst, SConfigVar *var );
inline Str DOLL_API core_getConfigVarName( SConfigVar *var );
inline Str DOLL_API core_getConfigVarValue( SConfigVar *var );

DOLL_FUNC SConfigValue *DOLL_API core_getFirstConfigVarValue( SConfigVar *var );
DOLL_FUNC SConfigValue *DOLL_API core_getLastConfigVarValue( SConfigVar *var );
DOLL_FUNC SConfigValue *DOLL_API core_getConfigValueBefore( SConfigValue *val );
DOLL_FUNC SConfigValue *DOLL_API core_getConfigValueAfter( SConfigValue *val );
DOLL_FUNC Bool DOLL_API core_getConfigValueString( Str &dst, SConfigValue *val );
inline Str DOLL_API core_getConfigValueString( SConfigValue *val );

DOLL_FUNC SConfigVar *DOLL_API core_removeConfigVar( SConfigVar *var );
```

## Defs

General definitions; included everywhere.

```cpp
#define DOLL_RGBA(R_,G_,B_,A_)
#define DOLL_RGB(R_,G_,B_)

#define DOLL_COLOR_R(RGBA_)
#define DOLL_COLOR_G(RGBA_)
#define DOLL_COLOR_B(RGBA_)
#define DOLL_COLOR_A(RGBA_)

DOLL_FUNC Bool DOLL_API app_getPath( Str &dst );
inline Str DOLL_API app_getPath();
inline Str DOLL_API app_getDir();

inline Str DOLL_API app_getName();

AX_CONSTEXPR_INLINE U32 doll_rgb( F32 r, F32 g, F32 b, F32 a = 1.0f );

template< typename T, UPtr tNum >
inline Bool isOneOf( const T x, const T( &buf )[ tNum ] );
template< typename T, typename... Args >
inline Bool isOneOf( const T &x, Args... args );

template< typename T >
inline U16 countBits( T x );

inline Bool takeFlag( U32 &uFlags, U32 uCheck );
```

## Engine

This part mostly just contains enumerations and structures for tying everything
together. Possibly the most important structure in Doll is `SCoreStruc`, which
holds Doll's entire state and serves as the communication channel for various
subsystems.

```cpp
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

	SCoreDir( ECoreDir kind, Bool isConfigurable, Bool isWritable, const Str &prefix );

	Bool is( ECoreDir x ) const;

	Bool isConfigurable() const;
	Bool isWritable() const;

	SCoreDir &operator=( const Str &x );
	SCoreDir &operator/=( const Str &x );

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

	inline SFrameTiming();
	inline ~SFrameTiming();

	inline SFrameTiming &updateMe( U64 uNewMicrosecs );

	inline U64 elapsedNanoseconds() const;
	inline U64 elapsedMicroseconds() const;
	inline U32 elapsedMilliseconds() const;
	inline F64 elapsedSeconds() const;
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
	U32 uFlags = 0; // see `ECoreInputFlag`
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

	CAsyncOp *   pBGMOp;
};

struct SCoreVersion
{
	U32      uOfficialVersion;
	Str      gitCommitHash;
	Str      gitCommitTime;
	EVariant buildVariant; // development, debug, profile, or release
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

	inline Bool notInitialized() const;
};

#ifdef DOLL__BUILD
extern SCoreStruc g_core;
# define DOLL__CORESTRUC g_core
#else
# define DOLL__CORESTRUC (*doll_getCoreStruc())
#endif

DOLL_FUNC SCoreStruc *DOLL_API doll_getCoreStruc();

inline EVariant doll_buildVariant();
inline Bool doll_isReleaseBuild();
inline Bool doll_isProfileBuild();
inline Bool doll_isDebugBuild();
inline Bool doll_isDevelopmentBuild();
inline Str doll_getVariantName();

#if DOLL__USE_GLFW
DOLL_FUNC OSWindow DOLL_API doll__getOSWindow();
#endif
inline OSWindow DOLL_API doll_getWindow();

inline F64 DOLL_API core_deltaTime();
inline U64 DOLL_API core_deltaTimeMicro();
inline U32 DOLL_API core_deltaTimeMilli();

inline const SCoreDir &DOLL_API core_getDirData( ECoreDir dir );
inline Str DOLL_API core_getDir( ECoreDir dir );

#define DOLL_ENGINE__FIXED_DIR(RW_,LongName_,ShortName_) \
	inline Str DOLL_API core_get##ShortName_##Dir()\
	{\
		return core_getDirData( ECoreDir::ShortName_ ).path;\
	}
#include "EngineDirs.def.hpp"
#undef DOLL_ENGINE__FIXED_DIR

inline const char *DOLL_API core_getDirLongName( ECoreDir dir );
```

Engine directories; `EngineDirs.def.hpp`:

```cpp
// NOTE: In the following directory listings, "<AppName>" refers to a
//       potentially configurable application name. This may actually be a
//       company name followed by an app name. e.g., "Key/Rewrite" such that a
//       separate shared directory exists.

// Directory the process launched in
DOLL_ENGINE__FIXED_DIR(R, Launch      , Launch    ) // core_getLaunchDir()
// Directory of the executable
DOLL_ENGINE__FIXED_DIR(R, App         , App       ) // core_getAppDir()
// Directory of the main configuration file (or $App: if invalid)
DOLL_ENGINE__FIXED_DIR(R, MainConfig  , Cfg       ) // core_getCfgDir()
// Local application data folder partitioned for this app ("%LocalAppData%/<AppName>")
DOLL_ENGINE__FIXED_DIR(W, LocalAppData, AppData   ) // core_getAppDataDir()
// Roaming application data folder partitioned for this app ("%AppData%/../Roaming/<AppName>")
DOLL_ENGINE__FIXED_DIR(W, CloudAppData, Roaming   ) // core_getRoamingDir()
// User's "My Documents" directory (e.g., "%UserProfile%/Documents")
DOLL_ENGINE__FIXED_DIR(R, Documents   , Docs      ) // core_getDocsDir()
// Writable location for documents specific to the app ("$Docs:<AppName>")
DOLL_ENGINE__FIXED_DIR(W, AppDocuments, AppDocs   ) // core_getAppDocsDir()

// Root folder of the game (e.g., "$App:GameData")
DOLL_ENGINE__DIR(R,Root            , Root   , "$App:"          ) // core_getRootDir()

// Folder that saves are written to
DOLL_ENGINE__DIR(W,Saves           , Sav    , "$AppDocs:sav"   ) // core_getSavDir()
// Folder that logs are written to
DOLL_ENGINE__DIR(W,Logs            , Log    , "$AppData:log"   ) // core_getLogDir()

// Abstract / miscellaneous data folder
DOLL_ENGINE__DIR(R,Data            , Dat    , "$Root:dat"      ) // core_getDatDir()
// Fonts and font data folder
DOLL_ENGINE__DIR(R,Fonts           , Fon    , "$Dat:"          ) // core_getFonDir()
// Scripts/scenes folder
DOLL_ENGINE__DIR(R,Scripts         , Scr    , "$Dat:scn"       ) // core_getScrDir()

// Top-level graphics folder
DOLL_ENGINE__DIR(R,Graphics        , Gfx    , "$Root:gfx"      ) // core_getGfxDir()
// Prerendered movies
DOLL_ENGINE__DIR(R,Movies          , Mov    , "$Gfx:av"        ) // core_getMovDir()
// Background images
DOLL_ENGINE__DIR(R,Backgrounds     , BG     , "$Gfx:bg"        ) // core_getBGDir()
// Gallery-enabled background images
DOLL_ENGINE__DIR(R,ComputerGraphics, CG     , "$Gfx:cg"        ) // core_getCGDir()
// Character descriptors folder
DOLL_ENGINE__DIR(R,Characters      , Chr    , "$Gfx:chr"       ) // core_getChrDir()

// Top-level sounds folder
DOLL_ENGINE__DIR(R,Sounds          , Snd    , "$Root:snd"      ) // core_getSndDir()
// Background music
DOLL_ENGINE__DIR(R,BackgroundMusic , BGM    , "$Snd:bgm"       ) // core_getBGMDir()
// Voice tracks
DOLL_ENGINE__DIR(R,Koe             , Koe    , "$Snd:koe"       ) // core_getKoeDir()
// Sound effects
DOLL_ENGINE__DIR(R,SoundEffects    , SE     , "$Snd:sfx"       ) // core_getSEDir()
```

## Logger

```cpp
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

	inline SReportDetails();
	inline SReportDetails( ESeverity Sev, Str file = Str(), U32 uLine = 0, U32 uColumn = 0, Str function = Str() );
	inline SReportDetails( ESeverity Sev, Str file, U32 uLine, Str function );
	inline SReportDetails( ESeverity Sev, int From, Str file = Str(), U32 uLine = 0, U32 uColumn = 0, Str function = Str() );
	inline SReportDetails( ESeverity Sev, int From, Str file, U32 uLine, Str function );
	inline SReportDetails( const SReportDetails &x );

	inline SReportDetails &operator=( const SReportDetails &x );
};

// Base interface class for reporters
class IReporter
{
public:
	// Constructor
	IReporter();
	// Destructor
	virtual ~IReporter();

	// Handle a report
	virtual void report( const SReportDetails &details, Str message ) = 0;
};

// Submit a report to all listening reporters
DOLL_FUNC void DOLL_API core_report( const SReportDetails &details, Str message );

inline void core_report( ESeverity Sev, Str file, int uLine, Str message );

// Add a reporter interface for handling reports
DOLL_FUNC void DOLL_API core_addReporter( IReporter *r );

// Remove an added reporter interface
DOLL_FUNC void DOLL_API core_removeReporter( IReporter *r );

// Report a warning
inline void warnf( Str file, int uLine, const char *pszFmt, ... );
// Report an error
inline void errorf( Str file, int uLine, const char *pszFmt, ... );
// Report debugging information
inline void debugf( Str file, int uLine, const char *pszFmt, ... );
// Report status
inline void statusf( Str file, int uLine, const char *pszFmt, ... );

// Report a warning
inline void warnf( Str file, const char *pszFmt, ... );
// Report an error
inline void errorf( Str file, const char *pszFmt, ... );
// Report debugging information
inline void debugf( Str file, const char *pszFmt, ... );
// Report status
inline void statusf( Str file, const char *pszFmt, ... );

// Report a warning
inline void basicWarnf( const char *pszFmt, ... );
// Report an error
inline void basicErrorf( const char *pszFmt, ... );
// Report debugging information
inline void basicDebugf( const char *pszFmt, ... );
// Report status
inline void basicStatusf( const char *pszFmt, ... );
// Same as basicStatusf (report status) -- includes newline in output
inline void notef( const char *pszFmt, ... );

// Install the debug.log reporter
DOLL_FUNC void DOLL_API core_installDebugLogReporter();
// Uninstall the debug.log reporter
DOLL_FUNC void DOLL_API core_uninstallDebugLogReporter();

// Install the colored console reporter
DOLL_FUNC void DOLL_API core_installConsoleReporter();
// Uninstall the colored console reporter
DOLL_FUNC void DOLL_API core_uninstallConsoleReporter();
```
