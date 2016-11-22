#pragma once

#include "../Core/Defs.hpp"
#include "../Core/Engine.hpp"
#include "../OS/Key.hpp"
#include "../OS/Window.hpp"

namespace doll
{

	enum class EWndReply;

	// Callback for when a window gains keyboard focus
	DOLL_FUNC EWndReply in_onAcceptKey_f( OSWindow );
	// Callback for when a window loses keyboard focus
	DOLL_FUNC EWndReply in_onResignKey_f( OSWindow );

	// Callback for when a key is pressed (or automatically repeated)
	DOLL_FUNC EWndReply in_onKeyPress_f( OSWindow, EKey, U32 uModFlags, Bool isRepeat );
	// Callback for when a key is released
	DOLL_FUNC EWndReply in_onKeyRelease_f( OSWindow, EKey, U32 uModFlags );
	// Callback for when a character (or action, such as backspace) is determined from the key presses
	DOLL_FUNC EWndReply in_onKeyChar_f( OSWindow, U32 utf32Char );

	// Callback for when a mouse button is pressed (0=left, 1=right, 2=middle, 3=thumb1, 4=thumb2)
	DOLL_FUNC EWndReply in_onMousePress_f( OSWindow, EMouse, S32 clientPosX, S32 clientPosY, U32 uModFlags );
	// Callback for when a mouse button is released
	DOLL_FUNC EWndReply in_onMouseRelease_f( OSWindow, EMouse, S32 clientPosX, S32 clientPosY, U32 uModFlags );
	// Callback for when the mouse wheel is scrolled
	DOLL_FUNC EWndReply in_onMouseWheel_f( OSWindow, F32 fDelta, S32 clientPosX, S32 clientPosY, U32 uModFlags );
	// Callback for when the mouse is moved
	DOLL_FUNC EWndReply in_onMouseMove_f( OSWindow, S32 clientPosX, S32 clientPosY, U32 uModFlags );
	// Callback for when the mouse leaves the window
	DOLL_FUNC EWndReply in_onMouseExit_f( OSWindow, U32 uModFlags );

	// ------------------------------------------------------------------ //

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
	inline SIntVector2 DOLL_API in_mousePos( OSWindow w = OSWindow( 0 ) )
	{
		return SIntVector2( in_mouseX( w ), in_mouseY( w ) );
	}

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
	inline Str DOLL_API in_getEntry( OSWindow wnd = OSWindow( 0 ) )
	{
		Str r;
		return in_getEntry( r, wnd ), r;
	}

	// Add an input action to the given window (e.g., "Alt+Enter Å® Fullscreen")
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

}
