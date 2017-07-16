#pragma once

/*

	!!! STOP !!!

	This library is only meant for use internally.
	
	If you're writing a GUI based application then use the SystemUI library
	instead.

	------------------------------------------------------------------------

	This interface operates directly on the native window handles. Internal
	data structures may be assigned to the native handles depending on the
	platform's implementation.

*/

#include "../Core/Defs.hpp"

#include "../Math/IntVector2.hpp"
#include "../Math/Rect.hpp"

#include "Key.hpp"

namespace doll
{

	// Forward declarations
	struct SNativeWindow;

	// Handle to a window
	typedef SNativeWindow *OSWindow;

	// Flags detailing how a window should be styled
	enum EWndStyleFlags
	{
		// window will not have a close button
		kWSF_NoCloseButton    = 1<<0,
		// window will not have a minimize button
		kWSF_NoMinimizeButton = 1<<1,
		// window will not have a maximize button
		kWSF_NoMaximizeButton = 1<<2,
		// window will not be resizeable
		kWSF_NoResize         = 1<<3,
		// window will have a thin title bar (like tool windows)
 		kWSF_ThinCaption      = 1<<4,
		// window will not have any system-supplied parts (like caption, border, etc)
		kWSF_NoOSParts        = 1<<5,
		// window is top-most
		kWSF_TopMost          = 1<<6,
		// window supports translucency
		kWSF_Translucent      = 1<<7
	};

	// Modifier key flags
	enum EModKeyFlags
	{
		kMF_LShift   = 1<<0,
		kMF_RShift   = 1<<1,
		kMF_LAlt     = 1<<2,
		kMF_RAlt     = 1<<3,
		kMF_LControl = 1<<4,
		kMF_RControl = 1<<5,
		kMF_Mouse1   = 1<<6,
		kMF_Mouse2   = 1<<7,
		kMF_Mouse3   = 1<<8,
		kMF_Mouse4   = 1<<9,
		kMF_Mouse5   = 1<<10,

		kMF_LCtrl    = kMF_LControl,
		kMF_RCtrl    = kMF_RControl,

		kMF_Shift    = kMF_LShift | kMF_RShift,
		kMF_Alt      = kMF_LAlt | kMF_RAlt,
		kMF_Control  = kMF_LControl | kMF_RControl,
		kMF_Ctrl     = kMF_Control
	};

	// Reply to an event
	enum class EWndReply
	{
		// The event was not handled
		NotHandled = 0,
		// The event has been handled
		Handled
	};

	// Hit test results
	enum class EHitTest
	{
		// The event was not handled
		NotHandled = 0,

		// Nothing was in this area
		Miss,

		// Client area (main content part of the window)
		ClientArea,
		// Caption area (where the title text and buttons go) -- allows window dragging
		TitleBar,
		// System menu (where the icon for the system menu is)
		SystemMenu,
		// Minimize button
		MinimizeButton,
		// Maximize/restore button
		MaximizeButton,
		// Close button
		CloseButton,
		// Top border of the window (for resize)
		TopSizer,
		// Top-right border of the window (for resize)
		TopRightSizer,
		// Right border of the window (for resize)
		RightSizer,
		// Bottom-right border of the window (for resize)
		BottomRightSizer,
		// Bottom border of the window (for resize)
		BottomSizer,
		// Bottom-left border of the window (for resize)
		BottomLeftSizer,
		// Left border of the window (for resize)
		LeftSizer,
		// Top-left border of the window (for resize)
		TopLeftSizer
	};

	// Just before a window is closed
	typedef EWndReply( *FnOnClose )( OSWindow );
	// Just before a window is minimized
	typedef EWndReply( *FnOnMinimize )( OSWindow );
	// Just before a window is maximized
	typedef EWndReply( *FnOnMaximize )( OSWindow );

	// Just after a window is "closed" (but not yet destroyed)
	typedef EWndReply( *FnOnClosed )( OSWindow );
	// Just after a window is minimized
	typedef EWndReply( *FnOnMinimized )( OSWindow );
	// Just after a window is maximized
	typedef EWndReply( *FnOnMaximized )( OSWindow );

	// When the application itself becomes active
	typedef EWndReply( *FnOnAppActivate )( OSWindow );
	// When the application itself is put in the background
	typedef EWndReply( *FnOnAppDeactivate )( OSWindow );

	// When a window becomes the "main window" of the app (front)
	typedef EWndReply( *FnOnAcceptMain )( OSWindow );
	// When a window is no longer the "main window" of the app
	typedef EWndReply( *FnOnResignMain )( OSWindow );

	// When a window gains keyboard focus
	typedef EWndReply( *FnOnAcceptKey )( OSWindow );
	// When a window loses keyboard focus
	typedef EWndReply( *FnOnResignKey )( OSWindow );

	// When a window becomes enabled
	typedef EWndReply( *FnOnEnabled )( OSWindow );
	// When a window becomes disabled
	typedef EWndReply( *FnOnDisabled )( OSWindow );

	// When a window becomes visible
	typedef EWndReply( *FnOnVisible )( OSWindow );
	// When a window becomes invisible
	typedef EWndReply( *FnOnInvisible )( OSWindow );

	// Just after (or during) a resize event
	typedef EWndReply( *FnOnSized )( OSWindow, U32 clientResX, U32 clientResY );
	// Just after (or during) a movement event
	typedef EWndReply( *FnOnMoved )( OSWindow, S32 framePosX, S32 framePosY );

	// When a window is being resized and its resolution needs to be checked - the parameters can be modified to adjust
	typedef EWndReply( *FnOnSizing )( OSWindow, S32 &clientResX, S32 &clientResY );
	// When a window is being moved and its location needs to be checked - the parameters can be modified to adjust
	typedef EWndReply( *FnOnMoving )( OSWindow, S32 &frameLeft, S32 &frameTop, S32 &frameRight, S32 &frameBottom );

	// Sent when a key is pressed (or automatically repeated)
	typedef EWndReply( *FnOnKeyPress )( OSWindow, EKey, U32 uModFlags, Bool bIsRepeat );
	// Sent when a key is released
	typedef EWndReply( *FnOnKeyRelease )( OSWindow, EKey, U32 uModFlags );
	// Sent automatically to designate a given character from a combination of key presses
	typedef EWndReply( *FnOnKeyChar )( OSWindow, U32 utf32Char );

	// Sent when a mouse button is pressed
	typedef EWndReply( *FnOnMousePress )( OSWindow, EMouse, S32 clientPosX, S32 clientPosY, U32 uModFlags );
	// Sent when a mouse button is released
	typedef EWndReply( *FnOnMouseRelease )( OSWindow, EMouse, S32 clientPosX, S32 clientPosY, U32 uModFlags );
	// Sent when the mouse wheel is scrolled
	typedef EWndReply( *FnOnMouseWheel )( OSWindow, float fDelta, S32 clientPosX, S32 clientPosY, U32 uModFlags );
	// Sent when the mouse moves within the window
	typedef EWndReply( *FnOnMouseMove )( OSWindow, S32 clientPosX, S32 clientPosY, U32 uModFlags );
	// Sent when a mouse leaves a window
	typedef EWndReply( *FnOnMouseExit )( OSWindow, U32 uModFlags );

	// Sent to determine where the mouse is within a window
	typedef EHitTest( *FnOnHitTest )( OSWindow, S32 mouseScreenPosX, S32 mouseScreenPosY, S32 mouseFramePosX, S32 mouseFramePosY );
	// Sent to a window to notify it that the title is changed
	typedef EWndReply( *FnOnChangeTitle )( OSWindow, Str title );

	// Event handling chain
	struct SWndDelegate
	{
		// Next delegate to try if the event is unhandled
		SWndDelegate *    pSuper;

		// Just before a window is closed
		FnOnClose         pfnOnClose;
		// Just before a window is minimized
		FnOnMinimize      pfnOnMinimize;
		// Just before a window is maximized
		FnOnMaximize      pfnOnMaximize;

		// Just after a window is "closed" (but not yet destroyed)
		FnOnClosed        pfnOnClosed;
		// Just after a window is minimized
		FnOnMinimized     pfnOnMinimized;
		// Just after a window is maximized
		FnOnMaximized     pfnOnMaximized;

		// When the application itself becomes active
		FnOnAppActivate   pfnOnAppActivate;
		// When the application itself is put in the background
		FnOnAppDeactivate pfnOnAppDeactivate;

		// When a window becomes the "main window" of the app (front)
		FnOnAcceptMain    pfnOnAcceptMain;
		// When a window is no longer the "main window" of the app
		FnOnResignMain    pfnOnResignMain;

		// When a window gains keyboard focus
		FnOnAcceptKey     pfnOnAcceptKey;
		// When a window loses keyboard focus
		FnOnResignKey     pfnOnResignKey;

		// When a window becomes enabled
		FnOnEnabled       pfnOnEnabled;
		// When a window becomes disabled
		FnOnDisabled      pfnOnDisabled;

		// When a window becomes visible
		FnOnVisible       pfnOnVisible;
		// When a window becomes invisible
		FnOnInvisible     pfnOnInvisible;

		// Just after (or during) a resize event
		FnOnSized         pfnOnSized;
		// Just after (or during) a movement event
		FnOnMoved         pfnOnMoved;

		// When a window is being resized and its resolution needs to be checked - the parameters can be modified to adjust
		FnOnSizing        pfnOnSizing;
		// When a window is being moved and its location needs to be checked - the parameters can be modified to adjust
		FnOnMoving        pfnOnMoving;

		// Sent when a key is pressed (or automatically repeated)
		FnOnKeyPress      pfnOnKeyPress;
		// Sent when a key is released
		FnOnKeyRelease    pfnOnKeyRelease;
		// Sent automatically to designate a given character from a combination of key presses
		FnOnKeyChar       pfnOnKeyChar;

		// Sent when a mouse button is pressed (0 = left, 1 = right, 2 = middle, 3 = thumb 1, 4 = thumb 2)
		FnOnMousePress    pfnOnMousePress;
		// Sent when a mouse button is released
		FnOnMouseRelease  pfnOnMouseRelease;
		// Sent when the mouse wheel is scrolled
		FnOnMouseWheel    pfnOnMouseWheel;
		// Sent when the mouse moves within the window
		FnOnMouseMove     pfnOnMouseMove;
		// Sent when a mouse leaves a window
		FnOnMouseExit     pfnOnMouseExit;

		// Sent to determine where the mouse is within a window
		FnOnHitTest       pfnOnHitTest;
		// Sent to a window to notify it that the title is changed
		FnOnChangeTitle   pfnOnChangeTitle;

		inline SWndDelegate()
		: pSuper            ( nullptr )
		, pfnOnClose        ( nullptr )
		, pfnOnMinimize     ( nullptr )
		, pfnOnMaximize     ( nullptr )
		, pfnOnClosed       ( nullptr )
		, pfnOnMinimized    ( nullptr )
		, pfnOnMaximized    ( nullptr )
		, pfnOnAppActivate  ( nullptr )
		, pfnOnAppDeactivate( nullptr )
		, pfnOnAcceptMain   ( nullptr )
		, pfnOnResignMain   ( nullptr )
		, pfnOnAcceptKey    ( nullptr )
		, pfnOnResignKey    ( nullptr )
		, pfnOnEnabled      ( nullptr )
		, pfnOnDisabled     ( nullptr )
		, pfnOnVisible      ( nullptr )
		, pfnOnInvisible    ( nullptr )
		, pfnOnSized        ( nullptr )
		, pfnOnMoved        ( nullptr )
		, pfnOnSizing       ( nullptr )
		, pfnOnMoving       ( nullptr )
		, pfnOnKeyPress     ( nullptr )
		, pfnOnKeyRelease   ( nullptr )
		, pfnOnKeyChar      ( nullptr )
		, pfnOnMousePress   ( nullptr )
		, pfnOnMouseRelease ( nullptr )
		, pfnOnMouseWheel   ( nullptr )
		, pfnOnMouseMove    ( nullptr )
		, pfnOnMouseExit    ( nullptr )
		, pfnOnHitTest      ( nullptr )
		, pfnOnChangeTitle  ( nullptr )
		{
		}
	};

	// window creation parameters
	struct SWndCreateInfo
	{
		// UTF-8 encoded title string
		Str           title;
		// Shape of the window
		SRect         shape;
		// Style flags for the window
		U32           uStyleFlags;
		// Delegate to handle events for the window
		SWndDelegate *pDelegate;
		// Extra data pointer for the window
		Void *        pData;

		inline SWndCreateInfo()
		: title()
		, shape()
		, uStyleFlags( 0 )
		, pDelegate( nullptr )
		, pData( nullptr )
		{
		}
	};

	// Create a window
	DOLL_FUNC OSWindow DOLL_API wnd_open( const SWndCreateInfo &Info );
	// Destroy a window
	DOLL_FUNC Void DOLL_API wnd_close( OSWindow window );
	// Minimize a window
	DOLL_FUNC Void DOLL_API wnd_minimize( OSWindow window );
	// Maximize a window
	DOLL_FUNC Void DOLL_API wnd_maximize( OSWindow window );

	// Add a delegate to the window's chain
	DOLL_FUNC Void DOLL_API wnd_addDelegate( OSWindow window, const SWndDelegate &wndDelegate );
	// Remove a delegate from the window's chain
	DOLL_FUNC Void DOLL_API wnd_removeDelegate( OSWindow window, const SWndDelegate &wndDelegate );

	// Set the title of a window
	DOLL_FUNC Void DOLL_API wnd_setTitle( OSWindow window, Str title );
	// Retrieve the title of a window
	DOLL_FUNC UPtr DOLL_API wnd_getTitle( OSWindow window, char *pszOutTitleUTF8, UPtr cMaxOutBytes );

	// Set the data of a window
	DOLL_FUNC Void DOLL_API wnd_setData( OSWindow window, Void *pData );
	// Retrieve the data of a window
	DOLL_FUNC Void *DOLL_API wnd_getData( OSWindow window );

	// Send the close message to a window
	DOLL_FUNC Void DOLL_API wnd_performClose( OSWindow window );
	// Send the minimize message to a window
	DOLL_FUNC Void DOLL_API wnd_performMinimize( OSWindow window );
	// Send the maximize message to a window
	DOLL_FUNC Void DOLL_API wnd_performMaximize( OSWindow window );

	// Set a window to be visible or invisible
	DOLL_FUNC Void DOLL_API wnd_setVisible( OSWindow window, Bool bIsVisible = true );
	// Determine whether a window is visible
	DOLL_FUNC Bool DOLL_API wnd_isVisible( OSWindow window );

	// Set a window to be enabled or disabled
	DOLL_FUNC Void DOLL_API wnd_setEnabled( OSWindow window, Bool bIsEnabled = true );
	// Determine whether a window is enabled
	DOLL_FUNC Bool DOLL_API wnd_isEnabled( OSWindow window );

	// Resize a window's client area
	DOLL_FUNC Void DOLL_API wnd_resize( OSWindow window, U32 resX, U32 resY );
	// Resize a window's screen area
	DOLL_FUNC Void DOLL_API wnd_resizeFrame( OSWindow window, U32 resX, U32 resY );

	// Position a window
	DOLL_FUNC Void DOLL_API wnd_position( OSWindow window, S32 posX, S32 posY );

	// Retrieve the size of a window's client area
	DOLL_FUNC Void DOLL_API wnd_getSize( OSWindow window, U32 &outResX, U32 &outResY );
	// Retrieve the size of a window's screen area
	DOLL_FUNC Void DOLL_API wnd_getFrameSize( OSWindow window, U32 &outResX, U32 &outResY );

	// Retrieve the position of a window
	DOLL_FUNC Void DOLL_API wnd_getPosition( OSWindow window, S32 &outPosX, S32 &outPosY );

	// Set the window to display again
	DOLL_FUNC Void DOLL_API wnd_setNeedsDisplay( OSWindow window );
	// Set a portion of the window to display again
	DOLL_FUNC Void DOLL_API wnd_setRectNeedsDisplay( OSWindow window, S32 clientLeft, S32 clientTop, S32 clientRight, S32 clientBottom );

	//------------------------------------------------------------------------//

	// Easier "get title"
	inline MutStr DOLL_API wnd_getTitle( OSWindow window )
	{
		MutStr temp;

		if( !window || !temp.reserveAndSetLen( wnd_getTitle( window, nullptr, 0 ) ) ) {
			return MutStr();
		}

		wnd_getTitle( window, temp.data(), temp.len() );
		return temp;
	}
	// Easier "get title"
	template< UPtr tBufferSize >
	inline char *DOLL_API wnd_getTitle( OSWindow window, char( &outBuffer )[ tBufferSize ] )
	{
		wnd_getTitle( window, outBuffer, tBufferSize );
		return &outBuffer[ 0 ];
	}

	// Templated "set data" - pointer
	template< typename tObject >
	inline Void DOLL_API wnd_setData( OSWindow window, tObject *pData )
	{
		wnd_setData( window, ( Void * )pData );
	}
	// Templated "set data" - reference
	template< typename tObject >
	inline Void DOLL_API wnd_setData( OSWindow window, tObject &Data )
	{
		wnd_setData( window, ( Void * )&Data );
	}

	// Templated "get data" - return pointer
	template< typename tObject >
	inline tObject *DOLL_API wnd_getData( OSWindow window )
	{
		return ( tObject * )wnd_getData( window );
	}
	// Templated "get data" - parameter pointer
	//
	// return: true if the data pointer isn't nullptr; false if it is nullptr
	template< typename tObject >
	inline Bool DOLL_API wnd_getData( OSWindow window, tObject *&pOutData )
	{
		pOutData = ( tObject * )wnd_getData( window );
		return pOutData != nullptr;
	}

}
