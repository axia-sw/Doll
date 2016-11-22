#pragma once

#include "../Core/Defs.hpp"

#include "Basic.hpp"
#include "Const.hpp"

namespace doll
{

	class Vec2f;
	class Vec3f;
	class Vec4f;
	class Mat3f;
	class Mat4f;

	// Perform a dot-product on two vectors
	F32 dot( const Vec2f &A, const Vec2f &B );
	F32 dot( const Vec3f &A, const Vec3f &B );
	F32 dot( const Vec4f &A, const Vec4f &B );

	// Perform a cross-product on two vectors
	Vec3f cross( const Vec3f &A, const Vec3f &B );

	// Create a vector pointing at the `Target` point from the given `Source`
	// point. `Source` and `Target` must be in the same space (e.g., world-
	// space, or the local-space of the same object). The returned direction
	// vector will be in the same space as the inputs.
	Vec3f lookAt( const Vec3f &Source, const Vec3f &Target );

	// Convert a point that is in local-space (relative to the given object's
	// world-space transform) to global-space
	Vec3f pointLocalToGlobal( const Mat4f &ObjectWorldXf, const Vec3f &LocalPoint );
	// Convert a point that is in global-space to local-space (relative to the
	// given object's world-space transform)
	Vec3f pointGlobalToLocal( const Mat4f &ObjectWorldXf, const Vec3f &GlobalPoint );
	// Convert a vector that is in local-space (relative to the given object's
	// world-space transform) to global-space
	Vec3f vectorLocalToGlobal( const Mat3f &ObjectWorldXf, const Vec3f &LocalDirection );
	Vec3f vectorLocalToGlobal( const Mat4f &ObjectWorldXf, const Vec3f &LocalDirection );
	// Convert a vector that is in global-space to local-space (relative to the
	// given object's world-space transform)
	Vec3f vectorGlobalToLocal( const Mat3f &ObjectWorldXf, const Vec3f &GlobalDirection );
	Vec3f vectorGlobalToLocal( const Mat4f &ObjectWorldXf, const Vec3f &GlobalDirection );

	// Create a vector that represents a distance to have been moved based on a
	// rotation matrix
	Vec3f moveVector( const Mat3f &ObjectLocalXf, const Vec3f &Distance );
	Vec3f moveVector( const Mat4f &ObjectLocalXf, const Vec3f &Distance );

	// Rotate a 2D point by an angle
	Vec2f rotate( const Vec2f &vec, F32 angle );

	// 2D vector, or coordinate
	class Vec2f
	{
	public:
		static const unsigned kRows = 1;
		static const unsigned kColumns = 2;
		static const unsigned kDimensions = kRows*kColumns;
		typedef F32 ElementType;

		F32 x, y;

		inline Vec2f( F32 v = 0 )
		: x( v )
		, y( v )
		{
		}
		inline Vec2f( F32 x, F32 y )
		: x( x )
		, y( y )
		{
		}
		inline Vec2f( const Vec2f &v )
		: x( v.x )
		, y( v.y )
		{
		}
		explicit Vec2f( const Vec3f &v );
		explicit Vec2f( const Vec4f &v );

		inline Vec2f &operator=( float v )
		{
			x = v;
			y = v;

			return *this;
		}
		inline Vec2f &operator=( const Vec2f &v )
		{
			x = v.x;
			y = v.y;

			return *this;
		}
		Vec2f &operator=( const Vec3f &v );
		Vec2f &operator=( const Vec4f &v );

		inline F32 *ptr()
		{
			return reinterpret_cast< F32 * >( this );
		}
		inline const F32 *ptr() const
		{
			return reinterpret_cast< const F32 * >( this );
		}

		inline size_t num() const
		{
			return kDimensions;
		}

		inline F32 &operator[]( size_t i )
		{
			AX_ASSERT( i < 2 && "Index out of bounds" );
			return reinterpret_cast< F32 * >( this )[ i ];
		}
		inline const F32 &operator[]( size_t i ) const
		{
			AX_ASSERT( i < 2 && "Index out of bounds" );
			return reinterpret_cast< const F32 * >( this )[ i ];
		}

		inline bool operator==( const Vec2f &V ) const
		{
			return
				x > V.x - DOLL_EPSILON && x < V.x + DOLL_EPSILON &&
				y > V.y - DOLL_EPSILON && y < V.y + DOLL_EPSILON;
		}
		inline bool operator!=( const Vec2f &V ) const
		{
			return
				x <= V.x - DOLL_EPSILON || x >= V.x + DOLL_EPSILON ||
				y <= V.y - DOLL_EPSILON || y >= V.y + DOLL_EPSILON;
		}

		inline Vec2f &operator+=( const Vec2f &V )
		{
			x += V.x;
			y += V.y;

			return *this;
		}
		inline Vec2f &operator-=( const Vec2f &V )
		{
			x -= V.x;
			y -= V.y;

			return *this;
		}
		inline Vec2f &operator*=( const Vec2f &V )
		{
			x *= V.x;
			y *= V.y;

			return *this;
		}
		inline Vec2f &operator/=( const Vec2f &V )
		{
			x /= V.x;
			y /= V.y;

			return *this;
		}

		inline Vec2f operator+( const Vec2f &V ) const
		{
			return Vec2f( x + V.x, y + V.y );
		}
		inline Vec2f operator-( const Vec2f &V ) const
		{
			return Vec2f( x - V.x, y - V.y );
		}
		inline Vec2f operator*( const Vec2f &V ) const
		{
			return Vec2f( x*V.x, y*V.y );
		}
		inline Vec2f operator/( const Vec2f &V ) const
		{
			return Vec2f( x/V.x, y/V.y );
		}

		inline F32 lengthSq() const
		{
			return dot( *this, *this );
		}
		inline F32 length() const
		{
			return sqrt( lengthSq() );
		}

		inline Vec2f normalized() const
		{
			const F32 fInvMag = 1.0f/lengthSq();
			return *this*fInvMag;
		}

		inline Vec2f snap() const
		{
			return Vec2f( floorf( x + 0.5f ), floorf( y + 0.5f ) );
		}
	};

	// 3D vector, coordinate, or color
	class Vec3f
	{
	public:
		static const unsigned kRows = 1;
		static const unsigned kColumns = 3;
		static const unsigned kDimensions = kRows*kColumns;
		typedef F32 ElementType;

		F32 x, y, z;

		inline Vec3f( F32 v = 0 )
		: x( v )
		, y( v )
		, z( v )
		{
		}
		inline Vec3f( F32 x, F32 y, F32 z = 0 )
		: x( x )
		, y( y )
		, z( z )
		{
		}
		inline Vec3f( const Vec2f &xy, F32 z = 0 )
		: x( xy.x )
		, y( xy.y )
		, z( z )
		{
		}
		inline Vec3f( F32 x, const Vec2f &yz )
		: x( x )
		, y( yz.x )
		, z( yz.y )
		{
		}
		inline Vec3f( const Vec3f &v )
		: x( v.x )
		, y( v.y )
		, z( v.z )
		{
		}
		explicit Vec3f( const Vec4f &v );

		inline Vec3f &operator=( F32 v )
		{
			x = v;
			y = v;
			z = v;

			return *this;
		}
		inline Vec3f &operator=( const Vec2f &v )
		{
			x = v.x;
			y = v.y;
			z = 0.0f;

			return *this;
		}
		inline Vec3f &operator=( const Vec3f &v )
		{
			x = v.x;
			y = v.y;
			z = v.z;

			return *this;
		}
		Vec3f &operator=( const Vec4f &v );

		inline F32 *ptr()
		{
			return reinterpret_cast< F32 * >( this );
		}
		inline const F32 *ptr() const
		{
			return reinterpret_cast< const F32 * >( this );
		}

		inline size_t num() const
		{
			return kDimensions;
		}

		inline F32 &operator[]( size_t i )
		{
			AX_ASSERT( i < 3 && "Index out of bounds" );
			return reinterpret_cast< F32 * >( this )[ i ];
		}
		inline const F32 &operator[]( size_t i ) const
		{
			AX_ASSERT( i < 3 && "Index out of bounds" );
			return reinterpret_cast< const F32 * >( this )[ i ];
		}

		inline bool operator==( const Vec3f &V ) const
		{
			return
				x > V.x - DOLL_EPSILON && x < V.x + DOLL_EPSILON &&
				y > V.y - DOLL_EPSILON && y < V.y + DOLL_EPSILON &&
				z > V.z - DOLL_EPSILON && z < V.z + DOLL_EPSILON;
		}
		inline bool operator!=( const Vec3f &V ) const
		{
			return
				x <= V.x - DOLL_EPSILON || x >= V.x + DOLL_EPSILON ||
				y <= V.y - DOLL_EPSILON || y >= V.y + DOLL_EPSILON ||
				z <= V.z - DOLL_EPSILON || z >= V.z + DOLL_EPSILON;
		}

		inline Vec3f &operator+=( const Vec3f &V )
		{
			x += V.x;
			y += V.y;
			z += V.z;

			return *this;
		}
		inline Vec3f &operator-=( const Vec3f &V )
		{
			x -= V.x;
			y -= V.y;
			z -= V.z;

			return *this;
		}
		inline Vec3f &operator*=( const Vec3f &V )
		{
			x *= V.x;
			y *= V.y;
			z *= V.z;

			return *this;
		}
		inline Vec3f &operator/=( const Vec3f &V )
		{
			x /= V.x;
			y /= V.y;
			z /= V.z;

			return *this;
		}

		inline Vec3f operator+( const Vec3f &V ) const
		{
			return Vec3f( x + V.x, y + V.y, z + V.z );
		}
		inline Vec3f operator-( const Vec3f &V ) const
		{
			return Vec3f( x - V.x, y - V.y, z - V.z );
		}
		inline Vec3f operator*( const Vec3f &V ) const
		{
			return Vec3f( x*V.x, y*V.y, z*V.z );
		}
		inline Vec3f operator/( const Vec3f &V ) const
		{
			return Vec3f( x/V.x, y/V.y, z/V.z );
		}

		inline F32 lengthSq() const
		{
			return dot( *this, *this );
		}
		inline F32 length() const
		{
			return sqrt( lengthSq() );
		}

		inline Vec3f normalized() const
		{
			const F32 fInvMag = 1.0f/lengthSq();
			return *this*fInvMag;
		}
	};

	// 4D vector, coordinate, or color
	class Vec4f
	{
	public:
		static const unsigned kRows = 1;
		static const unsigned kColumns = 4;
		static const unsigned kDimensions = kRows*kColumns;
		typedef F32 ElementType;

		F32 x, y, z, w;

		inline Vec4f( F32 v = 0 )
		: x( v )
		, y( v )
		, z( v )
		, w( v )
		{
		}
		inline Vec4f( F32 x, F32 y, F32 z = 0, F32 w = 0 )
		: x( x )
		, y( y )
		, z( z )
		, w( w )
		{
		}
		inline Vec4f( const Vec2f &xy, F32 z = 0, F32 w = 0 )
			: x( xy.x )
			, y( xy.y )
			, z( z )
			, w( w )
		{
		}
		inline Vec4f( F32 x, const Vec2f &yz, F32 w = 0 )
		: x( x )
		, y( yz.x )
		, z( yz.y )
		, w( w )
		{
		}
		inline Vec4f( F32 x, F32 y, const Vec2f &zw )
		: x( x )
		, y( y )
		, z( zw.x )
		, w( zw.y )
		{
		}
		inline Vec4f( const Vec3f &xyz, F32 w = 0 )
		: x( xyz.x )
		, y( xyz.y )
		, z( xyz.z )
		, w( w )
		{
		}
		inline Vec4f( F32 x, const Vec3f &yzw )
		: x( x )
		, y( yzw.x )
		, z( yzw.y )
		, w( yzw.z )
		{
		}
		inline Vec4f( const Vec4f &v )
		: x( v.x )
		, y( v.y )
		, z( v.z )
		, w( v.w )
		{
		}

		inline Vec4f &operator=( F32 v )
		{
			x = v;
			y = v;
			z = v;
			w = v;

			return *this;
		}
		inline Vec4f &operator=( const Vec2f &v )
		{
			x = v.x;
			y = v.y;
			z = 0;
			w = 0;

			return *this;
		}
		inline Vec4f &operator=( const Vec3f &v )
		{
			x = v.x;
			y = v.y;
			z = v.z;
			w = 0;

			return *this;
		}
		inline Vec4f &operator=( const Vec4f &v )
		{
			x = v.x;
			y = v.y;
			z = v.z;
			w = v.w;

			return *this;
		}

		inline F32 *ptr()
		{
			return reinterpret_cast< F32 * >( this );
		}
		inline const F32 *ptr() const
		{
			return reinterpret_cast< const F32 * >( this );
		}

		inline size_t num() const
		{
			return kDimensions;
		}

		inline F32 &operator[]( size_t i )
		{
			AX_ASSERT( i < 4 && "Index out of bounds" );
			return reinterpret_cast< F32 * >( this )[ i ];
		}
		inline const F32 &operator[]( size_t i ) const
		{
			AX_ASSERT( i < 4 && "Index out of bounds" );
			return reinterpret_cast< const F32 * >( this )[ i ];
		}

		inline bool operator==( const Vec4f &V ) const
		{
			return
				x > V.x - DOLL_EPSILON && x < V.x + DOLL_EPSILON &&
				y > V.y - DOLL_EPSILON && y < V.y + DOLL_EPSILON &&
				z > V.z - DOLL_EPSILON && z < V.z + DOLL_EPSILON &&
				w > V.w - DOLL_EPSILON && w < V.w + DOLL_EPSILON;
		}
		inline bool operator!=( const Vec4f &V ) const
		{
			return
				x <= V.x - DOLL_EPSILON || x >= V.x + DOLL_EPSILON ||
				y <= V.y - DOLL_EPSILON || y >= V.y + DOLL_EPSILON ||
				z <= V.z - DOLL_EPSILON || z >= V.z + DOLL_EPSILON ||
				w <= V.w - DOLL_EPSILON || w >= V.w + DOLL_EPSILON;
		}

		inline Vec4f &operator+=( const Vec4f &V )
		{
			x += V.x;
			y += V.y;
			z += V.z;
			w += V.w;

			return *this;
		}
		inline Vec4f &operator-=( const Vec4f &V )
		{
			x -= V.x;
			y -= V.y;
			z -= V.z;
			w -= V.w;

			return *this;
		}
		inline Vec4f &operator*=( const Vec4f &V )
		{
			x *= V.x;
			y *= V.y;
			z *= V.z;
			w *= V.w;

			return *this;
		}
		inline Vec4f &operator/=( const Vec4f &V )
		{
			x /= V.x;
			y /= V.y;
			z /= V.z;
			w /= V.w;

			return *this;
		}

		inline Vec4f operator+( const Vec4f &V ) const
		{
			return Vec4f( x + V.x, y + V.y, z + V.z, w + V.w );
		}
		inline Vec4f operator-( const Vec4f &V ) const
		{
			return Vec4f( x - V.x, y - V.y, z - V.z, w - V.w );
		}
		inline Vec4f operator*( const Vec4f &V ) const
		{
			return Vec4f( x*V.x, y*V.y, z*V.z, w*V.w );
		}
		inline Vec4f operator/( const Vec4f &V ) const
		{
			return Vec4f( x/V.x, y/V.y, z/V.z, w/V.w );
		}

		inline F32 lengthSq() const
		{
			return dot( *this, *this );
		}
		inline F32 length() const
		{
			return sqrt( lengthSq() );
		}

		inline Vec4f normalized() const
		{
			const F32 fInvMag = 1.0f/lengthSq();
			return *this*fInvMag;
		}
	};

	//--------------------------------------------------------------------//

	inline Vec2f::Vec2f( const Vec3f &v )
	: x( v.x )
	, y( v.y )
	{
	}
	inline Vec2f::Vec2f( const Vec4f &v )
	: x( v.x )
	, y( v.y )
	{
	}
	inline Vec2f &Vec2f::operator=( const Vec3f &v )
	{
		x = v.x;
		y = v.y;

		return *this;
	}
	inline Vec2f &Vec2f::operator=( const Vec4f &v )
	{
		x = v.x;
		y = v.y;

		return *this;
	}

	inline Vec3f::Vec3f( const Vec4f &v )
	: x( v.x )
	, y( v.y )
	, z( v.z )
	{
	}
	inline Vec3f &Vec3f::operator=( const Vec4f &v )
	{
		x = v.x;
		y = v.y;
		z = v.z;

		return *this;
	}

	//--------------------------------------------------------------------//

	inline F32 dot( const Vec2f &A, const Vec2f &B )
	{
		return A.x*B.x + A.y*B.y;
	}
	inline F32 dot( const Vec3f &A, const Vec3f &B )
	{
		return A.x*B.x + A.y*B.y + A.z*B.z;
	}
	inline F32 dot( const Vec4f &A, const Vec4f &B )
	{
		return A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w;
	}

}
