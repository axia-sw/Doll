#pragma once

#include "../Core/Defs.hpp"

namespace doll {

	// Forward declaration
	struct SRect;

	/*!
	 *	Aspect mode
	 *
	 *	Describes how the aspect ratio should be respected.
	 */
	enum class EAspect
	{
		/// Do not respect the aspect ratio
		None,

		/// Fit to bounds potentially leaving empty space, but with the entire source within the bounds
		Fit,
		/// Fill the bounds leaving no empty space, but potentially leaving part of the source out of bounds
		Fill
	};

	/*!
	 *	SIntVector2
	 *
	 *	Represents a point or a size
	 */
	struct SIntVector2
	{
		union { S32 x, w; };
		union { S32 y, h; };

		inline SIntVector2()
		: x( 0 )
		, y( 0 )
		{
		}
		inline SIntVector2( S32 value )
		: x( value )
		, y( value )
		{
		}
		inline SIntVector2( S32 x, S32 y )
		: x( x )
		, y( y )
		{
		}

		inline S32 min() const { return x < y ? x : y; }
		inline S32 max() const { return x > y ? x : y; }

		inline SIntVector2 &clampMe( const SIntVector2 &min, const SIntVector2 &max )
		{
			if( x < min.x ) { x = min.x; }
			if( y < min.y ) { y = min.y; }

			if( x > max.x ) { x = max.x; }
			if( y > max.y ) { y = max.y; }

			return *this;
		}
		inline SIntVector2 clamped( const SIntVector2 &min, const SIntVector2 &max ) const
		{
			return SIntVector2( *this ).clampMe( min, max );
		}

		SIntVector2 &clampMe( const SRect &bounds );
		inline SIntVector2 clamped( const SRect &bounds ) const
		{
			return SIntVector2( *this ).clampMe( bounds );
		}

		/*! Perform a resize with a given aspect adjustment */
		inline SIntVector2 &aspectResizeMe( const SIntVector2 &Size, double fAspectRatio, EAspect Aspect = EAspect::Fit )
		{
			const double fResX = double( Size.x );
			const double fResY = double( Size.y );

			if( fAspectRatio*fAspectRatio < 0.000001 || !Size.x || !Size.y ) {
				x = Size.x;
				y = Size.y;
				return *this;
			}

			const double fDstAspectRatio = fResX/fResY;

			switch( Aspect )
			{
			case EAspect::None:
				break;

			case EAspect::Fit:
				if( fDstAspectRatio > fAspectRatio ) {
					x = S32( fResY*fAspectRatio + 0.5 );
					y = Size.y;
				} else {
					x = Size.x;
					y = S32( fResX/fAspectRatio + 0.5 );
				}
				return *this;

			case EAspect::Fill:
				if( fDstAspectRatio > fAspectRatio ) {
					x = Size.x;
					y = S32( fResX/fAspectRatio + 0.5 );
				} else {
					x = S32( fResY*fAspectRatio + 0.5 );
					y = Size.y;
				}
				return *this;
			}

			x = Size.x;
			y = Size.y;
			return *this;
		}
		/*! Return a duplicate of this vector resized with the given aspect adjustment */
		inline SIntVector2 aspectResized( const SIntVector2 &Size, double fAspectRatio, EAspect Aspect = EAspect::Fit ) const
		{
			return SIntVector2( *this ).aspectResizeMe( Size, fAspectRatio, Aspect );
		}

		inline bool all() const
		{
			return x != 0 && y != 0;
		}
		inline bool any() const
		{
			return x != 0 || y != 0;
		}

		inline bool operator!() const { return !x && !y; }
		inline bool operator==( const SIntVector2 &v ) const { return x == v.x && y == v.y; }
		inline bool operator!=( const SIntVector2 &v ) const { return x != v.x || y != v.y; }
		inline bool operator<( const SIntVector2 &v ) const { return ( x*x + y*y ) < ( v.x*v.x + v.y*v.y ); }
		inline bool operator>( const SIntVector2 &v ) const { return ( x*x + y*y ) > ( v.x*v.x + v.y*v.y ); }
		inline bool operator<=( const SIntVector2 &v ) const { return ( x*x + y*y ) <= ( v.x*v.x + v.y*v.y ); }
		inline bool operator>=( const SIntVector2 &v ) const { return ( x*x + y*y ) >= ( v.x*v.x + v.y*v.y ); }

		inline SIntVector2 operator+( const SIntVector2 &v ) const { return SIntVector2( x + v.x, y + v.y ); }
		inline SIntVector2 operator-( const SIntVector2 &v ) const { return SIntVector2( x - v.x, y - v.y ); }
		inline SIntVector2 operator*( const SIntVector2 &v ) const { return SIntVector2( x * v.x, y * v.y ); }
		inline SIntVector2 operator/( const SIntVector2 &v ) const { return SIntVector2( x / v.x, y / v.y ); }
		inline SIntVector2 operator%( const SIntVector2 &v ) const { return SIntVector2( x % v.x, y % v.y ); }
		inline SIntVector2 operator|( const SIntVector2 &v ) const { return SIntVector2( x | v.x, y | v.y ); }
		inline SIntVector2 operator&( const SIntVector2 &v ) const { return SIntVector2( x & v.x, y & v.y ); }
		inline SIntVector2 operator^( const SIntVector2 &v ) const { return SIntVector2( x ^ v.x, y ^ v.y ); }
		inline SIntVector2 operator<<( const SIntVector2 &v ) const { return SIntVector2( x << v.x, y << v.y ); }
		inline SIntVector2 operator>>( const SIntVector2 &v ) const { return SIntVector2( x >> v.x, y >> v.y ); }

		inline SIntVector2 operator~() const { return SIntVector2( ~x, ~y ); }
		inline SIntVector2 operator-() const { return SIntVector2( -x, -y ); }

		inline SIntVector2 &operator+=( const SIntVector2 &v ) { *this = *this + v; return *this; }
		inline SIntVector2 &operator-=( const SIntVector2 &v ) { *this = *this - v; return *this; }
		inline SIntVector2 &operator*=( const SIntVector2 &v ) { *this = *this * v; return *this; }
		inline SIntVector2 &operator/=( const SIntVector2 &v ) { *this = *this / v; return *this; }
		inline SIntVector2 &operator%=( const SIntVector2 &v ) { *this = *this % v; return *this; }
		inline SIntVector2 &operator|=( const SIntVector2 &v ) { *this = *this | v; return *this; }
		inline SIntVector2 &operator&=( const SIntVector2 &v ) { *this = *this & v; return *this; }
		inline SIntVector2 &operator^=( const SIntVector2 &v ) { *this = *this ^ v; return *this; }
		inline SIntVector2 &operator<<=( const SIntVector2 &v ) { *this = *this << v; return *this; }
		inline SIntVector2 &operator>>=( const SIntVector2 &v ) { *this = *this >> v; return *this; }
	};
	inline bool operator==( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) == b; }
	inline bool operator!=( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) != b; }
	inline bool operator<( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) < b; }
	inline bool operator>( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) > b; }
	inline bool operator<=( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) <= b; }
	inline bool operator>=( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) >= b; }

	inline SIntVector2 operator+( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) + b; }
	inline SIntVector2 operator-( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) - b; }
	inline SIntVector2 operator*( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) * b; }
	inline SIntVector2 operator/( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) / b; }
	inline SIntVector2 operator%( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) % b; }
	inline SIntVector2 operator|( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) | b; }
	inline SIntVector2 operator&( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) & b; }
	inline SIntVector2 operator^( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) ^ b; }
	inline SIntVector2 operator<<( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) << b; }
	inline SIntVector2 operator>>( S32 a, const SIntVector2 &b ) { return SIntVector2( a ) >> b; }

}
