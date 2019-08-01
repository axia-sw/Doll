#include "Cocoa.h"
#include "../../BuildSettings.hpp"

#include "doll/OS/Monitor.hpp"
#include "doll/OS/Window.hpp"

#include "../Window_Delegate.hpp"

#import <Cocoa/Cocoa.h>
#include <IOKit/graphics/IOGraphicsLib.h>

/*
==============================================================================

    APPLICATION DELEGATE

==============================================================================
*/

@interface DollApplicationDelegate : NSApplication
@end

@implementation DollApplicationDelegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
	// FIXME: Ask each window whether we can die now
	return NSTerminateCancel;
}

- (void)applicationDidChangeScreenParameters:(NSNotification *)notif {
	// FIXME: Update monitor info and adjust windows as necessary
}

- (void)applicationDidFinishLaunching:(NSNotification *)notif {
	[NSApp stop:nil];
	// FIXME: Post an empty event
}

- (void)applicationDidHide:(NSNotification *)notif {
	// FIXME: Restore previous display modes if we changed it
}
@end

/*
==============================================================================

    WINDOW IMPLEMENTATION

==============================================================================
*/

@interface DollWindow : NSWindow {
	void *_userData;
}
@end

@implementation DollWindow
@end

/*
==============================================================================

    MAIN WRAPPER CODE

==============================================================================
*/

namespace doll {
namespace macOS {

	static NSAutoreleasePool *_autoreleasePool = nullptr;

	inline wm::Window asDollWnd( NSWindow *w ) {
		return reinterpret_cast<wm::Window>( w );
	}
	inline NSWindow *asNSWnd( wm::Window w ) {
		return reinterpret_cast<NSWindow *>( w );
	}

	namespace app {

		bool init() {
			return true;
		}
		void fini() {
		}

		static void processEvent( NSEvent *event, bool &inoutReceivedQuit ) {
			( (void)inoutReceivedQuit );
			[NSApp sendEvent:event];
		}
		static void drainPool() {
			if( __builtin_expect( ( _autoreleasePool != nullptr ), true ) ) {
				[_autoreleasePool drain];
			}
			_autoreleasePool = [[NSAutoreleasePool alloc] init];
		}

		void submitQuitEvent() {
			[NSApp terminate:nil];
		}

		NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
		                                    untilDate:[NSDate distantPast]
		                                       inMode:NSDefaultRunLoopMode
		                                      dequeue:YES];
		bool waitForAndProcessEvent( bool &inoutReceivedQuit ) {
			NSEvent *const event =
			    [NSApp nextEventMatchingMask:NSEventMaskAny
			                       untilDate:[NSDate distantFuture]
			                          inMode:NSDefaultRunLoopMode
			                         dequeue:YES];
			if( !event ) {
				inoutReceivedQuit = true;
				return false;
			}

			processEvent( event, inoutReceivedQuit );
			drainPool();

			return true;
		}
		bool processAllQueuedEvents( bool &inoutReceivedQuit ) {
			bool didProcessEvent = false;
			for( ;; ) {
				NSEvent *const event =
				    [NSApp nextEventMatchingMask:NSEventMaskAny
				                       untilDate:[NSDate distantPast]
				                          inMode:NSDefaultRunLoopMode
				                         dequeue:YES];
				if( !event ) {
					break;
				}

				processEvent( event, inoutReceivedQuit );
				didProcessEvent = true;
			}

			drainPool();
			return true;
		}

	}

	namespace monitor {

		// =BEGIN= : GLFW CODE : =BEGIN= //

		//========================================================================
		// GLFW 3.1 OS X - www.glfw.org
		//------------------------------------------------------------------------
		// Copyright (c) 2002-2006 Marcus Geelnard
		// Copyright (c) 2006-2010 Camilla Berglund <elmindreda@elmindreda.org>
		//
		// This software is provided 'as-is', without any express or implied
		// warranty. In no event will the authors be held liable for any damages
		// arising from the use of this software.
		//
		// Permission is granted to anyone to use this software for any purpose,
		// including commercial applications, and to alter it and redistribute it
		// freely, subject to the following restrictions:
		//
		// 1. The origin of this software must not be misrepresented; you must not
		//    claim that you wrote the original software. If you use this software
		//    in a product, an acknowledgment in the product documentation would
		//    be appreciated but is not required.
		//
		// 2. Altered source versions must be plainly marked as such, and must not
		//    be misrepresented as being the original software.
		//
		// 3. This notice may not be removed or altered from any source
		//    distribution.
		//
		//========================================================================

		// Returns the io_service_t corresponding to a CG display ID, or 0 on failure.
		// The io_service_t should be released with IOObjectRelease when not needed.
		//
		static io_service_t IOServicePortFromCGDisplayID(CGDirectDisplayID displayID) {
			io_iterator_t iter;
			io_service_t serv, servicePort = 0;
			
			CFMutableDictionaryRef matching = IOServiceMatching("IODisplayConnect");
			
			// releases matching for us
			kern_return_t err = IOServiceGetMatchingServices(kIOMasterPortDefault, matching, &iter);
			if (err) {
				return 0;
			}
			
			while ((serv = IOIteratorNext(iter)) != 0) {
				CFDictionaryRef info;
				CFIndex vendorID, productID, serialNumber;
				CFNumberRef vendorIDRef, productIDRef, serialNumberRef;
				Boolean success = true;
				
				info = IODisplayCreateInfoDictionary(serv, kIODisplayOnlyPreferredName);
				
				vendorIDRef = (CFNumberRef)CFDictionaryGetValue(info, CFSTR(kDisplayVendorID));
				productIDRef = (CFNumberRef)CFDictionaryGetValue(info, CFSTR(kDisplayProductID));
				serialNumberRef = (CFNumberRef)CFDictionaryGetValue(info, CFSTR(kDisplaySerialNumber));
				
				if( vendorIDRef != nil ) {
					success &= CFNumberGetValue(vendorIDRef, kCFNumberCFIndexType, &vendorID);
				}
				if( productIDRef != nil ) {
					success &= CFNumberGetValue(productIDRef, kCFNumberCFIndexType, &productID);
				}
				if( serialNumberRef != nil ) {
					success &= CFNumberGetValue(serialNumberRef, kCFNumberCFIndexType, &serialNumber);
				}
				
				if (!success) {
					CFRelease(info);
					continue;
				}
				
				// If the vendor and product id along with the serial don't match
				// then we are not looking at the correct monitor.
				// NOTE: The serial number is important in cases where two monitors
				//       are the exact same.
				if (CGDisplayVendorNumber(displayID) != vendorID  ||
					CGDisplayModelNumber(displayID) != productID  ||
					CGDisplaySerialNumber(displayID) != serialNumber)
				{
					CFRelease(info);
					continue;
				}
				
				// The VendorID, Product ID, and the Serial Number all Match Up!
				// Therefore we have found the appropriate display io_service
				servicePort = serv;
				CFRelease(info);
				break;
			}
			
			IOObjectRelease(iter);
			return servicePort;
		}

		// =END= : GLFW CODE : =END= //

		bool queryDesktopInfo( SDesktopInfo &dst ) {
			TMutArr<SMonitorInfo> monitors;

			@autoreleasepool {
				int screenIndex = 0;
				for( id screenId in [NSScreen screens] ) {
					NSScreen *const screen = screenId;
					if( !screen ) {
						return false;
					}

					const NSRect frame        = [screen frame];
					const NSRect visibleFrame = [screen visibleFrame];

					NSDictionary *const description = [screen deviceDescription];
					NSNumber *const displayId       = [description objectForKey:@"NSScreenNumber"];
					CGDirectDisplayID displayIdUInt = [displayId unsignedIntValue];

					const NSSize pixelSize    = [[description objectForKey:NSDeviceSize] sizeValue];
					const CGSize physicalSize = CGDisplayScreenSize( displayIdUInt );

					io_service_t serv = IOServicePortFromCGDisplayID( displayIdUInt );

					NSString *screenName = @"macOS Display";

					if( serv != 0 ) {
						NSDictionary *deviceInfo     = (NSDictionary *)IODisplayCreateInfoDictionary( serv, kIODisplayOnlyPreferredName );
						NSDictionary *localizedNames = [deviceInfo objectForKey:[NSString stringWithUTF8String:kDisplayProductName]];

						if( [localizedNames count] > 0 ) {
							screenName = [[localizedNames objectForKey:[[localizedNames allKeys] objectAtIndex:0]] retain];
						}
					}

					const float inchesScalar = 25.4f;

					SMonitorInfo mon;

					mon.bIsPrimary    = screenIndex++ == 0;
					mon.deviceName    = [screenName UTF8String];
					mon.identifier    = axf( "%u", [displayId unsignedIntValue] );
					mon.pNativeHandle = nullptr;
					mon.resolution    = SIntVector2( (int)frame.size.width, (int)frame.size.height );
					mon.dotsPerInch.x = (int)( pixelSize.width / physicalSize.width * inchesScalar );
					mon.dotsPerInch.y = (int)( pixelSize.height / physicalSize.height * inchesScalar );
					mon.workArea.positionMe( SIntVector2( (int)visibleFrame.origin.x, (int)visibleFrame.origin.y ) );
					mon.workArea.resizeMe( SIntVector2( (int)visibleFrame.size.width, (int)visibleFrame.size.height ) );

					if( serv != 0 ) {
						IOObjectRelease( serv );
					}

					if( !AX_VERIFY_MEMORY( monitors.tryAppend( mon ) ) ) {
						return false;
					}
				}
			}

			dst.monitors.swap( monitors );
			dst.primaryMonitor = 0;
			dst.launchMonitor  = 0;

			dst.virtualWorkArea = SRect();
			for( const SMonitorInfo &mon : dst.monitors ) {
				dst.virtualWorkArea.combineWith( mon.workArea );
			}

			return true;
		}

	}

	namespace wm {

		Window open( const SWndCreateInfo &info, void *initData ) {
			return nullptr;
		}
		void close( Window wnd ) {
			[asNSWnd( wnd ) close];
		}
		void minimize( Window wnd ) {
			[asNSWnd( wnd ) miniaturize:nil];
		}
		void maximize( Window wnd ) {
			[asNSWnd( wnd ) zoom:nil];
		}

		void *getData( Window wnd ) {
			//
			return nullptr;
		}

		void setTitle( Window wnd, Str title ) {
			//
		}
		UPtr getTitle( Window wnd, char *pszOutTitleUTF8, UPtr cMaxOutBytes ) {
			return 0;
		}

		void performClose( Window wnd ) {
			[asNSWnd( wnd ) performClose:nil];
		}
		void performMinimize( Window wnd ) {
			[asNSWnd( wnd ) performMiniaturize:nil];
		}
		void performMaximize( Window wnd ) {
			[asNSWnd( wnd ) performZoom:nil];
		}

		void setVisible( Window wnd, bool visible ) {
			[asNSWnd( wnd ) setIsVisible:visible];
		}
		bool isVisible( Window wnd ) {
			return !![asNSWnd( wnd ) isVisible];
		}

		void setEnabled( Window wnd, bool enable ) {
			// There doesn't seem to be a clear alternative on macOS
			( (void)wnd );
			( (void)enable );
		}
		bool isEnabled( Window wnd ) {
			( (void)wnd );
			return true;
		}

		void resize( Window wnd, U32 resX, U32 resY ) {
			//
		}
		void resizeFrame( Window wnd, U32 resX, U32 resY ) {
			//
		}

		void position( Window wnd, S32 posX, S32 posY ) {
			//
		}

		void getSize( Window wnd, U32 &dstResX, U32 &dstResY ) {
			//
			dstResX = 0;
			dstResY = 0;
		}
		void getFrameSize( Window wnd, U32 &dstResX, U32 &dstResY ) {
			//
			dstResX = 0;
			dstResY = 0;
		}

		void getPosition( Window wnd, S32 &dstPosX, S32 &dstPosY ) {
			//
			dstPosX = 0;
			dstPosY = 0;
		}

		void setNeedsDisplay( Window wnd ) {
			[[asNSWnd( wnd ) contentView] setNeedsDisplay:YES];
		}
		void setRectNeedsDisplay( Window wnd, S32 clientLeft, S32 clientTop, S32 clientRight, S32 clientBottom ) {
			NSView *const view = [asNSWnd( wnd ) contentView];

			NSRect rect;
			rect.origin.x    = double( clientLeft );
			rect.origin.y    = double( clientTop );
			rect.size.width  = double( clientRight - clientLeft );
			rect.size.height = double( clientBottom - clientTop );

			if( [view isFlipped] ) {
				rect.origin.y = [view bounds].size.height - rect.origin.y;
			}

			[view setNeedsDisplayInRect:rect];
		}

	}

}
}
