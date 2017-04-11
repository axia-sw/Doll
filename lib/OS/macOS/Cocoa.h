#pragma once

#include "doll/Core/Defs.hpp"

namespace doll {

    struct SDesktopInfo;
    struct SWndCreateInfo;

}

namespace doll { namespace macOS {

    namespace app {

        bool init();
        void fini();

        void submitQuitEvent();
        bool waitForAndProcessEvent( bool &inoutReceivedQuit );
        bool processAllQueuedEvents( bool &inoutReceivedQuit );

    }

    namespace monitor {

        bool queryDesktopInfo( SDesktopInfo &dst );

    }

    namespace wm {

        typedef struct {} *Window;

        Window open( const SWndCreateInfo &info, void *initData );
        void close( Window );
        void minimize( Window );
        void maximize( Window );

        void *getData( Window );

        void setTitle( Window, Str );
        UPtr getTitle( Window, char *pszOutTitleUTF8, UPtr cMaxOutBytes );

        void performClose( Window );
        void performMinimize( Window );
        void performMaximize( Window );

        void setVisible( Window, bool );
        bool isVisible( Window );

        void setEnabled( Window, bool );
        bool isEnabled( Window );

        void resize( Window, U32 resX, U32 resY );
        void resizeFrame( Window, U32 resX, U32 resY );

        void position( Window, S32 posX, S32 posY );

        void getSize( Window, U32 &dstResX, U32 &dstResY );
        void getFrameSize( Window, U32 &dstResX, U32 &dstResY );

        void getPosition( Window, S32 &dstPosX, S32 &dstPosY );

        void setNeedsDisplay( Window );
        void setRectNeedsDisplay( Window, S32 clientLeft, S32 clientTop, S32 clientRight, S32 clientBottom );

    }

}}
