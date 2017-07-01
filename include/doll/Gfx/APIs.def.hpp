#ifndef DOLL_GFX__API
# define DOLL_GFX__API(Name_,BriefName_)
# define DOLL_GFX__UNDEF__API
#endif

#if !defined(AX_OS_WINDOWS) || !defined(AX_OS_UWP) || !defined(AX_OS_MACOSX)
# error Current OS not defined. (ax_platform not included or is old.)
#endif

#ifndef DOLL_GFX_OPENGL_ENABLED
# if !AX_OS_UWP
#  define DOLL_GFX_OPENGL_ENABLED 1
# else
#  define DOLL_GFX_OPENGL_ENABLED 0
# endif
#endif

#ifndef DOLL_GFX_DIRECT3D11_ENABLED
# if AX_OS_WINDOWS || AX_OS_UWP
#  define DOLL_GFX_DIRECT3D11_ENABLED 1
# else
#  define DOLL_GFX_DIRECT3D11_ENABLED 0
# endif
#endif

// OpenGL API
#if DOLL_GFX_OPENGL_ENABLED
DOLL_GFX__API(OpenGL, GL)
#endif

// Direct3D 11 API
#if DOLL_GFX_DIRECT3D11_ENABLED
DOLL_GFX__API(Direct3D11, D3D11)
#endif

#ifdef DOLL_GFX__UNDEF__API
# undef DOLL_GFX__UNDEF__API
# undef DOLL_GFX__API
#endif
