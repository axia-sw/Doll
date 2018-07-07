#include "../BuildSettings.hpp"
#include "doll/Math/Vector.hpp"
#include "doll/Math/Matrix.hpp"

namespace doll
{

	// Internal namespace
	namespace detail
	{

		/*
			M[ 0]=xx; M[ 1]=yx; M[ 2]=zx; M[ 3]=wx;
			M[ 4]=xy; M[ 5]=yy; M[ 6]=zy; M[ 7]=wy;
			M[ 8]=xz; M[ 9]=yz; M[10]=zz; M[11]=wz;
			M[12]=xw; M[13]=yw; M[14]=zw; M[15]=ww;
		*/

		template< typename tMat >
		Vec3f rowTransform( const tMat &M, const Vec3f &V )
		{
			return
				Vec3f
				(
					M.xx*V.x + M.xy*V.y + M.xz*V.z,
					M.yx*V.x + M.yy*V.y + M.yz*V.z,
					M.zx*V.x + M.zy*V.y + M.zz*V.z
				);
		}
		template< typename tMat >
		Vec3f columnTransform( const tMat &M, const Vec3f &V )
		{
			return
				Vec3f
				(
					M.xx*V.x + M.yx*V.y + M.zx*V.z,
					M.xy*V.x + M.yy*V.y + M.zy*V.z,
					M.xz*V.x + M.yz*V.y + M.zz*V.z
				);
		}

	}

	Vec3f cross( const Vec3f &A, const Vec3f &B )
	{
		return
			Vec3f
			(
				A.y*B.z - A.z*B.y,
				A.z*B.x - A.x*B.z,
				A.x*B.y - A.y*B.x
			);
	}

	static inline F32 negateSigned( F32 fValue, F32 fCheckSign )
	{
		return fCheckSign < 0 ? -fValue : fValue;
	}

	Vec3f lookAt( const Vec3f &Source, const Vec3f &Target )
	{
		const Vec3f P( ( Target - Source ).normalized() );

		const F32 y = atan2f( P.x, P.z );
		const F32 x = negateSigned( atan2f( P.y*cos( y ), abs( P.z ) ), P.z );

		return Vec3f( degrees( x ), degrees( y ), 0 );
	}

	Vec3f vectorLocalToGlobal( const Mat3f &M, const Vec3f &V )
	{
		return detail::rowTransform( M, V );
	}
	Vec3f vectorGlobalToLocal( const Mat3f &M, const Vec3f &V )
	{
		return detail::columnTransform( M, V );
	}

	Vec3f pointLocalToGlobal( const Mat4f &M, const Vec3f &P )
	{
		return detail::rowTransform( M, P ) + Vec3f( M.xw, M.yw, M.zw );
	}
	Vec3f pointGlobalToLocal( const Mat4f &M, const Vec3f &P )
	{
		const Vec3f Q( P.x - M.xw, P.y - M.yw, P.z - M.zw );
		return detail::columnTransform( M, Q );
	}
	Vec3f vectorLocalToGlobal( const Mat4f &M, const Vec3f &V )
	{
		return detail::rowTransform( M, V );
	}
	Vec3f vectorGlobalToLocal( const Mat4f &M, const Vec3f &V )
	{
		return detail::columnTransform( M, V );
	}

	Vec3f moveVector( const Mat3f &ObjectLocalXf, const Vec3f &Distance )
	{
		return detail::rowTransform( ObjectLocalXf, Distance );
	}
	Vec3f moveVector( const Mat4f &ObjectLocalXf, const Vec3f &Distance )
	{
		return
			detail::rowTransform( ObjectLocalXf, Distance ) +
			Vec3f( ObjectLocalXf.xw, ObjectLocalXf.yw, ObjectLocalXf.zw );
	}

	Vec2f rotate( const Vec2f &vec, F32 angle )
	{
		const F32 s = sin( angle );
		const F32 c = cos( angle );

		return Vec2f( vec.x*c - s*vec.y, vec.x*s + c*vec.y );
	}

}
