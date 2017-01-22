#include "doll/OS/Monitor.hpp"

#include "doll/Core/Logger.hpp"

namespace doll {

#if AX_OS_WINDOWS
	namespace Windows
	{
		static BOOL CALLBACK enumMonitor_f( HMONITOR hMonitor, HDC hMonitorDC, LPRECT pMonitorRect, LPARAM dwData )
		{
			( Void )pMonitorRect;
			
			SDesktopInfo *const pDesktopInfo = reinterpret_cast< SDesktopInfo * >( dwData );
			AX_ASSERT_NOT_NULL( pDesktopInfo );
			if( !pDesktopInfo ) { return FALSE; }

			MONITORINFOEXW Info;
			Info.cbSize = sizeof( Info );

			if( !AX_VERIFY( GetMonitorInfoW( hMonitor, &Info ) != 0 ) ) {
				return FALSE;
			}

			const uintptr uIndex = pDesktopInfo->monitors.num();
			if( !AX_VERIFY( pDesktopInfo->monitors.resize( uIndex + 1 ) ) ) {
				return FALSE;
			}

			SMonitorInfo &monitor = *pDesktopInfo->monitors.pointer( uIndex );
			monitor.deviceName = MutStr::fromWStr( Info.szDevice );
			monitor.resolution.x = Info.rcMonitor.right - Info.rcMonitor.left;
			monitor.resolution.y = Info.rcMonitor.bottom - Info.rcMonitor.top;
			monitor.workArea.x1 = Info.rcWork.left;
			monitor.workArea.y1 = Info.rcWork.top;
			monitor.workArea.x2 = Info.rcWork.right;
			monitor.workArea.y2 = Info.rcWork.bottom;
			monitor.dotsPerInch.x = GetDeviceCaps( hMonitorDC, LOGPIXELSX );
			monitor.dotsPerInch.y = GetDeviceCaps( hMonitorDC, LOGPIXELSY );
			monitor.bIsPrimary = ( Info.dwFlags & MONITORINFOF_PRIMARY ) != 0;
			monitor.pNativeHandle = ( void * )hMonitor;

			if( monitor.bIsPrimary ) {
				pDesktopInfo->primaryMonitor = uIndex;
			}

			if( pDesktopInfo->virtualWorkArea.x1 > monitor.workArea.x1 ) {
				pDesktopInfo->virtualWorkArea.x1 = monitor.workArea.x1;
			}
			if( pDesktopInfo->virtualWorkArea.y1 > monitor.workArea.y1 ) {
				pDesktopInfo->virtualWorkArea.y1 = monitor.workArea.y1;
			}
			if( pDesktopInfo->virtualWorkArea.x2 < monitor.workArea.x2 ) {
				pDesktopInfo->virtualWorkArea.x2 = monitor.workArea.x2;
			}
			if( pDesktopInfo->virtualWorkArea.y2 < monitor.workArea.y2 ) {
				pDesktopInfo->virtualWorkArea.y2 = monitor.workArea.y2;
			}

			return TRUE;
		}
		bool queryDesktopInfo( SDesktopInfo &outInfo )
		{
			// Get information about all monitors
			HDC hdcTest = CreateDCW( L"DISPLAY", nullptr, nullptr, nullptr );
			if( !AX_VERIFY( EnumDisplayMonitors( hdcTest, nullptr, &enumMonitor_f, ( LPARAM )&outInfo ) ) ) {
				DeleteDC( hdcTest );
				return false;
			}
			DeleteDC( hdcTest );

			// Find the launch monitor
			STARTUPINFOW StartupInfo;
			GetStartupInfoW( &StartupInfo );

			outInfo.launchMonitor = outInfo.primaryMonitor;
			if( ( ~StartupInfo.dwFlags & STARTF_USESTDHANDLES ) && StartupInfo.hStdOutput != NULL ) {
				for( uintptr i = 0; i < outInfo.monitors.num(); ++i ) {
					if( outInfo.monitors[ i ].pNativeHandle != ( void * )StartupInfo.hStdOutput ) {
						continue;
					}

					outInfo.launchMonitor = i;
					break;
				}
			}

			// Done
			return true;
		}
	}
#endif

	DOLL_FUNC Bool DOLL_API os_getDesktopInfo( SDesktopInfo &dst )
	{
#if AX_OS_WINDOWS
		if( Windows::queryDesktopInfo( dst ) ) {
			return true;
		}
#endif
		
		dst = SDesktopInfo();
		return false;
	}

}
