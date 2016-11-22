#include "doll/Front/Input.hpp"
#include "doll/Front/Frontend.hpp"

#include "doll/Core/Logger.hpp"
#include "doll/OS/App.hpp"
#include "doll/OS/Window.hpp"

namespace doll
{

	struct SWndInputState
	{
		static const UPtr kNumFieldBits = sizeof(UPtr)*8;
		static const UPtr kNumKeyFields = 0x100/kNumFieldBits;

		static const UPtr kNumMouseBtns = 8;

		UPtr   keys[ kNumKeyFields ];
		UPtr   mouse;

		MutStr entry;
		Bool   entryEnabled;

		S32    mouseX, mouseY;
		F32    mouseZ;
		Bool   mouseInWindow;

		S32    mouseDeltaX, mouseDeltaY;
		F32    mouseDeltaZ;
		Bool   mouseDeltaDirty;

		U8     keyHits[ 0x100 ];
		U8     keyRels[ 0x100 ];
		U8     mouseHits[ kNumMouseBtns ];
		U8     mouseRels[ kNumMouseBtns ];

		U8     keyPressed;

		U32              cActions;
		SCoreInputAction actions[ kMaxInputActions ];

		inline SWndInputState()
		: mouse(0)
		, entry()
		, entryEnabled( false )
		, mouseX( 0 )
		, mouseY( 0 )
		, mouseZ( 0.0f )
		, mouseInWindow( false )
		, mouseDeltaX( 0 )
		, mouseDeltaY( 0 )
		, mouseDeltaZ( 0.0f )
		, mouseDeltaDirty( true )
		, keyPressed( 0 )
		, cActions( 0 )
		{
			memset( &keys[0], 0, sizeof(keys) );
			memset( &keyHits[0], 0, sizeof(keyHits) );
			memset( &keyRels[0], 0, sizeof(keyRels) );
			memset( &mouseHits[0], 0, sizeof(mouseHits) );
			memset( &mouseRels[0], 0, sizeof(mouseRels) );
		}

		inline Void setKey( U8 index )
		{
			const U8 i = index/kNumFieldBits;
			const U8 j = index%kNumFieldBits;

			keys[i] |= UPtr(1)<<j;
		}
		inline Void clearKey( U8 index )
		{
			const U8 i = index/kNumFieldBits;
			const U8 j = index%kNumFieldBits;

			keys[i] &= ~(UPtr(1)<<j);
		}
		inline Bool testKey( U8 index ) const
		{
			const U8 i = index/kNumFieldBits;
			const U8 j = index%kNumFieldBits;

			return ( keys[i] & (UPtr(1)<<j) ) != 0;
		}

		inline Void hitKey( U8 index )
		{
			if( keyHits[index] < 0xFF ) {
				++keyHits[index];
			}
		}
		inline Void relKey( U8 index )
		{
			if( keyRels[ index ] < 0xFF ) {
				++keyRels[index];
			}
		}

		inline Void hitMouse( U8 index )
		{
			if( index >= kNumMouseBtns ) {
				return;
			}

			if( mouseHits[ index ] < 0xFF ) {
				++mouseHits[index];
			}
		}
		inline Void relMouse( U8 index )
		{
			if( index >= kNumMouseBtns ) {
				return;
			}

			if( mouseRels[ index ] < 0xFF ) {
				++mouseRels[index];
			}
		}

		inline Void setMouse( U8 index )
		{
			mouse |= UPtr(1)<<index;
		}
		inline Void clearMouse( U8 index )
		{
			mouse &= ~(UPtr(1)<<index);
		}
		inline Bool testMouse( U8 index ) const
		{
			return ( mouse & (UPtr(1)<<index) ) != 0;
		}

		inline Bool setEntry( const Str &newEntry )
		{
			return entry.tryAssign( newEntry );
		}
		inline Void entryBackspace()
		{
			U8 x;
			do {
				x = entry.lastByte();
				entry.dropMe();
			} while( ( x & 0x80 ) != 0 );
		}
		inline Bool handleEntryChar( U32 utf32Char )
		{
			if( !utf32Char ) {
				return true;
			}

			if( utf32Char == '\b' ) {
				entryBackspace();
				return true;
			}

			axstr_utf8_t buf[ 5 ] = { 0, 0, 0, 0, 0 };
			const axstr_utf32_t src[ 2 ] = { utf32Char, 0 };

			axstr_utf32_to_utf8( buf, sizeof( buf ), src );
			return entry.tryAppend( Str( ( char * )buf ) );
		}

		inline Void handleMouseMove( S32 clientPosX, S32 clientPosY )
		{
			if( mouseDeltaDirty ) {
				mouseDeltaDirty = false;
			} else {
				mouseDeltaX += clientPosX - mouseX;
				mouseDeltaY += clientPosY - mouseY;
			}

			mouseX = clientPosX;
			mouseY = clientPosY;
		}

		SCoreInputAction *findAction( ECoreInputAction cmd )
		{
			for( U32 i = 0; i < cActions; ++i ) {
				if( actions[ i ].command == cmd ) {
					return &actions[ i ];
				}
			}

			return nullptr;
		}
		const SCoreInputAction *findAction( EKey key, U32 uMods ) const
		{
			for( U32 i = 0; i < cActions; ++i ) {
				const SCoreInputAction &act = actions[ i ];
				if( act.key == key && act.uMods == uMods ) {
					return &act;
				}
			}

			return nullptr;
		}

		Bool addAction( const SCoreInputAction &act )
		{
			// Don't add exact duplicates
			for( U32 i = 0; i < cActions; ++i ) {
				const SCoreInputAction &cmpAct = actions[ i ];
				if( act.command == cmpAct.command && act.key == cmpAct.key && act.uMods == cmpAct.uMods ) {
					return true;
				}
			}

			// Check if we're at the limit
			if( cActions >= kMaxInputActions ) {
				return false;
			}

			actions[ cActions++ ] = act;
			return true;
		}
		Bool setAction( const SCoreInputAction &act )
		{
			SCoreInputAction *pAct = findAction( act.command );
			if( !pAct ) {
				return addAction( act );
			}

			*pAct = act;
			return true;
		}
		Void removeAction( const SCoreInputAction &act )
		{
			for( U32 i = 0; i < cActions; ++i ) {
				SCoreInputAction &cmpAct = actions[ i ];
				if( act.command != cmpAct.command || act.key != cmpAct.key || act.uMods != cmpAct.uMods ) {
					continue;
				}

				if( i < --cActions ) {
					cmpAct = actions[ cActions ];
				}
			}
		}
		Bool hasAction( const SCoreInputAction &act ) const
		{
			for( U32 i = 0; i < cActions; ++i ) {
				const SCoreInputAction &cmpAct = actions[ i ];
				if( act.command == cmpAct.command && act.key == cmpAct.key && act.uMods == cmpAct.uMods ) {
					return true;
				}
			}

			return false;
		}

		Void issueCommand( ECoreInputAction cmd )
		{
			switch( cmd ) {
			case ECoreInputAction::Quit:
				g_DebugLog += "'Quit' action issued. (Probably pressed the 'Esc' key.)";
				os_submitQuitEvent();
				return;

			case ECoreInputAction::ToggleFullscreen:
				DOLL_WARNING_LOG += "'ToggleFullscreen' action not implemented";
				return;

			case ECoreInputAction::CaptureScreenshot:
				DOLL_WARNING_LOG += "'CaptureScreenshot' action not implemented";
				return;

			case ECoreInputAction::OpenDevcon:
				DOLL_WARNING_LOG += "'OpenDevcon' action not implemented";
				return;
			case ECoreInputAction::ToggleGraphs:
				DOLL_WARNING_LOG += "'ToggleGraphs' action not implemented";
				return;
			}

			char szBuf[128];
			DOLL_ERROR_LOG += (axspf(szBuf,"Unknown action '%u'",U32(cmd)),szBuf);
		}
		Bool checkCommand( EKey key, U32 uMods )
		{
			const SCoreInputAction *const pAct = findAction( key, uMods );
			if( !pAct ) {
				return false;
			}

			issueCommand( pAct->command );
			return true;
		}
	};

	static SWndInputState *getWndInput( OSWindow wnd )
	{
		static SWndInputState mainWndInputState;

		if( ~g_core.input.uFlags & kCoreInF_Enabled ) {
			return nullptr;
		}

#if DOLL__USE_GLFW
		if( !wnd ) {
			return &mainWndInputState;
		}
#else
		if( !wnd || wnd == g_core.view.window ) {
			return &mainWndInputState;
		}
#endif

		return nullptr;
	}

	// ------------------------------------------------------------------ //

	DOLL_FUNC EWndReply in_onAcceptKey_f( OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return EWndReply::NotHandled;
		}

		// FIXME: Load all current key state up here (at the time of the
		//        message's delivery)

		return EWndReply::Handled;
	}
	DOLL_FUNC EWndReply in_onResignKey_f( OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return EWndReply::NotHandled;
		}

		// FIXME: This should probably do something useful or be removed

		return EWndReply::Handled;
	}

	DOLL_FUNC EWndReply in_onKeyPress_f( OSWindow wnd, EKey key, U32 uModFlags, Bool isRepeat )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return EWndReply::NotHandled;
		}

		if( !isRepeat ) {
			if( p->checkCommand( key, uModFlags ) ) {
				return EWndReply::Handled;
			}

			p->hitKey( (U8)key );
			p->keyPressed = (U8)key;
		}

		p->setKey( (U8)key );
		return EWndReply::Handled;
	}
	DOLL_FUNC EWndReply in_onKeyRelease_f( OSWindow wnd, EKey key, U32 uModFlags )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return EWndReply::NotHandled;
		}

		( Void )uModFlags;

		p->relKey( (U8)key );
		if( p->keyPressed == ( U8 )key ) {
			p->keyPressed = 0;
		}

		p->clearKey( (U8)key );
		return EWndReply::Handled;
	}
	DOLL_FUNC EWndReply in_onKeyChar_f( OSWindow wnd, U32 utf32Char )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return EWndReply::NotHandled;
		}

		// FIXME: Check return value and do something useful (like notify the user)
		p->handleEntryChar( utf32Char );

		return EWndReply::Handled;
	}

	DOLL_FUNC EWndReply in_onMousePress_f( OSWindow wnd, EMouse button, S32 clientPosX, S32 clientPosY, U32 uModFlags )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p || button == EMouse::None ) {
			return EWndReply::NotHandled;
		}

		( Void )uModFlags;

		p->setMouse( U8(button) - 1 );
		p->handleMouseMove( clientPosX, clientPosY );
		p->mouseInWindow = true;

		p->hitMouse( U8(button) - 1 );

		return EWndReply::Handled;
	}
	DOLL_FUNC EWndReply in_onMouseRelease_f( OSWindow wnd, EMouse button, S32 clientPosX, S32 clientPosY, U32 uModFlags )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p || button == EMouse::None ) {
			return EWndReply::NotHandled;
		}

		( Void )uModFlags;

		p->clearMouse( U8(button) - 1 );
		p->handleMouseMove( clientPosX, clientPosY );
		p->mouseInWindow = true;

		p->relMouse( U8(button) - 1 );

		return EWndReply::Handled;
	}
	DOLL_FUNC EWndReply in_onMouseWheel_f( OSWindow wnd, F32 fDelta, S32 clientPosX, S32 clientPosY, U32 uModFlags )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return EWndReply::NotHandled;
		}

		( Void )uModFlags;

		p->mouseZ += fDelta;
		p->mouseDeltaZ += fDelta;
		p->handleMouseMove( clientPosX, clientPosY );
		p->mouseInWindow = true;

		return EWndReply::Handled;
	}
	DOLL_FUNC EWndReply in_onMouseMove_f( OSWindow wnd, S32 clientPosX, S32 clientPosY, U32 uModFlags )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return EWndReply::NotHandled;
		}

		( void )uModFlags;

		p->handleMouseMove( clientPosX, clientPosY );
		p->mouseInWindow = true;

		return EWndReply::Handled;
	}
	DOLL_FUNC EWndReply in_onMouseExit_f( OSWindow wnd, U32 uModFlags )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return EWndReply::NotHandled;
		}

		( Void )uModFlags;

		p->mouseInWindow = false;

		return EWndReply::Handled;
	}

	// ------------------------------------------------------------------ //

	DOLL_FUNC Void DOLL_API in_enableAll()
	{
		g_core.input.uFlags |= kCoreInF_Enabled;
	}
	DOLL_FUNC Void DOLL_API in_disableAll()
	{
		g_core.input.uFlags &= ~kCoreInF_Enabled;
	}
	DOLL_FUNC Bool DOLL_API in_allEnabled()
	{
		return ( g_core.input.uFlags & kCoreInF_Enabled ) != 0;
	}

	DOLL_FUNC Bool DOLL_API in_keyState( EKey key, OSWindow wnd )
	{
		SWndInputState const *const p = getWndInput( wnd );
		if( !p ) {
			return false;
		}

		return p->testKey( (U8)key );
	}
	DOLL_FUNC Bool DOLL_API in_keyHit( EKey key, OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return false;
		}

		if( p->keyHits[ ( U8 )key ] > 0 ) {
			--p->keyHits[ (U8)key ];
			return true;
		}

		return false;
	}
	DOLL_FUNC Bool DOLL_API in_keyReleased( EKey key, OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return false;
		}

		if( p->keyRels[ ( U8 )key ] > 0 ) {
			--p->keyRels[ (U8)key ];
			return true;
		}

		return false;
	}
	DOLL_FUNC U8 DOLL_API in_scancode( OSWindow wnd )
	{
		SWndInputState const *const p = getWndInput( wnd );
		if( !p ) {
			return 0;
		}

		return p->keyPressed;
	}
	DOLL_FUNC Bool DOLL_API in_mouseState( EMouse button, OSWindow wnd )
	{
		SWndInputState const *const p = getWndInput( wnd );
		if( !p || button == EMouse::None ) {
			return false;
		}

		return p->testMouse( U8(button) - 1 );
	}
	DOLL_FUNC Bool DOLL_API in_mouseHit( EMouse button, OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p || button == EMouse::None ) {
			return false;
		}

		const U8 index = U8( button ) - 1;

		if( index >= SWndInputState::kNumMouseBtns ) {
			return false;
		}

		if( p->mouseHits[ index ] > 0 ) {
			--p->mouseHits[ index ];
			return true;
		}

		return false;
	}
	DOLL_FUNC Bool DOLL_API in_mouseReleased( EMouse button, OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p || button == EMouse::None ) {
			return false;
		}

		const U8 index = U8( button ) - 1;

		if( index >= SWndInputState::kNumMouseBtns ) {
			return false;
		}

		if( p->mouseRels[ index ] > 0 ) {
			--p->mouseRels[ index ];
			return true;
		}

		return false;
	}
	DOLL_FUNC S32 DOLL_API in_mouseX( OSWindow wnd )
	{
		SWndInputState const *const p = getWndInput( wnd );
		if( !p ) {
			return 0;
		}

		return p->mouseX;
	}
	DOLL_FUNC S32 DOLL_API in_mouseY( OSWindow wnd )
	{
		SWndInputState const *const p = getWndInput( wnd );
		if( !p ) {
			return 0;
		}

		return p->mouseY;
	}

	DOLL_FUNC F32 DOLL_API in_mouseZ( OSWindow wnd )
	{
		SWndInputState const *const p = getWndInput( wnd );
		if( !p ) {
			return 0.0f;
		}

		return p->mouseZ;
	}
	DOLL_FUNC Void DOLL_API in_setMouseZ( F32 z, OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return;
		}

		p->mouseZ = z;
	}

	DOLL_FUNC S32 DOLL_API in_mouseMoveX( OSWindow wnd )
	{
		SWndInputState const *const p = getWndInput( wnd );
		if( !p ) {
			return 0;
		}

		return p->mouseDeltaX;
	}
	DOLL_FUNC S32 DOLL_API in_mouseMoveY( OSWindow wnd )
	{
		SWndInputState const *const p = getWndInput( wnd );
		if( !p ) {
			return 0;
		}

		return p->mouseDeltaY;
	}
	DOLL_FUNC F32 DOLL_API in_mouseMoveZ( OSWindow wnd )
	{
		SWndInputState const *const p = getWndInput( wnd );
		if( !p ) {
			return 0.0f;
		}

		return p->mouseDeltaZ;
	}
	DOLL_FUNC Void DOLL_API in_resetMouseMove( OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return;
		}

		p->mouseDeltaX = 0;
		p->mouseDeltaY = 0;
		p->mouseDeltaZ = 0;
	}

	DOLL_FUNC Void DOLL_API in_enableEntry( OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return;
		}

		p->entryEnabled = true;
	}
	DOLL_FUNC Void DOLL_API in_disableEntry( OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return;
		}

		p->entryEnabled = false;
	}
	DOLL_FUNC Bool DOLL_API in_isEntryEnabled( OSWindow wnd )
	{
		SWndInputState const *const p = getWndInput( wnd );
		if( !p ) {
			return false;
		}

		return p->entryEnabled;
	}
	DOLL_FUNC Bool DOLL_API in_setEntry( Str entry, OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return false;
		}

		return p->entry.tryAssign( entry );
	}
	DOLL_FUNC Bool DOLL_API in_getEntry( Str &dst, OSWindow wnd )
	{
		SWndInputState const *const p = getWndInput( wnd );
		if( !p ) {
			dst = Str();
			return false;
		}

		dst = p->entry;
		return true;
	}

	DOLL_FUNC Bool DOLL_API in_addAction( const SCoreInputAction &act, OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return false;
		}

		return p->addAction( act );
	}
	DOLL_FUNC Bool DOLL_API in_setAction( const SCoreInputAction &act, OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return false;
		}

		return p->setAction( act );
	}
	DOLL_FUNC Void DOLL_API in_removeAction( const SCoreInputAction &act, OSWindow wnd )
	{
		SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return;
		}

		p->removeAction( act );
	}
	DOLL_FUNC Bool DOLL_API in_hasAction( const SCoreInputAction &act, OSWindow wnd )
	{
		const SWndInputState *const p = getWndInput( wnd );
		if( !p ) {
			return false;
		}

		return p->hasAction( act );
	}

	static const SCoreInputAction g_escapeAct = { EKey::Esc, 0, ECoreInputAction::Quit };
	DOLL_FUNC Void DOLL_API in_enableEscapeKey( OSWindow wnd )
	{
		in_setAction( g_escapeAct, wnd );
	}
	DOLL_FUNC Void DOLL_API in_disableEscapeKey( OSWindow wnd )
	{
		in_removeAction( g_escapeAct, wnd );
	}
	DOLL_FUNC Bool DOLL_API in_escapeKeyEnabled( OSWindow wnd )
	{
		return in_hasAction( g_escapeAct, wnd );
	}

	static const SCoreInputAction g_togfullAct1 = { EKey::Enter, kMF_Shift, ECoreInputAction::ToggleFullscreen };
	static const SCoreInputAction g_togfullAct2 = { EKey::F11, 0, ECoreInputAction::ToggleFullscreen };
	DOLL_FUNC Void DOLL_API in_enableToggleFullscreen( OSWindow wnd )
	{
		in_setAction( g_togfullAct1, wnd );
		in_setAction( g_togfullAct2, wnd );
	}
	DOLL_FUNC Void DOLL_API in_disableToggleFullscreen( OSWindow wnd )
	{
		in_removeAction( g_togfullAct2, wnd );
		in_removeAction( g_togfullAct1, wnd );
	}
	DOLL_FUNC Bool DOLL_API in_toggleFullscreenEnabled( OSWindow wnd )
	{
		return in_hasAction( g_togfullAct1, wnd ) || in_hasAction( g_togfullAct2, wnd );
	}

	static const SCoreInputAction g_screencapAct = { EKey::F2, 0, ECoreInputAction::CaptureScreenshot };
	DOLL_FUNC Void DOLL_API in_enableScreenshot( OSWindow wnd )
	{
		in_setAction( g_screencapAct, wnd );
	}
	DOLL_FUNC Void DOLL_API in_disableScreenshot( OSWindow wnd )
	{
		in_removeAction( g_screencapAct, wnd );
	}
	DOLL_FUNC Bool DOLL_API in_screenshotEnabled( OSWindow wnd )
	{
		return in_hasAction( g_screencapAct, wnd );
	}

	static const SCoreInputAction g_devconAct = { EKey::Grave, 0, ECoreInputAction::OpenDevcon };
	DOLL_FUNC Void DOLL_API in_enableDevcon( OSWindow wnd )
	{
		in_setAction( g_devconAct, wnd );
	}
	DOLL_FUNC Void DOLL_API in_disableDevcon( OSWindow wnd )
	{
		in_removeAction( g_devconAct, wnd );
	}
	DOLL_FUNC Bool DOLL_API in_devconEnabled( OSWindow wnd )
	{
		return in_hasAction( g_devconAct, wnd );
	}

	static const SCoreInputAction g_devdbgAct = { EKey::F3, 0, ECoreInputAction::ToggleGraphs };
	DOLL_FUNC Void DOLL_API in_enableDevDebug( OSWindow wnd )
	{
		in_setAction( g_devdbgAct, wnd );
	}
	DOLL_FUNC Void DOLL_API in_disableDevDebug( OSWindow wnd )
	{
		in_removeAction( g_devdbgAct, wnd );
	}
	DOLL_FUNC Bool DOLL_API in_devDebugEnabled( OSWindow wnd )
	{
		return in_hasAction( g_devdbgAct, wnd );
	}

}
