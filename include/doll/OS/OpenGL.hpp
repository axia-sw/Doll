#pragma once

#include "../Core/Defs.hpp"
#include "Window.hpp"

namespace doll
{

	struct SGLInitInfo
	{
		U32  uMajorVer;
		U32  uMinorVer;
		Bool bCoreContext;
		U32  cDepthBits;
		U32  cStencilBits;
		Bool bIsFullscreen;
		S32  iFullscreenVSync; // If negative then tries swap-tear-control

		inline SGLInitInfo()
		: uMajorVer(0)
		, uMinorVer(0)
		, bCoreContext(false)
		, cDepthBits(0)
		, cStencilBits(0)
		, bIsFullscreen(false)
		, iFullscreenVSync(0)
		{
		}

		inline SGLInitInfo &setVersion( U32 uMajor, U32 uMinor = 0 )
		{
			uMajorVer = uMajor;
			uMinorVer = uMinor;

			return *this;
		}

		inline SGLInitInfo &enableCore()
		{
			bCoreContext = true;
			return *this;
		}
		inline SGLInitInfo &disableCore()
		{
			bCoreContext = false;
			return *this;
		}

		inline SGLInitInfo &setDepthStencil( U32 cDepth, U32 cStencil )
		{
			cDepthBits = cDepth;
			cStencilBits = cStencil;

			return *this;
		}

		inline SGLInitInfo &enableFullscreen( S32 iVSync = 0 )
		{
			bIsFullscreen = true;
			iFullscreenVSync = iVSync;

			return *this;
		}
		inline SGLInitInfo &disableFullscreen()
		{
			bIsFullscreen = false;
			return *this;
		}
		inline SGLInitInfo &setFullscreen( Bool bFull, S32 iVSync = 0 )
		{
			bIsFullscreen = bFull;
			iFullscreenVSync = iVSync;

			return *this;
		}
	};

	struct SGLContext;

	DOLL_FUNC SGLContext *DOLL_API os_initGL( OSWindow, const SGLInitInfo & );
	DOLL_FUNC SGLContext *DOLL_API os_finiGL( SGLContext * );

	DOLL_FUNC Bool DOLL_API os_setActiveGL( SGLContext & );
	DOLL_FUNC Void DOLL_API os_deactivateGL();

	DOLL_FUNC Bool DOLL_API os_isActiveGL( const SGLContext & );
	DOLL_FUNC Bool DOLL_API os_hasActiveGL();

	DOLL_FUNC Bool DOLL_API os_swapGL();

#if AX_OS_WINDOWS
	DOLL_FUNC HDC DOLL_API os_getHDC_mswin( SGLContext & );
	DOLL_FUNC HGLRC DOLL_API os_getHGLRC_mswin( SGLContext & );
	DOLL_FUNC HWND DOLL_API os_getHWND_mswin( SGLContext & );
#endif

	struct SGLContext
	{
#if AX_OS_WINDOWS
		HDC   hDC;
		HGLRC hRC;
		HWND  hWnd;
		Bool  bIsFullscreen;

		SGLContext();
		~SGLContext();
#endif
	};

}
