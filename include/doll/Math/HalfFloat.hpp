#pragma once

#include "../Core/Defs.hpp"
#include "Bits.hpp"

namespace doll {

	/// Represents a half-precision (16-bit) floating-point value.
	struct F16
	{
		F16();
		F16( float f );
		F16( const F16 &x );
		
		F16 &operator=( float f );
		operator float() const;
	
	private:
		U16 m_uBits;
	};
	
	inline F16::F16()
	: m_uBits( 0 )
	{
	}
	inline F16::F16( float f )
	{
		*this = f;
	}
	inline F16::F16( const F16 &x )
	: m_uBits( x.m_uBits )
	{
	}

	/// Convert a float value to a half-float value.
	inline F16 &F16::operator=( float f )
	{
		U32 fbits = floatToUintBits( f );
		U32 sign = ( fbits>>16 ) & 0x8000;
		fbits &= 0x7FFFFFFF;

		// check if the number is too large
		if( fbits > 0x477FE000 ) {
			// handle nan
			if( isNAN( f ) ) {
				// 16-bit nan
				m_uBits = sign | 0x7FFF;
				return *this;
			}

			// clamp to infinity
			m_uBits = sign | 0x7C00;
			return *this;
		}

		// check if the number is too small to represent normalized
		if( fbits < 0x38800000 ) {
			// convert to a denormalized value
			const U32 shift = 113 - ( fbits>>23 );
			fbits = ( 0x800000 | ( fbits & 0x7FFFFF ) )>>shift;
		} else {
			// exponent needs to be biased
			fbits += 0xC8000000;
		}

		U32 r = ( ( fbits + 0x0FFF + ( ( fbits>>13 ) & 1 ) )>>13 ) & 0x7FFF;
		m_uBits = ( uint16 )( sign | r );
		return *this;
	}
	/// Convert a half-float value to a float value.
	inline F16::operator float() const
	{
		U32 m = ( U32 )( m_uBits & 0x03FF );
		U32 e = ( U32 )( m_uBits & 0x7C00 );

		// check for INF/NAN
		if( e == 0x7C00 ) {
			e = 0x8F;
		} else if( e != 0 ) {
			// normalized
			e = ( U32 )( ( m_uBits >> 10 ) & 0x1F );
		} else if( m != 0 ) {
			// denormalized
			e = 1;
			
			do
			{
				--e;
				m <<= 1;
			} while( ( m & 0x0400 ) == 0 );
			
			m &= 0x03FF;
		} else {
			e = ( U32 )-112;
		}

		const U32 r = ( ( m_uBits & 0x8000 ) << 16 ) | ( ( e + 112 ) << 23 ) | ( m << 13 );
		return uintBitsToFloat( r );
	}

}
