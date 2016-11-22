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

#ifndef DOLL_GFX_VULKAN_ENABLED
# if !AX_OS_UWP && !AX_OS_MACOSX
#  define DOLL_GFX_VULKAN_ENABLED 1
# else
#  define DOLL_GFX_VULKAN_ENABLED 0
# endif
#endif

#ifndef DOLL_GFX_DIRECT3D11_ENABLED
# if AX_OS_WINDOWS || AX_OS_UWP
#  define DOLL_GFX_DIRECT3D11_ENABLED 1
# else
#  define DOLL_GFX_DIRECT3D11_ENABLED 0
# endif
#endif

#ifndef DOLL_GFX_DIRECT3D12_ENABLED
# if AX_OS_WINDOWS || AX_OS_UWP
#  define DOLL_GFX_DIRECT3D12_ENABLED 1
# else
#  define DOLL_GFX_DIRECT3D12_ENABLED 0
# endif
#endif

// FIXME: Implement Vulkan API
#undef  DOLL_GFX_VULKAN_ENABLED
#define DOLL_GFX_VULKAN_ENABLED 0

// FIXME: Implement Direct3D 12 API
#undef  DOLL_GFX_DIRECT3D12_ENABLED
#define DOLL_GFX_DIRECT3D12_ENABLED 0

// OpenGL API
#if DOLL_GFX_OPENGL_ENABLED
DOLL_GFX__API(OpenGL, GL)
#endif

// Vulkan API
#if DOLL_GFX_VULKAN_ENABLED
DOLL_GFX__API(Vulkan, Vk)
#endif

// Direct3D 11 API
#if DOLL_GFX_DIRECT3D11_ENABLED
DOLL_GFX__API(Direct3D11, D3D11)
#endif

// Direct3D 12 API
#if DOLL_GFX_DIRECT3D12_ENABLED
DOLL_GFX__API(Direct3D12, D3D12)
#endif

#ifdef DOLL_GFX__UNDEF__API
# undef DOLL_GFX__UNDEF__API
# undef DOLL_GFX__API
#endif
