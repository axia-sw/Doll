#pragma once

#ifdef _WIN32
# ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0603
# endif
#endif

#ifndef DOLL_EXTRNC
# ifdef __cplusplus
#  define DOLL_EXTRNC extern "C"
# else
#  define DOLL_EXTRNC
# endif
#endif

#ifndef DOLL_DLLFUNC
# ifdef _WIN32
#  ifdef DOLL__BUILD
#   define DOLL_DLLFUNC __declspec(dllexport)
#  else
#   define DOLL_DLLFUNC __declspec(dllimport)
#  endif
# else
#  define DOLL_DLLFUNC
# endif
#endif

#ifndef DOLL_FUNC
# define DOLL_FUNC DOLL_EXTRNC DOLL_DLLFUNC
#endif

#ifndef DOLL_API
# ifdef _WIN32
#  define DOLL_API __stdcall
# else
#  define DOLL_API
# endif
#endif

#define AX_FUNC       DOLL_DLLFUNC
#define AX_CALL       DOLL_API

#define AXASSERT_FUNC DOLL_DLLFUNC
#define AXASSERT_CALL DOLL_API

#define AXCONF_FUNC   DOLL_DLLFUNC
#define AXCONF_CALL   DOLL_API

#define AXFIBER_FUNC  DOLL_DLLFUNC
#define AXFIBER_CALL  DOLL_API

#define AXLOG_FUNC    DOLL_DLLFUNC
#define AXLOG_CALL    DOLL_API

#define AXMM_FUNC     DOLL_DLLFUNC
#define AXMM_CALL     DOLL_API

#define AXPF_FUNC     DOLL_DLLFUNC
#ifdef _WIN32
# define AXPF_CALL    __cdecl
#else
# define AXPF_CALL
#endif

#define AXSTR_FUNC    DOLL_DLLFUNC
#define AXSTR_CALL    DOLL_API

#define AXTHREAD_FUNC DOLL_DLLFUNC
#define AXTHREAD_CALL DOLL_API

#define AXTIME_FUNC   DOLL_DLLFUNC
#define AXTIME_CALL   DOLL_API
