#include "Cocoa.h"

#include "doll/OS/Monitor.hpp"
#include "doll/OS/Window.hpp"

#include "../Window_Delegate.hpp"

#import <Cocoa/Cocoa.h>

/*
==============================================================================

    APPLICATION DELEGATE

==============================================================================
*/

@interface DollApplicationDelegate: NSApplication
@end

@implementation DollApplicationDelegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    // FIXME: Ask each window whether we can die now
    return NSTerminateCancel;
}

- (void)applicationDidChangeScreenParameters:(NSNotification *)notif
{
    // FIXME: Update monitor info and adjust windows as necessary
}

- (void)applicationDidFinishLaunching:(NSNotification *)notif
{
    [NSApp stop:nil];
    // FIXME: Post an empty event
}

- (void)applicationDidHide:(NSNotification *)notif
{
    // FIXME: Restore previous display modes if we changed it
}
@end

/*
==============================================================================

    WINDOW IMPLEMENTATION

==============================================================================
*/

@interface DollWindow: NSWindow
{
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

namespace doll { namespace macOS {

    static NSAutoreleasePool *_autoreleasePool = nullptr;

    inline wm::Window asDollWnd( NSWindow *w ) {
        return reinterpret_cast< wm::Window >( w );
    }
    inline NSWindow *asNSWnd( wm::Window w ) {
        return reinterpret_cast< NSWindow * >( w );
    }

    namespace app {

        bool init() {
            return true;
        }
        void fini() {
        }

        static void processEvent( NSEvent *event, bool &inoutReceivedQuit ) {
            ((void)inoutReceivedQuit);
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


        NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
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
            for(;;) {
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

        bool queryDesktopInfo( SDesktopInfo &dst ) {
            return false;
        }

    }

    namespace wm {

        Window open( const SWndCreateInfo &info, void *initData ) {
            return nullptr;
        }
        void close( Window wnd ) {
            [asNSWnd(wnd) close];
        }
        void minimize( Window wnd ) {
            [asNSWnd(wnd) miniaturize:nil];
        }
        void maximize( Window wnd ) {
            [asNSWnd(wnd) zoom:nil];
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
            [asNSWnd(wnd) performClose:nil];
        }
        void performMinimize( Window wnd ) {
            [asNSWnd(wnd) performMiniaturize:nil];
        }
        void performMaximize( Window wnd ) {
            [asNSWnd(wnd) performZoom:nil];
        }

        void setVisible( Window wnd, bool visible ) {
            [asNSWnd(wnd) setIsVisible:visible];
        }
        bool isVisible( Window wnd ) {
            return !![asNSWnd(wnd) isVisible];
        }

        void setEnabled( Window wnd, bool enable ) {
            // There doesn't seem to be a clear alternative on macOS
            ((void)wnd);
            ((void)enable);
        }
        bool isEnabled( Window wnd ) {
            ((void)wnd);
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
            [[asNSWnd(wnd) contentView] setNeedsDisplay:YES];
        }
        void setRectNeedsDisplay( Window wnd, S32 clientLeft, S32 clientTop, S32 clientRight, S32 clientBottom ) {
            NSView *const view = [asNSWnd(wnd) contentView];

            NSRect rect;
            rect.origin.x = double(clientLeft);
            rect.origin.y = double(clientTop);
            rect.size.width = double(clientRight - clientLeft);
            rect.size.height = double(clientBottom - clientTop);

            if( [view isFlipped] ) {
                rect.origin.y = [view bounds].size.height - rect.origin.y;
            }

            [view setNeedsDisplayInRect:rect];
        }

    }

}}
