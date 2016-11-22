#pragma once

#include "../Core/Defs.hpp"

namespace doll {

	/// Unsigned bit-shift right
	template< typename tInt >
	inline tInt bitShiftRightU( tInt x, tInt y )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		typedef typename TMakeUnsigned< tInt >::type tUnsigned;

		return
			static_cast< tInt >(
				static_cast< tUnsigned >( x ) >> y
			);
	}
	/// Signed bit-shift right
	template< typename tInt >
	inline tInt bitShiftRightS( tInt x, tInt y )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		typedef typename TMakeSigned< tInt >::type tSigned;

		return
			static_cast< tInt >(
				static_cast< tSigned >( x ) >> y
			);
	}

	/// Put a zero bit between each of the lower bits of the given value
	template< typename tInt >
	inline tInt bitExpand( tInt x )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );

		tInt r = 0;

		for( uintcpu i = 0; i < ( sizeof( x )*8 )/2; ++i )
		{
			r |= ( x & ( 1 << i ) ) << i;
		}

		return r;
	}
	/// Take each other bit of a value and merge into one value
	template< typename tInt >
	inline tInt bitMerge( tInt x )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );

		tInt r = 0;

		for( uintcpu i = 0; i < ( sizeof( x )*8 )/2; ++i )
		{
			r |= ( x & ( 1 << ( i*2 ) ) ) >> i;
		}

		return r;
	}

	/// Specialization of bitExpand() for uint32
	inline uint32 bitExpand( uint32 x )
	{
		// http://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
		x &= 0xFFFF;

		x = ( x ^ ( x << 8 ) ) & 0x00FF00FF;
		x = ( x ^ ( x << 4 ) ) & 0x0F0F0F0F;
		x = ( x ^ ( x << 2 ) ) & 0x33333333;
		x = ( x ^ ( x << 1 ) ) & 0x55555555;

		return x;
	}
	/// Specialization of bitMerge() for uint32
	inline uint32 bitMerge( uint32 x )
	{
		// http://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
		x &= 0x55555555;

		x = ( x ^ ( x >> 1 ) ) & 0x33333333;
		x = ( x ^ ( x >> 2 ) ) & 0x0F0F0F0F;
		x = ( x ^ ( x >> 4 ) ) & 0x00FF00FF;
		x = ( x ^ ( x >> 8 ) ) & 0x0000FFFF;

		return x;
	}

	/// Turn off the right-most set bit (e.g., 01011000 -> 01010000)
	template< typename tInt >
	inline tInt bitRemoveLowestSet( tInt x )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		return x & ( x - 1 );
	}
	/// Determine whether a number is a power of two
	template< typename tInt >
	inline bool bitIsPowerOfTwo( tInt x )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		return bitRemoveLowestSet( x ) == 0;
	}
	/// Isolate the right-most set bit (e.g., 01011000 -> 00001000)
	template< typename tInt >
	inline tInt bitIsolateLowestSet( tInt x )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		return x & ( -x );
	}
	/// Isolate the right-most clear bit (e.g., 10100111 -> 00001000)
	template< typename tInt >
	inline tInt bitIsolateLowestClear( tInt x )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		return -x & ( x + 1 );
	}
	/// Create a mask of the trailing clear bits (e.g., 01011000 -> 00000111)
	template< typename tInt >
	inline tInt bitIdentifyLowestClears( tInt x )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		return -x & ( x - 1 );
	}
	/// Create a mask that identifies the least significant set bit and the
	/// trailing clear bits (e.g., 01011000 -> 00001111)
	template< typename tInt >
	inline tInt bitIdentifyLowestSetAndClears( tInt x )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		return x ^ ( x - 1 );
	}
	/// Propagate the lowest set bit to the lower clear bits
	template< typename tInt >
	inline tInt bitPropagateLowestSet( tInt x )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		return x | ( x - 1 );
	}
	/// Find the absolute value of an integer
	template< typename tInt >
	inline tInt bitAbs( tInt x )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		const tInt y = bitShiftRightS( x, ( tInt )( sizeof( x )*8 - 1 ) );
		return ( x ^ y ) - y;
	}
	/// Find the sign of an integer
	template< typename tInt >
	inline tInt bitSign( tInt x )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		static const tInt s = sizeof( tInt )*8 - 1;
		return bitShiftRightS( x, s ) | bitShiftRightU( -x, s );
	}
	/// Transfer the sign of src into dst
	template< typename tInt >
	inline tInt bitCopySign( tInt dst, tInt src )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		const tInt t = bitShiftRightS( src, ( tInt )( sizeof( tInt )*8 - 1 ) );
		return ( bitAbs( dst ) + t ) ^ t;
	}
	/// Rotate a field of bits left
	template< typename tInt >
	inline tInt bitRotateLeft( tInt x, tInt y )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		return ( x << y ) | bitShiftRightU( x, sizeof( x )*8 - y );
	}
	/// Rotate a field of bits right
	template< typename tInt >
	inline tInt bitRotateRight( tInt x, tInt y )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );
		return bitShiftRightU( x, y ) | ( x << ( sizeof( x )*8 - y ) );
	}
	/// Count the number of set bits
	template< typename tInt >
	inline tInt bitCount( tInt x )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );

		tInt r = x;

		for( int i = 0; i < sizeof( x )*8; ++i )
		{
			r -= x >> ( 1 << i );
		}

		return r;
	}
	/// Specialization of bitCount() for 32-bit integers
	inline uint32 bitCount( uint32 x )
	{
		x = x - ( ( x >> 1 ) & 0x55555555 );
		x = ( x & 0x33333333 ) + ( ( x >> 2 ) & 0x33333333 );
		x = ( x + ( x >> 4 ) ) & 0x0F0F0F0F;
		x = x + ( x >> 8 );
		x = x + ( x >> 16 );
		return x & 0x0000003F;
	}
	/// Compute the parity of an integer (true for odd, false for even)
	template< typename tInt >
	inline tInt bitParity( tInt x )
	{
		static_assert( TIsInt< tInt >::value, "Integer type required" );

		x = x ^ ( x >> 1 );
		x = x ^ ( x >> 2 );
		x = x ^ ( x >> 4 );

		if( sizeof( x ) > 1 )
		{
			x = x ^ ( x >> 8 );
		
			if( sizeof( x ) > 2 )
			{
				x = x ^ ( x >> 16 );

				if( sizeof( x ) > 4 )
				{
					x = x ^ ( x >> 32 );

					int i = 8;
					while( sizeof( x ) > i )
					{
						x = x ^ ( x >> ( i*8 ) );
						i = i*2;
					}
				}
			}
		}

		return x;
	}

	/// Treat the bits of a signed-integer as a float encoding.
	inline float intBitsToFloat( int32 x )
	{
		union
		{
			float f;
			int32 i;
		} v;

		v.i = x;
		return v.f;
	}
	inline double intBitsToFloat( int64 x )
	{
		union
		{
			double f;
			int64 i;
		} v;

		v.i = x;
		return v.f;
	}

	/// Treat the bits of an unsigned-integer as a float encoding.
	inline float uintBitsToFloat( uint32 x )
	{
		union
		{
			float f;
			uint32 i;
		} v;
		
		v.i = x;
		return v.f;
	}
	inline double uintBitsToFloat( uint64 x )
	{
		union
		{
			double f;
			uint64 i;
		} v;
		
		v.i = x;
		return v.f;
	}
	
	/// Retrieve the encoding of a float's bits as a signed-integer.
	inline int32 floatToIntBits( float x )
	{
		union
		{
			float f;
			int32 i;
		} v;
		
		v.f = x;
		return v.i;
	}
	inline int64 floatToIntBits( double x )
	{
		union
		{
			double f;
			int64 i;
		} v;
		
		v.f = x;
		return v.i;
	}
	
	/// Retrieve the encoding of a float's bits as an unsigned-integer.
	inline uint32 floatToUintBits( float x )
	{
		union
		{
			float f;
			uint32 i;
		} v;
		
		v.f = x;
		return v.i;
	}
	inline uint64 floatToUintBits( double x )
	{
		union
		{
			double f;
			uint64 i;
		} v;
		
		v.f = x;
		return v.i;
	}

	/// Check whether a floating-point value is a NAN.
	inline bool isNAN( float x )
	{
		const uint32 xi = floatToUintBits( x );
		return ( xi & 0x7F800000 ) == 0x7F800000 && ( xi & 0x7FFFFF ) != 0;
	}
	/// Check whether a floating-point value is infinity.
	inline bool isInf( float x )
	{
		return ( floatToUintBits( x ) & 0x7FFFFFFF ) == 0x7F800000;
	}

}
