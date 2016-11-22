#include "doll/OS/App.hpp"

#if AX_OS_WINDOWS
# undef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN 1
# include <Windows.h>
# undef min
# undef max
#endif

namespace doll {

	static Bool g_bReceivedQuit = false;

#if AX_OS_WINDOWS
	namespace Windows
	{

		Void processMessage( MSG &Message )
		{
			if( Message.message == WM_QUIT ) {
				g_bReceivedQuit = true;
			}

			TranslateMessage( &Message );
			DispatchMessageW( &Message );
		}

	}
#endif

	DOLL_FUNC Void DOLL_API os_submitQuitEvent()
	{
#if AX_OS_WINDOWS
		PostQuitMessage( 0 );
#endif
	}
	DOLL_FUNC Bool DOLL_API os_receivedQuitEvent()
	{
		return g_bReceivedQuit;
	}

	DOLL_FUNC Bool DOLL_API os_waitForAndProcessEvent()
	{
#if AX_OS_WINDOWS
		MSG Msg;

		BOOL Result = GetMessageW( &Msg, NULL, 0, 0 );
		if( Result <= FALSE ) {
			g_bReceivedQuit = Result == FALSE;
			return false;
		}

		Windows::processMessage( Msg );
		return true;
#endif
	}
	DOLL_FUNC Bool DOLL_API os_processAllQueuedEvents()
	{
#if AX_OS_WINDOWS
		MSG Msg;
		Bool bDidProcess = false;

		while( PeekMessageW( &Msg, NULL, 0, 0, PM_REMOVE ) ) {
			bDidProcess = true;
			Windows::processMessage( Msg );
		}

		return bDidProcess;
#endif
	}

}
