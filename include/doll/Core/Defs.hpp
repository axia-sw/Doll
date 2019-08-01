#pragma once

#include "../AxlibConfig.h"

#include "ax_intdatetime.h"

#include "ax_platform.h"
#include "ax_types.h"

#include "ax_printf.h"
#include "ax_string.h"
#include "ax_logger.h"
#include "ax_assert.h"

#include "ax_thread.h"
#include "ax_memory.h"

#include "ax_time.h"
#include "ax_thread.h"
//#include "ax_fiber.h"

#include "ax_config.h"

#include "ax_typetraits.hpp"
#include "ax_manager.hpp"
#include "ax_list.hpp"
#include "ax_array.hpp"

#include "ax_dictionary.hpp"

#ifdef DOLL__BUILD
# include "Private/Variants.hpp"
# ifndef DOLL__SECURE_LIB
#  if defined( _MSC_VER ) && defined( __STDC_WANT_SECURE_LIB__ )
#   define DOLL__SECURE_LIB __STDC_WANT_SECURE_LIB__
#  else
#   define DOLL__SECURE_LIB 0
#  endif
# endif
// When building we use a different internal logging channel
# ifndef DOLL_TRACE_FACILITY
#  define DOLL_TRACE_FACILITY doll::kLog_Internal
# endif
#endif

// Temporary hack
#ifndef DOLL__USE_GLFW
# ifdef _WIN32
#  define DOLL__USE_GLFW 0
# else
#  define DOLL__USE_GLFW 1
# endif
#endif

#define DOLL_RGBA(R_,G_,B_,A_)\
	(\
		( U32((A_)&0xFF)<<24 ) |\
		( U32((B_)&0xFF)<<16 ) |\
		( U32((G_)&0xFF)<< 8 ) |\
		( U32((R_)&0xFF)<< 0 )\
	)
#define DOLL_RGB(R_,G_,B_)\
	DOLL_RGBA((R_),(G_),(B_),0xFF)

#define DOLL_COLOR_R(RGBA_)\
	( ( U32(RGBA_)&0x000000FF )>>0 )
#define DOLL_COLOR_G(RGBA_)\
	( ( U32(RGBA_)&0x0000FF00 )>>8 )
#define DOLL_COLOR_B(RGBA_)\
	( ( U32(RGBA_)&0x00FF0000 )>>16 )
#define DOLL_COLOR_A(RGBA_)\
	( ( U32(RGBA_)&0xFF000000 )>>24 )

#ifndef DOLL_DX_AVAILABLE
# if defined( _WIN32 ) || defined( _XBOX )
#  define DOLL_DX_AVAILABLE 1
# else
#  define DOLL_DX_AVAILABLE 0
# endif
#endif

namespace doll
{

	using namespace ax;

	enum EResult
	{
		kSuccess = 1,

		kError_OutOfMemory = -4096,
		kError_InvalidParameter,
		kError_InvalidOperation,
		kError_Overflow,
		kError_Underflow
	};

	DOLL_FUNC Bool DOLL_API app_getPath( Str &dst );
	inline Str DOLL_API app_getPath()
	{
		Str r;
		return app_getPath( r ), r;
	}
	inline Str DOLL_API app_getDir()
	{
		return app_getPath().getDirectory();
	}

	inline Str DOLL_API app_getName()
	{
		const Str x = app_getPath().getBasename();
		if( x.caseEndsWith( "dbg" ) ) {
			return x.drop( 3 );
		}
		return x;
	}

	AX_CONSTEXPR_INLINE U32 doll_rgb( F32 r, F32 g, F32 b, F32 a = 1.0f )
	{
		return
			DOLL_RGBA
			(
				U32((r < 0.0f ? 0.0f : r > 1.0f ? 1.0f : r)*255.0f),
				U32((g < 0.0f ? 0.0f : g > 1.0f ? 1.0f : g)*255.0f),
				U32((b < 0.0f ? 0.0f : b > 1.0f ? 1.0f : b)*255.0f),
				U32((a < 0.0f ? 0.0f : a > 1.0f ? 1.0f : a)*255.0f)
			);
	}

	template< typename T, UPtr tNum >
	inline Bool isOneOf( const T x, const T( &buf )[ tNum ] )
	{
		for( const T &y : buf ) {
			if( x == y ) {
				return true;
			}
		}

		return false;
	}
	template< typename T, typename... Args >
	inline Bool isOneOf( const T &x, Args... args ) {
		const T buf[] = { args... };

		for( const T &y : buf ) {
			if( x == y ) {
				return true;
			}
		}

		return false;
	}
	template< typename T >
	inline U16 countBits( T x )
	{
		U16 n = 0;

		while( x ) {
			++n;
			x &= x - 1;
		}

		return n;
	}

	inline Bool takeFlag( U32 &uFlags, U32 uCheck )
	{
		if( ~uFlags & uCheck ) {
			return false;
		}

		uFlags &= ~uCheck;
		return true;
	}

	template< class Functor >
	class TScopeGuard
	{
	public:
		inline TScopeGuard( const Functor &func )
		: mFunc( func )
		, mState( EState::Valid )
		{
		}
		inline TScopeGuard( TScopeGuard< Functor > &&r )
		: mFunc( r.mFunc )
		, mState( r.mState )
		{
			r.commit();
		}
		inline ~TScopeGuard()
		{
			call();
		}

		inline void commit()
		{
			mState = EState::Committed;
		}
		inline void call()
		{
			if( mState != EState::Valid ) {
				return;
			}

			mFunc();
			commit();
		}

		inline void operator()()
		{
			call();
		}

	private:
		enum class EState
		{
			Valid,
			Committed
		};

		const Functor mFunc;
		EState mState;
	};

	template< class Functor >
	inline TScopeGuard< Functor > makeScopeGuard( const Functor &func )
	{
		return forward< TScopeGuard< Functor > >( func );
	}

}
