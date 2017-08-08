# Frontend Commands

Files:

- [Frontend.hpp](../include/doll/Front/Frontend.hpp)
- [Input.hpp](../include/doll/Front/Input.hpp)
- [Setup.hpp](../include/doll/Front/Setup.hpp)


## Front

Contains the fundamental frontend commands, such as initialization and updates.

```cpp
DOLL_FUNC const char *DOLL_API doll_getEngineString();

DOLL_FUNC Void DOLL_API doll_preInit();
DOLL_FUNC Bool DOLL_API doll_init( const SCoreConfig *pConf = nullptr );
DOLL_FUNC Bool DOLL_API doll_initConsoleApp();
DOLL_FUNC Void DOLL_API doll_fini();

DOLL_FUNC Bool DOLL_API doll_sync_app();
DOLL_FUNC Void DOLL_API doll_sync_update();
DOLL_FUNC Void DOLL_API doll_sync_render();
DOLL_FUNC Void DOLL_API doll_sync_timing();

DOLL_FUNC Bool DOLL_API doll_sync();
```

## Input

Handle user input.

```cpp
// Enable the entire input system (enabled by default)
DOLL_FUNC Void DOLL_API in_enableAll();
// Disable the entire input system (useful if you're using your own)
DOLL_FUNC Void DOLL_API in_disableAll();
// Determine whether the input system is enabled
DOLL_FUNC Bool DOLL_API in_allEnabled();

// Retrieve the current state of a given key
DOLL_FUNC Bool DOLL_API in_keyState( EKey, OSWindow = OSWindow(0) );
// Determine whether a key was hit
//
// Decrements the internal count for the number of times the key was hit
DOLL_FUNC Bool DOLL_API in_keyHit( EKey, OSWindow = OSWindow(0) );
// Determine whether a key was released
//
// Decrements the internal count for the number of times the key was released
DOLL_FUNC Bool DOLL_API in_keyReleased( EKey, OSWindow = OSWindow(0) );
// Retrieve the scancode of the last key pressed, or 0
//
// If nonzero this can be safely cast to an `EKey`
DOLL_FUNC U8 DOLL_API in_scancode( OSWindow = OSWindow(0) );

// Retrieve the current state of a given mouse button (0=left, 1=right, 2=middle, 3=thumb1, 4=thumb2)
DOLL_FUNC Bool DOLL_API in_mouseState( EMouse, OSWindow = OSWindow(0) );
// Determines whether a mouse button was hit
//
// Decrements the internal count for the number of times that button was hit
DOLL_FUNC Bool DOLL_API in_mouseHit( EMouse, OSWindow = OSWindow(0) );
// Determines whether a mouse button was released
//
// Decrements the internal count for the number of times that button was released
DOLL_FUNC Bool DOLL_API in_mouseReleased( EMouse, OSWindow = OSWindow(0) );
// Retrieve the current mouse x-position (within the window)
DOLL_FUNC S32 DOLL_API in_mouseX( OSWindow = OSWindow(0) );
// Retrieve the current mouse y-position (within the window)
DOLL_FUNC S32 DOLL_API in_mouseY( OSWindow = OSWindow(0) );
// Retrieve the current mouse position
inline SIntVector2 DOLL_API in_mousePos( OSWindow w = OSWindow( 0 ) );

// Retrieve the current mouse-z position
DOLL_FUNC F32 DOLL_API in_mouseZ( OSWindow = OSWindow(0) );
// Set the current mouse-z position
DOLL_FUNC Void DOLL_API in_setMouseZ( F32 z, OSWindow = OSWindow(0) );

// Retrieves the distance the mouse has moved since the last frame on the x-axis
DOLL_FUNC S32 DOLL_API in_mouseMoveX( OSWindow = OSWindow(0) );
// Retrieves the distance the mouse has moved since the last frame on the y-axis
DOLL_FUNC S32 DOLL_API in_mouseMoveY( OSWindow = OSWindow(0) );
// Retrieves the distance the mouse wheel has scrolled since the last frame
DOLL_FUNC F32 DOLL_API in_mouseMoveZ( OSWindow = OSWindow(0) );
// Resets mouse movement (called internally by `sync()` for the main window)
DOLL_FUNC Void DOLL_API in_resetMouseMove( OSWindow = OSWindow(0) );

// Enable keyboard entry capture for the window
DOLL_FUNC Void DOLL_API in_enableEntry( OSWindow = OSWindow(0) );
// Disable keyboard entry capture for the window
DOLL_FUNC Void DOLL_API in_disableEntry( OSWindow = OSWindow(0) );
// Retrieve whether keyboard entry capture is enabled for the window
DOLL_FUNC Bool DOLL_API in_isEntryEnabled( OSWindow = OSWindow(0) );
// Set the entry buffer for the window
DOLL_FUNC Bool DOLL_API in_setEntry( Str entry, OSWindow = OSWindow(0) );
// Retrieve the entry buffer for the window
DOLL_FUNC Bool DOLL_API in_getEntry( Str &dst, OSWindow = OSWindow(0) );
inline Str DOLL_API in_getEntry( OSWindow wnd = OSWindow( 0 ) );

// Add an input action to the given window (e.g., "Alt+Enter → Fullscreen")
//
// Exact duplicates (within a window's stack) are silently ignored.
//
// There's an internal maximum number of input actions per window. This
// will fail only if that limit is reached.
DOLL_FUNC Bool DOLL_API in_addAction( const SCoreInputAction &act, OSWindow = OSWindow(0) );
// Change the key combination for an input action in the given window
//
// Finds the first key combination that performs the given command
// (`act.command`) and changes its key combination to match those given in
// `act`. If no such command is found then it is added as though
// `in_addAction()` were called with the same parameters.
//
// There's an internal maximum number of input actions per window. This
// will fail only if that limit is reached.
DOLL_FUNC Bool DOLL_API in_setAction( const SCoreInputAction &act, OSWindow = OSWindow(0) );
// Searches for the action exactly matching the given `act` and removes it
// from the window.
DOLL_FUNC Void DOLL_API in_removeAction( const SCoreInputAction &act, OSWindow = OSWindow(0) );
// Determine whether the given window has an action exactly matching `act`.
DOLL_FUNC Bool DOLL_API in_hasAction( const SCoreInputAction &act, OSWindow = OSWindow(0) );

// Enables the default escape key action on the given window.
//
// Pressing the escape key will cause a "quit" command to be issued.
DOLL_FUNC Void DOLL_API in_enableEscapeKey( OSWindow = OSWindow(0) );
// Disables the default escape key action on the given window.
//
// Pressing the escape key will cause a "quit" command to be issued.
DOLL_FUNC Void DOLL_API in_disableEscapeKey( OSWindow = OSWindow(0) );
// Determine whether the default escape key action is enabled for the given
// window.
DOLL_FUNC Bool DOLL_API in_escapeKeyEnabled( OSWindow = OSWindow(0) );

// Enables the default toggle-fullscreen action on the given window.
//
// Pressing either Alt+Enter or F11 will toggle between fullscreen mode and
// windowed mode by default.
DOLL_FUNC Void DOLL_API in_enableToggleFullscreen( OSWindow = OSWindow(0) );
//
// Pressing either Alt+Enter or F11 will toggle between fullscreen mode and
// windowed mode by default.
DOLL_FUNC Void DOLL_API in_disableToggleFullscreen( OSWindow = OSWindow(0) );
// Determine whether the default toggle-fullscreen action is enabled for the
// given window.
DOLL_FUNC Bool DOLL_API in_toggleFullscreenEnabled( OSWindow = OSWindow(0) );

// Enables the default screenshot capture action on the given window.
//
// Pressing F2 will issue a capture screenshot action by default.
DOLL_FUNC Void DOLL_API in_enableScreenshot( OSWindow = OSWindow(0) );
// Disables the default screenshot capture action on the given window.
//
// Pressing F2 will issue a capture screenshot action by default.
DOLL_FUNC Void DOLL_API in_disableScreenshot( OSWindow = OSWindow(0) );
// Determine whether the default screenshot capture action is enabled for
// the given window.
DOLL_FUNC Bool DOLL_API in_screenshotEnabled( OSWindow = OSWindow(0) );

// Enables the default developer console open action on the given window.
//
// Pressing the "`"/"~" key will open the developer console by default.
DOLL_FUNC Void DOLL_API in_enableDevcon( OSWindow = OSWindow(0) );
// Disables the default developer console open action on the given window.
//
// Pressing the "`"/"~" key will open the developer console by default.
DOLL_FUNC Void DOLL_API in_disableDevcon( OSWindow = OSWindow(0) );
// Determine whether the default developer console open action is enabled
// for the given window.
DOLL_FUNC Bool DOLL_API in_devconEnabled( OSWindow = OSWindow(0) );

// Enables the default developer debug display toggle action on the given
// window.
//
// Pressing F3 will toggle the debug display by default.
DOLL_FUNC Void DOLL_API in_enableDevDebug( OSWindow = OSWindow(0) );
// Disables the default developer debug display toggle action on the given
// window.
//
// Pressing F3 will toggle the debug display by default.
DOLL_FUNC Void DOLL_API in_disableDevDebug( OSWindow = OSWindow(0) );
// Determine whether the default debug display toggle action is enabled for
// the given wiindow.
DOLL_FUNC Bool DOLL_API in_devDebugEnabled( OSWindow = OSWindow(0) );
```

## Setup

Handles front-end setup/configuration.

```cpp
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
	} video;

	struct SAudio
	{
		char szDevices [ 256 ] = { '\0' };
		char szChannels[  64 ] = { '\0' }; // ([1-9](\.[0-9])?) | { L R C S Lb Rb Lf Rf Cb Ls Rs }
		U32  cSamplesHz        = 0;
	} audio;

	inline SUserConfig &setResolution( U32 uResX, U32 uResY );
	inline SUserConfig &setFullscreen( Bool bFullscreen = true );
	inline SUserConfig &setVsync( S32 iVsync );
	inline SUserConfig &setFrameLimit( U32 uFrameLimitFPS );

	// Set the render API
	//
	// Should be one of:
	// - "" (use default set for this platform)
	// - "dx", "d3d", or "d3d11"
	// - "gl", "ogl", or "opengl"
	// - "vk", or "vulkan"
	// - "d3d12"
	inline SUserConfig &setRenderAPI( const Str &api );
	// Add another render API to try if the prior fails
	//
	// Should be one of:
	// - "dx", "d3d", or "d3d11"
	// - "gl", "ogl", or "opengl"
	// - "vk", or "vulkan"
	// - "d3d12"
	inline SUserConfig &addRenderAPI( const Str &api );

	// Set the render API
	inline SUserConfig &setRenderAPI( EGfxAPI api );
	// Add another render API if the prior fails
	inline SUserConfig &addRenderAPI( EGfxAPI api );
};

struct SCoreConfig: public SUserConfig
{
	struct SMeta
	{
		char szStudio[ 128 ];
		char szName  [ 128 ];

		inline SMeta();
	} meta;
	struct SBaseFS
	{
		static const UPtr kMaxPath = 256;

#define DOLL_ENGINE__DIR(RW_,LongName_,ShortName_,DefVal_) char sz##ShortName_##Dir[ kMaxPath ];
#include "../Core/EngineDirs.def.hpp"
#undef DOLL_ENGINE__DIR

		inline SBaseFS();
	} baseFS;
	struct SScript
	{
		char szTitle[ 128 ];
		U32  clearColor = 0x805020;

		inline SScript();
	} script;

	inline SCoreConfig();

	inline SCoreConfig &setStudio( const Str &studio );
	inline SCoreConfig &setName( const Str &name );

#define DOLL_ENGINE__DIR(RW_,LongName_,ShortName_,DefVal_) \
inline SCoreConfig &set##ShortName_##Dir( const Str &dirname );
#include "../Core/EngineDirs.def.hpp"
#undef DOLL_ENGINE__DIR

	inline SCoreConfig &setTitle( const Str &title );
	inline SCoreConfig &setClearColor( U32 uColorRGBA );
	inline SCoreConfig &setClearColor( F32 r, F32 g, F32 b, F32 a = 1.0f );
};

DOLL_FUNC Bool DOLL_API app_loadUserConfig( SUserConfig &dstConf, Str filename );
inline SUserConfig DOLL_API app_loadUserConfig( Str filename );
DOLL_FUNC Bool DOLL_API app_loadConfig( SCoreConfig &dstConf, Str filename );
inline SCoreConfig DOLL_API app_loadConfig( Str filename );

DOLL_FUNC Bool DOLL_API app_convertArgs( TArr<Str> &dst, unsigned argc, const char *const *argv );
DOLL_FUNC Bool DOLL_API app_convertArgsW( TArr<Str> &dst, unsigned argc, const wchar_t *const *wargv );
inline Bool DOLL_API app_convertArgs( TArr<Str> &dst, unsigned argc, const wchar_t *const *wargv );
inline TArr<Str> DOLL_API app_convertArgs( unsigned argc, const char *const *argv );
inline TArr<Str> DOLL_API app_convertArgs( unsigned argc, const wchar_t *const *wargv );
```
