#pragma once

#include "../Core/Defs.hpp"
#include "../Core/Logger.hpp"

#include "../Types/IntVector2.hpp"
#include "../Types/Rect.hpp"


namespace doll {

	/*!
	 *	Monitor Information
	 *
	 *	Information about a specific monitor
	 */
	struct SMonitorInfo
	{
		MutStr      deviceName;
		MutStr      identifier;
		SIntVector2 resolution;
		SRect       workArea;
		SIntVector2 dotsPerInch;
		Bool        bIsPrimary;
		Void *      pNativeHandle;
		
		inline SMonitorInfo()
		: deviceName()
		, identifier()
		, resolution()
		, workArea()
		, dotsPerInch()
		, bIsPrimary( false )
		, pNativeHandle( nullptr )
		{
		}
	};

	/*!
	 *	Desktop Information
	 *
	 *	Contains information about the desktop
	 */
	struct SDesktopInfo
	{
		TMutArr< SMonitorInfo > monitors;
		UPtr                    primaryMonitor;
		UPtr                    launchMonitor;
		SRect                   virtualWorkArea;
		
		inline SDesktopInfo()
		: monitors()
		, primaryMonitor( 0 )
		, launchMonitor( 0 )
		, virtualWorkArea()
		{
		}

		inline const SMonitorInfo &getPrimaryMonitor() const
		{
			AX_ASSERT( primaryMonitor < monitors.num() );
			return monitors[ primaryMonitor ];
		}
		inline const SMonitorInfo &getLaunchMonitor() const
		{
			AX_ASSERT( launchMonitor < monitors.num() );
			return monitors[ launchMonitor ];
		}
	};

	// Retrieve information about the desktop (including all monitors)
	DOLL_FUNC Bool DOLL_API os_getDesktopInfo( SDesktopInfo &dst );
	inline SDesktopInfo DOLL_API os_getDesktopInfo()
	{
		SDesktopInfo r;
		return ( os_getDesktopInfo( r ), r );
	}
	// Retrieve information about the primary monitor
	inline SMonitorInfo os_getPrimaryMonitorInfo()
	{
		return os_getDesktopInfo().getPrimaryMonitor();
	}
	// Retrieve the resolution of the primary monitor
	inline SIntVector2 os_getDesktopSize()
	{
		return os_getDesktopInfo().getPrimaryMonitor().resolution;
	}

}
