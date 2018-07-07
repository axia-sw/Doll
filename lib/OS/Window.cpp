#define DOLL_TRACE_FACILITY doll::kLog_OSWindow
#include "../BuildSettings.hpp"

#include "doll/OS/Window.hpp"

#include "doll/Core/Defs.hpp"
#include "Window_Delegate.hpp"

#ifndef DOLL_WINDOWS_ICON_RESOURCE
# define DOLL_WINDOWS_ICON_RESOURCE   1
#endif
#ifndef DOLL_WINDOWS_CURSOR_RESOURCE
# define DOLL_WINDOWS_CURSOR_RESOURCE 2
#endif

#if AX_OS_MACOSX
# include "macOS/Cocoa.h"
#endif

namespace doll
{

#ifndef __APPLE__
	struct SWndGlobInstance
	{
# if AX_OS_WINDOWS
		ATOM wndClass = ((ATOM)0);
# endif

		static SWndGlobInstance &get()
		{
			static SWndGlobInstance instance;
			return instance;
		}

	private:
		SWndGlobInstance();
		~SWndGlobInstance();
	};
	static TManager<SWndGlobInstance> WM;
#endif

	struct SWindow
	{
		SWndDelegate *pTopDelegate = nullptr;
		Void *        pUserData    = nullptr;
		MutStr        title;
	};

	static SWindow &W( OSWindow window )
	{
		static SWindow Dummy;

#if AX_OS_WINDOWS
		SWindow *p = ( SWindow * )GetWindowLongPtrW( ( HWND )window, 0 );
		if( p != nullptr ) {
			return *p;
		}
#elif AX_OS_MACOSX
		SWindow *p = ( SWindow * )macOS::wm::getData( ( macOS::wm::Window )window );
		if( p != nullptr ) {
			return *p;
		}
#endif

		return Dummy;
	}

#if AX_OS_WINDOWS
	namespace Windows
	{

		static Bool checkKey( int vk )
		{
			return U16( GetKeyState( vk ) ) > 1;
		}
		static U32 getMods()
		{
			U32 uMods = 0;
			
			uMods |= checkKey( VK_LSHIFT   ) ? kMF_LShift   : 0;
			uMods |= checkKey( VK_RSHIFT   ) ? kMF_RShift   : 0;
			uMods |= checkKey( VK_LMENU    ) ? kMF_LAlt     : 0;
			uMods |= checkKey( VK_RMENU    ) ? kMF_RAlt     : 0;
			uMods |= checkKey( VK_LCONTROL ) ? kMF_LControl : 0;
			uMods |= checkKey( VK_RCONTROL ) ? kMF_RControl : 0;
			uMods |= checkKey( VK_LBUTTON  ) ? kMF_Mouse1   : 0;
			uMods |= checkKey( VK_RBUTTON  ) ? kMF_Mouse2   : 0;
			uMods |= checkKey( VK_MBUTTON  ) ? kMF_Mouse3   : 0;
			uMods |= checkKey( VK_XBUTTON1 ) ? kMF_Mouse4   : 0;
			uMods |= checkKey( VK_XBUTTON2 ) ? kMF_Mouse5   : 0;

			return uMods;
		}
		static U32 getScancode( LPARAM lParm )
		{
			return ( ( lParm & 0x00FF0000 )>>16 ) + ( ( lParm & 0x01000000 )>>11 );
		}
		static Bool isRepeat( LPARAM lParm )
		{
			return ( lParm & ( 1<<30 ) ) != 0;
		}
		static U32 convertUTF16ToUTF32( const uint16( &src )[ 2 ] )
		{
			// check if the value is encoded as one UTF16 word
			if( *src < 0xD800 || *src > 0xDFFF ) {
				return U32( *src );
			}
		
			// check for an error (0xD800..0xDBFF)
			if( *src > 0xDBFF ) {
				// this is an invalid character; replace with U+FFFD
				return 0xFFFD;
			}

			// if no character follows, or is out of range, then this is an invalid encoding
			if( *( src + 1 ) < 0xDC00 || *( src + 1 ) > 0xDFFF ) {
				// replace with U+FFFD
				return 0xFFFD;
			}

			// encode
			return 0x10000 + ( ( ( ( *src ) & 0x3FF ) << 10 ) | ( ( *( src + 1 ) ) & 0x3FF ) );
		}

		static LRESULT convertHitTest( EHitTest hitTest )
		{
			switch( hitTest )
			{
			case EHitTest::NotHandled:
			case EHitTest::Miss:
				return HTNOWHERE;

			case EHitTest::ClientArea:
				return HTCLIENT;
			case EHitTest::TitleBar:
				return HTCAPTION;
			case EHitTest::SystemMenu:
				return HTSYSMENU;

			case EHitTest::MinimizeButton:
				return HTMINBUTTON;
			case EHitTest::MaximizeButton:
				return HTMAXBUTTON;
			case EHitTest::CloseButton:
				return HTCLOSE;

			case EHitTest::TopSizer:
				return HTTOP;
			case EHitTest::TopRightSizer:
				return HTTOPRIGHT;
			case EHitTest::RightSizer:
				return HTRIGHT;
			case EHitTest::BottomRightSizer:
				return HTBOTTOMRIGHT;
			case EHitTest::BottomSizer:
				return HTBOTTOM;
			case EHitTest::BottomLeftSizer:
				return HTBOTTOMLEFT;
			case EHitTest::LeftSizer:
				return HTLEFT;
			case EHitTest::TopLeftSizer:
				return HTTOPLEFT;

			default:
				AX_ASSERT_MSG( false, "Invalid input" );
				break;
			}

			return HTNOWHERE;
		}

		LRESULT CALLBACK msgProc_f( HWND hWindow, UINT uMessage, WPARAM wParm, LPARAM lParm )
		{
#define DELEGATE__(Name_) window.pTopDelegate, &window.pTopDelegate->pfn##Name_, pWindow

			if( uMessage == WM_NCCREATE ) {
				SetWindowLongPtrW( hWindow, 0, ( LONG_PTR )( ( ( const CREATESTRUCTW * )lParm )->lpCreateParams ) );
				return DefWindowProcW( hWindow, uMessage, wParm, lParm );
			}

			OSWindow pWindow = OSWindow( hWindow );
			SWindow &window = W( pWindow);

			switch( uMessage )
			{
				// close
				case WM_CLOSE:
					if( !callDelegate( DELEGATE__(OnClose) ) ) {
						DestroyWindow( hWindow );
					}

					return 0;
				// closed
				case WM_DESTROY:
					callDelegate( DELEGATE__(OnClosed) );
					SetWindowLongPtrW( hWindow, 0, 0 );
					destroy( window );
					return 0;

				// minimize/maximize
				case WM_SYSCOMMAND:
					if( ( wParm & 0xFFF0 ) == SC_MINIMIZE ) {
						callDelegate( DELEGATE__(OnMinimize) );
					} else if( ( wParm & 0xFFF0 ) == SC_MAXIMIZE ) {
						callDelegate( DELEGATE__(OnMaximize) );
					}

					break;

# if 0 // is sending a notification on style change unnecessary? only results from SetWindowLong, might cause WM_SYSCOMMAND anyway
				case WM_STYLECHANGING:
					if( wParm == GWL_STYLE ) {
						const STYLESTRUCT *const pStyles = ( const STYLESTRUCT * )lParm;
						if( !pStyles ) {
							break;
						}

						// The flags that have changed between the new and old styles
						const DWORD dwDifference = pStyles->styleNew ^ pStyles->styleOld;
						// The flags that were added in the new style that weren't in the old style
						const DWORD dwAdded = dwDifference & pStyles->styleNew;
	
						// If we now have a minimize flag then we are about to be minimized
						if( dwAdded & WS_MINIMIZE ) {
							callDelegate( DELEGATE__(OnMinimize) );
							return 0;
						}
						
						// If we now have a maximize flag then we are about to be maximized
						if( dwAdded & WS_MAXIMIZE ) {
							callDelegate( DELEGATE__(OnMaximize) );
							return 0;
						}
					}

					break;
# endif

				// app activate/deactivate
				case WM_ACTIVATEAPP:
					if( wParm == TRUE ) {
						callDelegate( DELEGATE__(OnAppActivate) );
					} else {
						callDelegate( DELEGATE__(OnAppDeactivate) );
					}
					return 0;

				// accept/resign main
				case WM_ACTIVATE:
					if( wParm != WA_INACTIVE ) {
						callDelegate( DELEGATE__(OnAcceptMain) );
					} else {
						callDelegate( DELEGATE__(OnResignMain) );
					}
					return 0;

				// accept key
				case WM_SETFOCUS:
					callDelegate( DELEGATE__(OnAcceptKey) );
					return 0;
				// resign key
				case WM_KILLFOCUS:
					callDelegate( DELEGATE__(OnResignKey) );
					return 0;

				// enable/disable
				case WM_ENABLE:
					if( wParm == TRUE ) {
						callDelegate( DELEGATE__(OnEnabled) );
					} else {
						callDelegate( DELEGATE__(OnDisabled) );
					}
					return 0;

				// visible/invisible
				case WM_SHOWWINDOW:
					if( wParm == TRUE ) {
						callDelegate( DELEGATE__(OnVisible) );
					} else {
						callDelegate( DELEGATE__(OnInvisible) );
					}
					return 0;

				// move
				case WM_MOVE:
					{
						RECT Screen;

						if( !GetWindowRect( hWindow, &Screen ) ) {
							return 0;
						}

						callDelegate( DELEGATE__(OnMoved), Screen.left, Screen.top );
					}
					return 0;
				// size
				case WM_SIZE:
					{
						const U32 uResX = U32( LOWORD( lParm ) );
						const U32 uResY = U32( HIWORD( lParm ) );

						switch( wParm ) {
						case SIZE_RESTORED:
							callDelegate( DELEGATE__(OnSized), uResX, uResY );
							break;

						case SIZE_MINIMIZED:
							callDelegate( DELEGATE__(OnMinimized) );
							break;

						case SIZE_MAXIMIZED:
							callDelegate( DELEGATE__(OnSized), uResX, uResY );
							callDelegate( DELEGATE__(OnMaximized) );
							break;

						default:
							break;
						}
					}
					return 0;

				// moving
				case WM_MOVING:
					{
						RECT *const pArea = ( RECT * )lParm;
						if( !pArea ) {
							return FALSE;
						}

						S32 frameLeft = pArea->left;
						S32 frameTop = pArea->top;
						S32 frameRight = pArea->right;
						S32 frameBottom = pArea->bottom;

						if( !callDelegate( DELEGATE__(OnMoving), frameLeft, frameTop, frameRight, frameBottom ) ) {
							break;
						}

						pArea->left = frameLeft;
						pArea->top = frameTop;
						pArea->right = frameRight;
						pArea->bottom = frameBottom;
					}
					return TRUE;
				// sizing
				case WM_SIZING:
					{
						RECT *const pArea = ( RECT * )lParm;
						if( !pArea ) {
							return FALSE;
						}

						S32 resX = pArea->right - pArea->left;
						S32 resY = pArea->bottom - pArea->top;

						if( !callDelegate( DELEGATE__(OnSizing), resX, resY ) ) {
							break;
						}

						// handle x-resolution adjustment
						switch( wParm ) {
						case WMSZ_LEFT:
						case WMSZ_TOPLEFT:
						case WMSZ_BOTTOMLEFT:
							pArea->left = pArea->right - resX;
							break;

						case WMSZ_RIGHT:
						case WMSZ_TOPRIGHT:
						case WMSZ_BOTTOMRIGHT:
							pArea->right = pArea->left + resX;
							break;

						default:
							break;
						}

						// handle y-resolution adjustment
						switch( wParm ) {
						case WMSZ_TOP:
						case WMSZ_TOPLEFT:
						case WMSZ_TOPRIGHT:
							pArea->top = pArea->bottom - resY;
							break;

						case WMSZ_BOTTOM:
						case WMSZ_BOTTOMLEFT:
						case WMSZ_BOTTOMRIGHT:
							pArea->bottom = pArea->top + resY;
							break;

						default:
							break;
						}
					}
					return TRUE;
					
				// mouse press/release and movement
				case WM_LBUTTONDOWN:
				case WM_RBUTTONDOWN:
				case WM_MBUTTONDOWN:
				case WM_XBUTTONDOWN:
				case WM_LBUTTONUP:
				case WM_RBUTTONUP:
				case WM_MBUTTONUP:
				case WM_XBUTTONUP:
				case WM_MOUSEMOVE:
					{
						const U32 uMods = getMods();
						const S32 clientPosX = S32( short( LOWORD( lParm ) ) );
						const S32 clientPosY = S32( short( HIWORD( lParm ) ) );

						if( uMessage == WM_MOUSEMOVE ) {
							TRACKMOUSEEVENT Tracker;

							Tracker.cbSize = sizeof( Tracker );
							Tracker.dwFlags = TME_LEAVE;
							Tracker.hwndTrack = hWindow;
							Tracker.dwHoverTime = 0;

							TrackMouseEvent( &Tracker );

							callDelegate( DELEGATE__(OnMouseMove), clientPosX, clientPosY, uMods );
						} else {
							EMouse button = EMouse::Left;

							if( uMessage == WM_RBUTTONDOWN || uMessage == WM_RBUTTONUP ) {
								button = EMouse::Right;
							} else if( uMessage == WM_MBUTTONDOWN || uMessage == WM_MBUTTONUP ) {
								button = EMouse::Middle;
							} else if( uMessage == WM_XBUTTONDOWN || uMessage == WM_XBUTTONUP ) {
								button = EMouse::Thumb1;
								if( HIWORD( wParm ) > 1 ) {
									button = EMouse::Thumb2;
								}
							}

							if( uMessage ==  WM_LBUTTONDOWN || uMessage == WM_RBUTTONDOWN || uMessage == WM_MBUTTONDOWN || uMessage == WM_XBUTTONDOWN ) {
								callDelegate( DELEGATE__(OnMousePress), button, clientPosX, clientPosY, uMods );
							} else {
								callDelegate( DELEGATE__(OnMouseRelease), button, clientPosX, clientPosY, uMods );
							}
						}
					}
					return 0;

				// mouse wheel
				case WM_MOUSEWHEEL:
					{
						const U32 uMods      = getMods();
						const S32 clientPosX = S32( short( LOWORD( lParm ) ) );
						const S32 clientPosY = S32( short( HIWORD( lParm ) ) );
						const float fDelta   = float( short( HIWORD( wParm ) )/WHEEL_DELTA );

						callDelegate( DELEGATE__(OnMouseWheel), fDelta, clientPosX, clientPosY, uMods );
					}
					return 0;

				// mouse exit
				case WM_MOUSELEAVE:
					callDelegate( DELEGATE__(OnMouseExit), getMods() );
					return 0;

				// key press
				case WM_SYSKEYDOWN:
				case WM_KEYDOWN:
					{
						const U32 uMods      = getMods();
						const U32 uScancode  = getScancode( lParm );
						const EKey key       = EKey( uScancode );
						const Bool bIsRepeat = isRepeat( lParm );

						callDelegate( DELEGATE__(OnKeyPress), key, uMods, bIsRepeat );
					}
					return 0;

				// key release
				case WM_SYSKEYUP:
				case WM_KEYUP:
					{
						const U32 uMods     = getMods();
						const U32 uScancode = getScancode( lParm );
						const EKey key      = EKey( uScancode );

						callDelegate( DELEGATE__(OnKeyRelease), key, uMods );
					}
					return 0;

				// key char
				case WM_CHAR:
					{
						const uint16 utf16Char[ 2 ] = {
							uint16( ( wParm >>  0 ) & 0xFFFF ),
							uint16( ( wParm >> 16 ) & 0xFFFF )
						};

						const U32 utf32Char = convertUTF16ToUTF32( utf16Char );

						callDelegate( DELEGATE__(OnKeyChar), utf32Char );
					}
					return 0;
				case WM_UNICHAR:
					{
						if( wParm == UNICODE_NOCHAR ) {
							return TRUE;
						}

						const U32 utf32Char = U32( wParm );

						callDelegate( DELEGATE__(OnKeyChar), utf32Char );
					}
					return FALSE;

				// hit test
				case WM_NCHITTEST:
					{
						const S32 mouseScreenPosX = S32( short( LOWORD( lParm ) ) );
						const S32 mouseScreenPosY = S32( short( HIWORD( lParm ) ) );

						RECT area;
						if( !GetWindowRect( hWindow, &area ) ) {
							area.left = mouseScreenPosX;
							area.top = mouseScreenPosY;
							area.right = 0;
							area.bottom = 0;
						}

						const S32 mouseFramePosX = mouseScreenPosX - area.left;
						const S32 mouseFramePosY = mouseScreenPosY - area.top;

						EHitTest result = EHitTest::NotHandled;
						SWndDelegate *pTest = window.pTopDelegate;
						while( pTest != nullptr ) {
							SWndDelegate *const pCheck = pTest;
							pTest = pTest->pSuper;

							if( !pCheck->pfnOnHitTest ) {
								continue;
							}

							result = pCheck->pfnOnHitTest( pWindow, mouseScreenPosX, mouseScreenPosY, mouseFramePosX, mouseFramePosY );
							if( result == EHitTest::NotHandled ) {
								continue;
							}

							break;
						}

						if( result != EHitTest::NotHandled ) {
							return convertHitTest( result );
						}
					}
					break;

				// change title
				case WM_SETTEXT:
					{
						if( !window.title.tryAssign( MutStr::fromWStr( ( const wchar_t * )lParm ) ) ) {
							return FALSE;
						}

						Bool bHasChangeText = false;
						SWndDelegate *pTest = window.pTopDelegate;
						while( pTest != nullptr ) {
							SWndDelegate *const pCheck = pTest;
							pTest = pTest->pSuper;

							if( !pCheck->pfnOnChangeTitle ) {
								continue;
							}

							bHasChangeText = true;
							break;
						}

						if( !bHasChangeText ) {
							//break;
							return TRUE;
						}

						if( !callDelegate( DELEGATE__(OnChangeTitle), window.title ) ) {
							//break;
							return TRUE;
						}
					}
					return TRUE;
				// retrieve the title
				case WM_GETTEXT:
					{
						window.title.toWStr( ( wchar_t * )lParm, UPtr( wParm ) );
						return wcslen( ( const wchar_t * )lParm );
					}

				// erase background
				case WM_ERASEBKGND:
					return TRUE;
			}

			return DefWindowProcW( hWindow, uMessage, wParm, lParm );

#undef DELEGATE__
		}

	}
#endif

#ifndef __APPLE__
	SWndGlobInstance::SWndGlobInstance()
	{
# if AX_OS_WINDOWS
		WNDCLASSEXW wc;

		wc.cbSize        = sizeof( wc );
		wc.style         = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc   = &Windows::msgProc_f;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = sizeof( Void * );
		wc.hInstance     = GetModuleHandleW( NULL );
		wc.hIcon         = ( HICON )LoadImageW( wc.hInstance, MAKEINTRESOURCEW( DOLL_WINDOWS_ICON_RESOURCE ), IMAGE_ICON, 32, 32, LR_CREATEDIBSECTION );

		if( !( wc.hCursor = ( HCURSOR )LoadImageW( wc.hInstance, MAKEINTRESOURCEW( DOLL_WINDOWS_CURSOR_RESOURCE ), IMAGE_CURSOR, 0, 0, LR_CREATEDIBSECTION ) ) ) {
			wc.hCursor   = LoadCursor( NULL, IDC_ARROW );
		}

		wc.hbrBackground = GetSysColorBrush( COLOR_BTNFACE );
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = L"FloatingWindow";
		wc.hIconSm       = ( HICON )LoadImageW( wc.hInstance, MAKEINTRESOURCEW( DOLL_WINDOWS_ICON_RESOURCE ), IMAGE_ICON, 16, 16, LR_CREATEDIBSECTION );

		wndClass = RegisterClassExW( &wc );
		AX_EXPECT_NOT_NULL( wndClass );
# endif
	}
	SWndGlobInstance::~SWndGlobInstance()
	{
# if AX_OS_WINDOWS
		UnregisterClassW( ( LPCWSTR )( UPtr )wndClass, GetModuleHandleW( NULL ) );
# endif
	}
#endif // __APPLE__

	static SWndDelegate *duplicateDelegate_r( const SWndDelegate &Source, SWndDelegate *pTopDelegate = NULL )
	{
		SWndDelegate *pNewDelegate = new SWndDelegate();
		AX_EXPECT_NOT_NULL( pNewDelegate );

		memcpy( ( Void * )pNewDelegate, ( const Void * )&Source, sizeof( SWndDelegate ) );
		pNewDelegate->pSuper = Source.pSuper != NULL ? duplicateDelegate_r( *Source.pSuper, pTopDelegate ) : pTopDelegate;

		return pNewDelegate;
	}
	static Bool compareDelegates( const SWndDelegate &first, const SWndDelegate &second )
	{
#if 0
		const UPtr *const pFirst = ( const UPtr * )&first;
		const UPtr *const pSecond = ( const UPtr * )&second;

		static const UPtr uStart = 1;
		static const UPtr uEnd = sizeof( SWndDelegate )/sizeof( Void * );

		for( UPtr i = uStart; i < uEnd; ++i ) {
			if( pFirst[ i ] != pSecond[ i ] ) {
				return false;
			}
		}

		return true;
#else
		return memcmp( ( const void * )&first, ( const void * )&second, sizeof( SWndDelegate ) ) == 0;
#endif
	}

#if AX_OS_WINDOWS
	static Void getStyleAndExstyle( U32 uStyleFlags, DWORD &OutStyle, DWORD &OutExstyle )
	{
		OutStyle = WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
		OutExstyle = 0;

		if( ~uStyleFlags & kWSF_NoCloseButton ) {
			OutStyle |= WS_SYSMENU;
		}
		if( ~uStyleFlags & kWSF_NoMinimizeButton ) {
			OutStyle |= WS_MINIMIZEBOX;
		}
		if( ~uStyleFlags & kWSF_NoMaximizeButton ) {
			OutStyle |= WS_MAXIMIZEBOX;
		}
		if( ~uStyleFlags & kWSF_NoResize ) {
			OutStyle |= WS_THICKFRAME;
		}
		if( uStyleFlags & kWSF_ThinCaption ) {
			OutExstyle |= WS_EX_TOOLWINDOW;
		}
		if( uStyleFlags & kWSF_NoOSParts ) {
			OutStyle |= WS_POPUP;
		} else {
			OutStyle |= WS_CAPTION;
		}
		if( uStyleFlags & kWSF_TopMost ) {
			OutExstyle |= WS_EX_TOPMOST;
		}
		if( uStyleFlags & kWSF_Translucent ) {
			OutExstyle |= WS_EX_LAYERED | WS_EX_COMPOSITED;
		}
	}
#endif

	DOLL_FUNC OSWindow DOLL_API wnd_open( const SWndCreateInfo &info )
	{
		SWindow *pWindow = new SWindow();
		AX_EXPECT_NOT_NULL( pWindow );

		if( info.pDelegate != nullptr ) {
			pWindow->pTopDelegate = duplicateDelegate_r( *info.pDelegate );
		}
		pWindow->pUserData = info.pData;

#if AX_OS_WINDOWS
		DWORD style;
		DWORD exstyle;

		getStyleAndExstyle( info.uStyleFlags, style, exstyle );

		RECT area = {
			LONG( info.shape.x1 ),
			LONG( info.shape.y1 ),
			LONG( info.shape.x2 ),
			LONG( info.shape.y2 )
		};
		AX_EXPECT( AdjustWindowRectEx( &area, style, FALSE, exstyle ) );

		wchar_t wszBuf[ 256 ];
		HWND hWindow = CreateWindowExW
		(
			exstyle,
			( LPCWSTR )( UPtr )WM->wndClass,
			info.title.toWStr( wszBuf ),
			style,
			area.left,
			area.top,
			area.right - area.left,
			area.bottom - area.top,
			NULL,
			NULL,
			GetModuleHandleW( NULL ),
			( Void * )pWindow
		);
		if( !hWindow ) {
			return NULL;
		}

		return OSWindow( hWindow );
#elif AX_OS_MACOSX
		return OSWindow( macOS::wm::open( info, ( void * )pWindow ) );
#endif
	}
	DOLL_FUNC Void DOLL_API wnd_close( OSWindow window )
	{
		AX_ASSERT_NOT_NULL(window);

#if AX_OS_WINDOWS
		DestroyWindow( HWND(window) );
#elif AX_OS_MACOSX
		macOS::wm::close( macOS::wm::Window(window) );
#endif
	}
	DOLL_FUNC Void DOLL_API wnd_minimize( OSWindow window )
	{
		AX_ASSERT_NOT_NULL(window);

#if AX_OS_WINDOWS
		ShowWindow( HWND(window), SW_MINIMIZE );
#elif AX_OS_MACOSX
		macOS::wm::minimize( macOS::wm::Window(window) );
#endif
	}
	DOLL_FUNC Void DOLL_API wnd_maximize( OSWindow window )
	{
		AX_ASSERT_NOT_NULL(window);

#if AX_OS_WINDOWS
		ShowWindow( HWND(window), SW_MAXIMIZE );
#elif AX_OS_MACOSX
		macOS::wm::maximize( macOS::wm::Window(window) );
#endif
	}

	DOLL_FUNC Void DOLL_API wnd_addDelegate( OSWindow window, const SWndDelegate &wndDelegate )
	{
		AX_ASSERT_NOT_NULL(window);

		SWindow &Self = W(window);

		Self.pTopDelegate = duplicateDelegate_r( wndDelegate, Self.pTopDelegate );
		AX_ASSERT_NOT_NULL( Self.pTopDelegate );
	}
	DOLL_FUNC Void DOLL_API wnd_removeDelegate( OSWindow window, const SWndDelegate &wndDelegate )
	{
		AX_ASSERT_NOT_NULL(window);

		SWindow &Self = W(window);

		SWndDelegate **ppSet = &Self.pTopDelegate;
		for( SWndDelegate *pTest = Self.pTopDelegate; pTest != NULL; pTest = pTest->pSuper ) {
			if( !compareDelegates( *pTest, wndDelegate ) ) {
				ppSet = &pTest->pSuper;
				continue;
			}

			*ppSet = pTest->pSuper;
			delete pTest;
			break;
		}
	}

	DOLL_FUNC Void DOLL_API wnd_setTitle( OSWindow window, Str title )
	{
		AX_ASSERT_NOT_NULL(window);

#if AX_OS_WINDOWS
		wchar_t wszBuf[ 256 ];
		SetWindowTextW( HWND(window), title.toWStr( wszBuf ) );
#endif
	}
	DOLL_FUNC UPtr DOLL_API wnd_getTitle( OSWindow window, char *pszOutTitleUTF8, UPtr cMaxOutBytes )
	{
		AX_ASSERT_NOT_NULL(window);

		SWindow &IntWindow = W(window);
		if( !pszOutTitleUTF8 || !cMaxOutBytes ) {
			return IntWindow.title.num();
		}

		return
			axstr_cpyn
			(
				pszOutTitleUTF8,
				cMaxOutBytes,
				IntWindow.title.get(),
				IntWindow.title.len()
			);
	}

	DOLL_FUNC Void DOLL_API wnd_setData( OSWindow window, Void *pData )
	{
		AX_ASSERT_NOT_NULL(window);
		W(window).pUserData = pData;
	}
	DOLL_FUNC Void *DOLL_API wnd_getData( OSWindow window )
	{
		AX_ASSERT_NOT_NULL(window);
		return W(window).pUserData;
	}

	DOLL_FUNC Void DOLL_API wnd_performClose( OSWindow window )
	{
		AX_ASSERT_NOT_NULL( window );

#if AX_OS_WINDOWS
		SendMessageW( HWND(window), WM_SYSCOMMAND, SC_CLOSE, 0 );
#elif AX_OS_MACOSX
		macOS::wm::performClose( macOS::wm::Window(window) );
#endif
	}
	DOLL_FUNC Void DOLL_API wnd_performMinimize( OSWindow window )
	{
		AX_ASSERT_NOT_NULL( window );

#if AX_OS_WINDOWS
		SendMessageW( HWND(window), WM_SYSCOMMAND, SC_MINIMIZE, 0 );
#elif AX_OS_MACOSX
		macOS::wm::performMinimize( macOS::wm::Window(window) );
#endif
	}
	DOLL_FUNC Void DOLL_API wnd_performMaximize( OSWindow window )
	{
		AX_ASSERT_NOT_NULL( window );

#if AX_OS_WINDOWS
		SendMessageW( HWND(window), WM_SYSCOMMAND, SC_MAXIMIZE, 0 );
#elif AX_OS_MACOSX
		macOS::wm::performMaximize( macOS::wm::Window(window) );
#endif
	}

	DOLL_FUNC Void DOLL_API wnd_setVisible( OSWindow window, Bool bIsVisible )
	{
		AX_ASSERT_NOT_NULL( window );

#if AX_OS_WINDOWS
		ShowWindow( HWND(window), bIsVisible ? SW_SHOW : SW_HIDE );
		if( bIsVisible ) {
			UpdateWindow( HWND(window) );
		}
#elif AX_OS_MACOSX
		macOS::wm::setVisible( macOS::wm::Window(window), bIsVisible );
#endif
	}
	DOLL_FUNC Bool DOLL_API wnd_isVisible( OSWindow window )
	{
		AX_ASSERT_NOT_NULL( window );

#if AX_OS_WINDOWS
		return IsWindowVisible( HWND(window) ) != FALSE;
#elif AX_OS_MACOSX
		return macOS::wm::isVisible( macOS::wm::Window(window) );
#endif
	}

	DOLL_FUNC Void DOLL_API wnd_setEnabled( OSWindow window, Bool bIsEnabled )
	{
		AX_ASSERT_NOT_NULL( window );

#if AX_OS_WINDOWS
		EnableWindow( HWND(window), bIsEnabled ? TRUE : FALSE );
#elif AX_OS_MACOSX
		macOS::wm::setEnabled( macOS::wm::Window(window), bIsEnabled );
#endif
	}
	DOLL_FUNC Bool DOLL_API wnd_isEnabled( OSWindow window )
	{
		AX_ASSERT_NOT_NULL( window );

#if AX_OS_WINDOWS
		return IsWindowEnabled( HWND(window) ) != FALSE;
#elif AX_OS_MACOSX
		return macOS::wm::isEnabled( macOS::wm::Window(window) );
#endif
	}

	DOLL_FUNC Void DOLL_API wnd_resize( OSWindow window, U32 resX, U32 resY )
	{
		AX_ASSERT_NOT_NULL(window);

#if AX_OS_WINDOWS
		const DWORD style   = GetWindowLongW( HWND(window), GWL_STYLE );
		const DWORD exstyle = GetWindowLongW( HWND(window), GWL_EXSTYLE );
		const BOOL bHasMenu = GetMenu( HWND(window) ) != NULL;

		RECT adjust = { 0, 0, LONG( resX ), LONG( resY ) };

		if( !AX_VERIFY( AdjustWindowRectEx( &adjust, style, bHasMenu, exstyle ) ) ) {
			return;
		}

		const U32 clientResX = adjust.right - adjust.left;
		const U32 clientResY = adjust.bottom - adjust.top;

		SetWindowPos( HWND(window), NULL, 0, 0, clientResX, clientResY, SWP_NOMOVE|SWP_NOZORDER );
#elif AX_OS_MACOSX
		macOS::wm::resize( macOS::wm::Window(window), resX, resY );
#endif
	}
	DOLL_FUNC Void DOLL_API wnd_resizeFrame( OSWindow window, U32 resX, U32 resY )
	{
		AX_ASSERT_NOT_NULL(window);

#if AX_OS_WINDOWS
		SetWindowPos( HWND(window), NULL, 0, 0, resX, resY, SWP_NOMOVE|SWP_NOZORDER );
#elif AX_OS_MACOSX
		macOS::wm::resizeFrame( macOS::wm::Window(window), resX, resY );
#endif
	}

	DOLL_FUNC Void DOLL_API wnd_position( OSWindow window, S32 posX, S32 posY )
	{
		AX_ASSERT_NOT_NULL(window);

#if AX_OS_WINDOWS
		SetWindowPos( HWND(window), NULL, posX, posY, 0, 0, SWP_NOSIZE|SWP_NOZORDER );
#elif AX_OS_MACOSX
		macOS::wm::position( macOS::wm::Window(window), posX, posY );
#endif
	}

	DOLL_FUNC Void DOLL_API wnd_getSize( OSWindow window, U32 &outResX, U32 &outResY )
	{
		AX_ASSERT_NOT_NULL(window);

#if AX_OS_WINDOWS
		RECT client;

		if( !AX_VERIFY( GetClientRect( HWND(window), &client ) ) ) {
			outResX = 0;
			outResY = 0;

			return;
		}

		outResX = client.right;
		outResY = client.bottom;
#elif AX_OS_MACOSX
		macOS::wm::getSize( macOS::wm::Window(window), outResX, outResY );
#endif
	}
	DOLL_FUNC Void DOLL_API wnd_getFrameSize( OSWindow window, U32 &outResX, U32 &outResY )
	{
		AX_ASSERT_NOT_NULL(window);

#if AX_OS_WINDOWS
		RECT screen;

		if( !AX_VERIFY( GetWindowRect( HWND(window), &screen ) ) ) {
			outResX = 0;
			outResY = 0;

			return;
		}

		outResX = screen.right - screen.left;
		outResY = screen.bottom - screen.top;
#elif AX_OS_MACOSX
		macOS::wm::getFrameSize( macOS::wm::Window(window), outResX, outResY );
#endif
	}

	DOLL_FUNC Void DOLL_API wnd_getPosition( OSWindow window, S32 &outPosX, S32 &outPosY )
	{
		AX_ASSERT_NOT_NULL(window);

#if AX_OS_WINDOWS
		RECT screen;

		if( !AX_VERIFY( GetWindowRect( HWND(window), &screen ) ) ) {
			outPosX = 0;
			outPosY = 0;

			return;
		}

		outPosX = screen.left;
		outPosY = screen.top;
#elif AX_OS_MACOSX
		macOS::wm::getPosition( macOS::wm::Window(window), outPosX, outPosY );
#endif
	}

	DOLL_FUNC Void DOLL_API wnd_setNeedsDisplay( OSWindow window )
	{
		AX_ASSERT_NOT_NULL(window);

#if AX_OS_WINDOWS
		InvalidateRect( HWND(window), NULL, TRUE );
#elif AX_OS_MACOSX
		macOS::wm::setNeedsDisplay( macOS::wm::Window(window) );
#endif
	}
	DOLL_FUNC Void DOLL_API wnd_setRectNeedsDisplay( OSWindow window, S32 clientLeft, S32 clientTop, S32 clientRight, S32 clientBottom )
	{
		AX_ASSERT_NOT_NULL(window);

#if AX_OS_WINDOWS
		const RECT area = { clientLeft, clientTop, clientRight, clientBottom };
		InvalidateRect( HWND(window), &area, TRUE );
#elif AX_OS_MACOSX
		macOS::wm::setRectNeedsDisplay( macOS::wm::Window(window), clientLeft, clientTop, clientRight, clientBottom );
#endif
	}

}
