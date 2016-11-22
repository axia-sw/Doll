#include "doll/OS/OpenGL.hpp"

#include "doll/Core/Logger.hpp"

namespace doll
{

#if AX_OS_WINDOWS
	SGLContext::SGLContext()
	: hDC( NULL )
	, hRC( NULL )
	, hWnd( NULL )
	, bIsFullscreen( false )
	{
	}
	SGLContext::~SGLContext()
	{
		if( os_isActiveGL( *this ) ) {
			os_deactivateGL();
		}

		if( hRC != NULL ) {
			wglDeleteContext( hRC );
			hRC = NULL;
		}

		if( hDC != NULL ) {
			ReleaseDC( hWnd, hDC );
			hDC = NULL;
		}
	}
#endif

	DOLL_FUNC SGLContext *DOLL_API os_initGL( OSWindow wnd, const SGLInitInfo &info )
	{
		AX_ASSERT_NOT_NULL( wnd );

		SGLContext *const pCtx = new SGLContext();
		if( !AX_VERIFY_MEMORY( pCtx ) ) {
			return nullptr;
		}

#if AX_OS_WINDOWS
		pCtx->hWnd = HWND( wnd );
		pCtx->hDC = GetDC( pCtx->hWnd );

		PIXELFORMATDESCRIPTOR pfd;

		pfd.nSize = sizeof( pfd );
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cRedBits = 0;
		pfd.cRedShift = 0;
		pfd.cGreenBits = 0;
		pfd.cGreenShift = 0;
		pfd.cBlueBits = 0;
		pfd.cBlueShift = 0;
		pfd.cAlphaBits = 0;
		pfd.cAlphaShift = 0;
		pfd.cAccumBits = 0;
		pfd.cAccumRedBits = 0;
		pfd.cAccumGreenBits = 0;
		pfd.cAccumBlueBits = 0;
		pfd.cAccumAlphaBits = 0;
		pfd.cDepthBits = info.cDepthBits;
		pfd.cStencilBits = info.cStencilBits;
		pfd.cAuxBuffers = 0;
		pfd.iLayerType = PFD_MAIN_PLANE;
		pfd.bReserved = 0;
		pfd.dwLayerMask = 0;
		pfd.dwVisibleMask = 0;
		pfd.dwDamageMask = 0;

		const int iPixelFmt = ChoosePixelFormat( pCtx->hDC, &pfd );
		if( !iPixelFmt ) {
			delete pCtx;
			return nullptr;
		}

		if( !SetPixelFormat( pCtx->hDC, iPixelFmt, &pfd ) ) {
			delete pCtx;
			return nullptr;
		}

		if( !( pCtx->hRC = wglCreateContext( pCtx->hDC ) ) ) {
			delete pCtx;
			return nullptr;
		}
#endif

		if( !os_setActiveGL( *pCtx ) ) {
			delete pCtx;
			return nullptr;
		}

		return pCtx;
	}
	DOLL_FUNC SGLContext *DOLL_API os_finiGL( SGLContext *pCtx )
	{
		delete pCtx;
		return nullptr;
	}

	DOLL_FUNC Bool DOLL_API os_setActiveGL( SGLContext &ctx )
	{
#if AX_OS_WINDOWS
		return wglMakeCurrent( ctx.hDC, ctx.hRC ) != FALSE;
#endif
	}
	DOLL_FUNC Void DOLL_API os_deactivateGL()
	{
#if AX_OS_WINDOWS
		wglMakeCurrent( NULL, NULL );
#endif
	}

	DOLL_FUNC Bool DOLL_API os_isActiveGL( const SGLContext &ctx )
	{
#if AX_OS_WINDOWS
		return
			wglGetCurrentDC() == ctx.hDC &&
			wglGetCurrentContext() == ctx.hRC;
#endif
	}
	DOLL_FUNC Bool DOLL_API os_hasActiveGL()
	{
#if AX_OS_WINDOWS
		return wglGetCurrentContext() != NULL;
#endif
	}

	DOLL_FUNC Bool DOLL_API os_swapGL()
	{
#if AX_OS_WINDOWS
		return SwapBuffers( wglGetCurrentDC() ) != FALSE;
#endif
	}

#if AX_OS_WINDOWS
	DOLL_FUNC HDC DOLL_API os_getHDC_mswin( SGLContext &ctx )
	{
		return ctx.hDC;
	}
	DOLL_FUNC HGLRC DOLL_API os_getHGLRC_mswin( SGLContext &ctx )
	{
		return ctx.hRC;
	}
	DOLL_FUNC HWND DOLL_API os_getHWND_mswin( SGLContext &ctx )
	{
		return ctx.hWnd;
	}
#endif

}
