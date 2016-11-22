#pragma once

#include "../Core/Defs.hpp"

#include "Const.hpp"
#include "Bits.hpp"

#include "Intrinsics.hpp"

#include <math.h>

/// @def DOLL_LB(x)
/// Retrieve the index of the last bit of an integer, \a x.
#define DOLL_LB( x ) ( sizeof( x )*8 - 1 )

namespace doll {

	/// Dot product
	///
	/// Used generically by `slerp()`
	inline F32 dot( F32 x, F32 y )
	{
		return x*y;
	}

	/// Convert degrees to radians.
	inline F32 degrees( F32 x )
	{
		return x/180.0f*DOLL_PI;
	}
	/// Convert radians to degrees.
	inline F32 radians( F32 x )
	{
		return x/DOLL_PI*180.0f;
	}

	/// Cosine with input in degrees
	inline F32 cos( F32 fAngleInDegrees )
	{
		return ::cosf( radians( fAngleInDegrees ) );
	}
	/// Sine with input in degrees
	inline F32 sin( F32 fAngleInDegrees )
	{
		return ::sinf( radians( fAngleInDegrees ) );
	}
	/// Tangent with input in degrees
	inline F32 tan( F32 fAngleInDegrees )
	{
		return ::tanf( radians( fAngleInDegrees ) );
	}
	/// Square root (just a wrapper for consistency)
	inline F32 sqrt( F32 fValue )
	{
		return ::sqrtf( fValue );
	}

	/// Get the sign of a number.
	///
	/// @return -1, 0, or 1.
	inline F32 sign( F32 x )
	{
		return ( F32 )( 1|( ( S32 )x >> DOLL_LB( x ) ) );
	}
	/// Get the sign of an integer number.
	///
	/// @return -1, 0, or 1.
	inline S32 sign( S32 x )
	{
		return bitSign( x );
	}

	/// Get 1 if the value is > 0 or 0 if <= 0
	///
	/// @return 1 or 0
	inline F32 heaviside( F32 x )
	{
#if 0
		// TODO: Test this code path
		return uintBitsToFloat( ~floatToUintBits( x ) >> 31 );
#else
		return x > 0.0f ? 1.0f : 0.0f;
#endif
	}

	/// Get the absolute value of a number.
	inline F32 abs( F32 x )
	{
		return uintBitsToFloat( floatToUintBits( x ) & ( U32 )~DOLL_LB( x ) );
	}
	/// Get the absolute value of an integer number.
	inline S32 abs( S32 x )
	{
		return bitAbs( x );
	}

	/// Copy the sign of 'b' into 'a'.
	inline F32 copySign( F32 a, F32 b )
	{
		return sign( b )*abs( a );
	}
	/// Copy the sign of 'b' into 'a'
	inline S32 copySign( S32 a, S32 b )
	{
		return bitCopySign( a, b );
	}

	/// Find the minimum of two integer values.
	inline S32 min( S32 a, S32 b )
	{
		return a < b ? a : b;
	}
	/// Find the maximum of two integer values.
	inline S32 max( S32 a, S32 b )
	{
		return a > b ? a : b;
	}
	/// Find the minimum of two values.
	inline F32 min( F32 a, F32 b )
	{
#if AX_INTRIN_SSE
		F32 r;
		_mm_store_ss( &r, _mm_min_ss( _mm_set_ss( a ), _mm_set_ss( b ) ) );
		return r;
#else
		return a < b ? a : b;
#endif
	}
	/// Find the maximum of two values.
	inline F32 max( F32 a, F32 b )
	{
#if AX_INTRIN_SSE
		F32 r;
		_mm_store_ss( &r, _mm_max_ss( _mm_set_ss( a ), _mm_set_ss( b ) ) );
		return r;
#else
		return a > b ? a : b;
#endif
	}
	/// Clamp a value to a range.
	inline F32 clamp( F32 x, F32 l, F32 h )
	{
#if AX_INTRIN_SSE
		F32 r = 0.0f;
		return _mm_store_ss( &r, _mm_min_ss( _mm_max_ss( _mm_set_ss( x ), _mm_set_ss( l ) ), _mm_set_ss( h ) ) ), r;
#else
		F32 r = x;

		r = ( r + h - abs( r - h ) )*0.5f;
		r = ( r + l - abs( r - l ) )*0.5f;

		return r;
#endif
	}
	/// Clamp a value to the range of 0 to 1.
	inline F32 saturate( F32 x )
	{
		return clamp( x, 0.0f, 1.0f );
	}
	/// Clamp a value to the range of -1 to 1.
	inline F32 saturateSigned( F32 x )
	{
		return clamp( x, -1.0f, 1.0f );
	}

	/// Linearly interpolate between two values.
	template< typename T, typename U >
	inline T lerp( const T &x, const T &y, const U &t )
	{
		return x + ( y - x )*t;
	}
	/// Cubically interpolate between four values.
	template< typename T, typename U >
	inline T cerp( const T &x, const T &y, const T &z, const T &w, const U &t )
	{
		const T a = ( w - z ) - ( x - y );
		const T b = ( x - y ) - a;
		const T c = z - x;
		return t*( t*( t*a + b ) + c ) + y;
	}
	/// Spherically interpolate between two values.
	template< typename T >
	inline T slerp( const T &a, const T &b, F32 t )
	{
		if( t <= 0.0f ) {
			return a;
		}
		if( t >= 1.0f ) {
			return b;
		}

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4244) //conversion from 'x' to 'y', possible loss of data
#endif
		const F32 cosom = fmodf( dot( a, b ), 1.0f );
#ifdef _MSC_VER
# pragma warning(pop)
#endif

		F32 scale[ 2 ];
		if( ( 1.0f - cosom ) > 1e-8f ) {
			const F32 omega = acosf( cosom );
			const F32 sinom = sinf( omega );
			scale[ 0 ] = sinf( ( 1.0f - t )*omega )/sinom;
			scale[ 1 ] = sinf( t*omega )/sinom;
		} else {
			scale[ 0 ] = 1.0f - t;
			scale[ 1 ] = t;
		}

		return a*scale[ 0 ] + b*scale[ 1 ];
	}

	/// Round a value down
	inline F32 floor( F32 f )
	{
		if( ( floatToUintBits( f ) & 0x801FFFFF ) > 0x80000000 ) {
			return ( F32 )( ( int )f - 1 );
		}

		return ( F32 )( int )f;
	}

	/// Wrap an angle between 0 and 360.
	inline F32 wrap360( F32 angle )
	{
		// This simultaneously checks whether angle is above 360 or below 0
		// 0x43B40000 is the integer bits representation 360.0f
		if( floatToUintBits( angle ) >= 0x43B40000 ) {
			angle -= floor( angle/360.0f )*360.0f;
		}
		
		return angle;
	}
	/// Wrap an angle between -180 and 180.
	inline F32 wrap180( F32 angle )
	{
		angle = wrap360( angle );
		return angle > 180.0f ? angle - 360.0f : angle;
	}

	/// Calculate the delta between two angles.
	///
	/// @return Angle between -180 and 180.
	inline F32 angleDelta( F32 a, F32 b )
	{
		return wrap180( a - b );
	}

	/// Get the fractional part of a F32.
	inline F32 frac( F32 x )
	{
		return x - floor( x );
	}

	/// Approximate a square root.
	inline F32 fastSqrt( F32 x )
	{
#if AX_INTRIN_SSE
		return _mm_store_ss( &x, _mm_sqrt_ss( _mm_load_ss( &x ) ) ), x;
#else
		S32 i = floatToIntBits( x );

		i  -= 0x3F800000;
		i >>= 1;
		i  += 0x3F800000;

		return intBitsToFloat( i );
#endif
	}
	/// Approximate the reciprocal of a square root.
	inline F32 fastInvSqrt( F32 x )
	{
#if AX_INTRIN_SSE
		return _mm_store_ss( &x, _mm_rsqrt_ss( _mm_load_ss( &x ) ) ), x;
#else
		const F32 f = intBitsToFloat( 0x5F3759DF - ( floatToIntBits( x ) >> 1 ) );
		return f*( 1.5f - 0.5f*x*f*f );
#endif
	}

}
