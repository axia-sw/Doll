#include "../BuildSettings.hpp"
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
		inline tMat &loadRotation( tMat &O, const Vec3f &I )
		{
			F32 sx, sy, sz, cx, cy, cz;

			sx = sin( I.x ); sy = sin( I.y ); sz = sin( I.z );
			cx = cos( I.x ); cy = cos( I.y ); cz = cos( I.z );

			const F32 cz_zx = cz*-sx;
			const F32 zy = -sy;

			O.xx = cz*cy;            O.xy = sz*cy + cz_zx*sy; O.xz = cx*sy;
			O.yx =-sz*cx;            O.yy = cz*cx;            O.yz = sx;
			O.zx = cz*zy + sz*sx*cy; O.zy = sz*zy + cz_zx*cy; O.zz = cx*cy;

			return O;
		}

		template< typename tMat >
		inline tMat &applyXRotation( tMat &O, F32 I )
		{
			const F32 s = sin( I );
			const F32 c = cos( I );
			const F32 z = -s;

			F32 t;

			t = O.yx;
			O.yx = t*c + O.zx*s;
			O.zx = t*z + O.zx*c;

			t = O.yy;
			O.yy = t*c + O.zy*s;
			O.zy = t*z + O.zy*c;

			t = O.yz;
			O.yz = t*c + O.zz*s;
			O.zz = t*z + O.zz*c;

			return O;
		}
		template< typename tMat >
		inline tMat &applyYRotation( tMat &O, F32 I )
		{
			const F32 s = sin( I );
			const F32 c = cos( I );
			const F32 z = -s;

			F32 t;

			t = O.xx;
			O.xx = t*c + O.zx*s;
			O.zx = t*z + O.zx*c;

			t = O.xy;
			O.xy = t*c + O.zy*s;
			O.zy = t*z + O.zy*c;

			t = O.xz;
			O.xz = t*c + O.zz*s;
			O.zz = t*z + O.zz*c;

			return O;
		}
		template< typename tMat >
		inline tMat &applyZRotation( tMat &O, F32 I )
		{
			const F32 s = sin( I );
			const F32 c = cos( I );
			const F32 z = -s;

			F32 t;

			t = O.xx;
			O.xx = t*c + O.yx*s;
			O.yx = t*z + O.yx*c;

			t = O.xy;
			O.xy = t*c + O.yy*s;
			O.yy = t*z + O.yy*c;

			t = O.xz;
			O.xz = t*c + O.yz*s;
			O.yz = t*z + O.yz*c;

			return O;
		}

		template< typename tMat >
		inline tMat &applyScaling( tMat &O, const Vec3f &Scaling )
		{
			O.xx *= Scaling.x; O.yx *= Scaling.x; O.zx *= Scaling.x;
			O.xy *= Scaling.y; O.yy *= Scaling.y; O.zy *= Scaling.y;
			O.xz *= Scaling.z; O.yz *= Scaling.z; O.zz *= Scaling.z;

			return O;
		}

	}




	/*
	========================================================================

		MATRIX 3x3

	========================================================================
	*/

	Mat3f &Mat3f::loadIdentity()
	{
		*this = Mat3f();
		return *this;
	}
	Mat3f &Mat3f::loadTranspose()
	{
		*this = Mat3f( rowX(), rowY(), rowZ() );
		return *this;
	}
	Mat3f &Mat3f::loadTranspose( const Mat3f &Other )
	{
		*this = Mat3f( Other.rowX(), Other.rowY(), Other.rowZ() );
		return *this;
	}

	Mat3f &Mat3f::loadRotation( const Vec3f &AnglesDegrees )
	{
		return detail::loadRotation( *this, AnglesDegrees );
	}
	Mat3f &Mat3f::loadScaling( const Vec3f &Scaling )
	{
		const F32 a = Scaling.x;
		const F32 b = Scaling.y;
		const F32 c = Scaling.z;

		xx = a; yx = 0; zx = 0;
		xy = 0; yy = b; zy = 0;
		xz = 0; yz = 0; zz = c;

		return *this;
	}

	Mat3f &Mat3f::applyRotation( const Vec3f &AnglesDegrees )
	{
		return
			applyZRotation( AnglesDegrees.z ).
			applyXRotation( AnglesDegrees.x ).
			applyYRotation( AnglesDegrees.y );
	}
	Mat3f &Mat3f::applyXRotation( F32 fXAngleDegrees )
	{
		return detail::applyXRotation( *this, fXAngleDegrees );
	}
	Mat3f &Mat3f::applyYRotation( F32 fYAngleDegrees )
	{
		return detail::applyYRotation( *this, fYAngleDegrees );
	}
	Mat3f &Mat3f::applyZRotation( F32 fZAngleDegrees )
	{
		return detail::applyZRotation( *this, fZAngleDegrees );
	}
	Mat3f &Mat3f::applyScaling( const Vec3f &Scaling )
	{
		return detail::applyScaling( *this, Scaling );
	}

	Mat3f &Mat3f::loadMultiply( const Mat3f &A, const Mat3f &B )
	{
		/*
			The following code is equivalent to:

				xx = dot( A.columnX(), B.rowX() );
				xy = dot( A.columnX(), B.rowY() );
				xz = dot( A.columnX(), B.rowZ() );

				yx = dot( A.columnY(), B.rowX() );
				yy = dot( A.columnY(), B.rowY() );
				yz = dot( A.columnY(), B.rowZ() );

				zx = dot( A.columnZ(), B.rowX() );
				zy = dot( A.columnZ(), B.rowY() );
				zw = dot( A.columnZ(), B.rowZ() );
		*/

		xx = A.xx*B.xx + A.xy*B.yx + A.xz*B.zx;
		xy = A.xx*B.xy + A.xy*B.yy + A.xz*B.zy;
		xz = A.xx*B.xz + A.xy*B.yz + A.xz*B.zz;

		yx = A.yx*B.xx + A.yy*B.yx + A.yz*B.zx;
		yy = A.yx*B.xy + A.yy*B.yy + A.yz*B.zy;
		yz = A.yx*B.xz + A.yy*B.yz + A.yz*B.zz;

		zx = A.zx*B.xx + A.zy*B.yx + A.zz*B.zx;
		zy = A.zx*B.xy + A.zy*B.yy + A.zz*B.zy;
		zz = A.zx*B.xz + A.zy*B.yz + A.zz*B.zz;

		return *this;
	}

	F32 Mat3f::determinant() const
	{
		/*
			The following code is equivalent to:

				return dot( rowX(), cross( rowY(), rowZ() ) );
		*/
		return
			xx*( yy*zz - zy*yz ) +
			yx*( zy*xz - xy*zz ) +
			zx*( xy*yz - yy*xz );
	}




	/*
	========================================================================

		MATRIX 4x4

	========================================================================
	*/

	Mat4f &Mat4f::loadIdentity()
	{
		*this = Mat4f();
		return *this;
	}
	Mat4f &Mat4f::loadTranspose()
	{
		*this = Mat4f( rowX(), rowY(), rowZ(), rowW() );
		return *this;
	}
	Mat4f &Mat4f::loadTranspose( const Mat4f &Other )
	{
		*this = Mat4f( Other.rowX(), Other.rowY(), Other.rowZ(), Other.rowW() );
		return *this;
	}

	Mat4f &Mat4f::loadPerspProj( F32 fFovDeg, F32 fAspect, F32 fZNear, F32 fZFar )
	{
		const F32 zn = fZNear;
		const F32 zf = fZFar;

		F32 a, b, c, d;

		b = 1.0f/tan( 0.5f*fFovDeg );
		a = b/fAspect;

		c = zf/( zf - zn );
		d = -zn*zf/( zf - zn );

		xx = a; yx = 0; zx = 0; wx = 0;
		xy = 0; yy = b; zy = 0; wy = 0;
		xz = 0; yz = 0; zz = c; wz = 1;
		xw = 0; yw = 0; zw = d; ww = 0;

		return *this;
	}
	Mat4f &Mat4f::loadOrthoProj( F32 fLeft, F32 fRight, F32 fBottom, F32 fTop, F32 fZNear, F32 fZFar )
	{
		const F32  l = fLeft;
		const F32  r = fRight;
		const F32  b = fBottom;
		const F32  t = fTop;
		const F32 zn = fZNear;
		const F32 zf = fZFar;

		F32 A, B, C, D, E, F;

		A =      2.0f/( r  -  l );
		B = ( l + r )/( l  -  r );
		C =      2.0f/( t  -  b );
		D = ( t + b )/( b  -  t );
		E =      1.0f/( zf - zn );
		F =        zn/( zn - zf );

		xx = A; yx = 0; zx = 0; wx = 0;
		xy = 0; yy = C; zy = 0; wy = 0;
		xz = 0; yz = 0; zz = E; wz = 0;
		xw = B; yw = D; zw = F; ww = 1;

		return *this;
	}

	bool Mat4f::isProjPersp() const
	{
		return wz >= 1 - DOLL_EPSILON && wz <= 1 + DOLL_EPSILON;
	}
	bool Mat4f::isProjOrtho() const
	{
		return wz >= 0 - DOLL_EPSILON && wz <= 0 + DOLL_EPSILON;
	}
	float Mat4f::getPerspProjFov() const
	{
		// avoid divide-by-zero
		if( yy >= 0 - DOLL_EPSILON && yy <= 0 + DOLL_EPSILON ) {
			return 0;
		}

		return degrees( atanf( 1.0f/yy ) )*2;
	}
	float Mat4f::getPerspZNear() const
	{
		return -zw/zz;
	}
	float Mat4f::getPerspZFar() const
	{
		return zw/( 1 - zz );
	}

	Mat4f &Mat4f::loadTranslation( const Vec3f &Translation )
	{
		*this = Mat4f();
		return setTranslation( Translation );
	}
	Mat4f &Mat4f::loadRotation( const Vec3f &AnglesDegrees )
	{
		detail::loadRotation( *this, AnglesDegrees );

		wx = 0;
		wy = 0;
		wz = 0;
		xw = 0;
		yw = 0;
		zw = 0;
		ww = 1;

		return *this;
	}
	Mat4f &Mat4f::loadScaling( const Vec3f &Scaling )
	{
		const F32 a = Scaling.x;
		const F32 b = Scaling.y;
		const F32 c = Scaling.z;

		xx = a; yx = 0; zx = 0;
		xy = 0; yy = b; zy = 0;
		xz = 0; yz = 0; zz = c;

		return *this;
	}

	Mat4f &Mat4f::loadAffine( const Mat3f &Rotation, const Vec3f &Position )
	{
		xx = Rotation.xx; yx = Rotation.yx; zx = Rotation.zx; wx = 0;
		xy = Rotation.xy; yy = Rotation.yy; zy = Rotation.zy; wy = 0;
		xz = Rotation.xz; yz = Rotation.yz; zz = Rotation.zz; wz = 0;
		xw = Position.x; yw = Position.y; zw = Position.z; ww = 1;

		return *this;
	}

	Mat4f &Mat4f::setTranslation( const Vec3f &Translation )
	{
		xw = Translation.x;
		yw = Translation.y;
		zw = Translation.z;

		return *this;
	}

	Mat4f &Mat4f::applyTranslation( const Vec3f &Translation )
	{
		const Vec3f &V = Translation;

		xw += xx*V.x + yx*V.y + zx*V.z;
		yw += xy*V.x + yy*V.y + zy*V.z;
		zw += xz*V.x + yz*V.y + zz*V.z;

		return *this;
	}
	Mat4f &Mat4f::applyRotation( const Vec3f &AnglesDegrees )
	{
		return
			applyZRotation( AnglesDegrees.z ).
			applyXRotation( AnglesDegrees.x ).
			applyYRotation( AnglesDegrees.y );
	}
	Mat4f &Mat4f::applyXRotation( F32 fXAngleDegrees )
	{
		return detail::applyXRotation( *this, fXAngleDegrees );
	}
	Mat4f &Mat4f::applyYRotation( F32 fYAngleDegrees )
	{
		return detail::applyYRotation( *this, fYAngleDegrees );
	}
	Mat4f &Mat4f::applyZRotation( F32 fZAngleDegrees )
	{
		return detail::applyZRotation( *this, fZAngleDegrees );
	}
	Mat4f &Mat4f::applyScaling( const Vec3f &Scaling )
	{
		return detail::applyScaling( *this, Scaling );
	}

	Mat4f &Mat4f::loadAffineInverse( const Mat4f &M )
	{
		const F32 x = -( M.xw*M.xx + M.yw*M.yx + M.zw*M.zx );
		const F32 y = -( M.xw*M.xy + M.yw*M.yy + M.zw*M.zy );
		const F32 z = -( M.xw*M.xz + M.yw*M.yz + M.zw*M.zz );

		xx = M.xx; yx = M.xy; zx = M.xz; wx = 0;
		xy = M.yx; yy = M.yy; zy = M.yz; wy = 0;
		xz = M.zx; yz = M.zy; zz = M.zz; wz = 0;
		xw = x; yw = y; zw = z; ww = 1;

		return *this;
	}
	Mat4f &Mat4f::loadAffineMultiply( const Mat4f &A, const Mat4f &B )
	{
		xx = A.xx*B.xx + A.xy*B.yx + A.xz*B.zx;
		xy = A.xx*B.xy + A.xy*B.yy + A.xz*B.zy;
		xz = A.xx*B.xz + A.xy*B.yz + A.xz*B.zz;
		xw = A.xx*B.xw + A.xy*B.yw + A.xz*B.zw + A.xw;

		yx = A.yx*B.xx + A.yy*B.yx + A.yz*B.zx;
		yy = A.yx*B.xy + A.yy*B.yy + A.yz*B.zy;
		yz = A.yx*B.xz + A.yy*B.yz + A.yz*B.zz;
		yw = A.yx*B.xw + A.yy*B.yw + A.yz*B.zw + A.yw;

		zx = A.zx*B.xx + A.zy*B.yx + A.zz*B.zx;
		zy = A.zx*B.xy + A.zy*B.yy + A.zz*B.zy;
		zz = A.zx*B.xz + A.zy*B.yz + A.zz*B.zz;
		zw = A.zx*B.xw + A.zy*B.yw + A.zz*B.zw + A.zw;

		wx = 0;
		wy = 0;
		wz = 0;
		ww = 1;

		return *this;
	}
	Mat4f &Mat4f::loadMultiply( const Mat4f &A, const Mat4f &B )
	{
		xx = A.xx*B.xx + A.xy*B.yx + A.xz*B.zx + A.xw*B.wx;
		xy = A.xx*B.xy + A.xy*B.yy + A.xz*B.zy + A.xw*B.wy;
		xz = A.xx*B.xz + A.xy*B.yz + A.xz*B.zz + A.xw*B.wz;
		xw = A.xx*B.xw + A.xy*B.yw + A.xz*B.zw + A.xw*B.ww;

		yx = A.yx*B.xx + A.yy*B.yx + A.yz*B.zx + A.yw*B.wx;
		yy = A.yx*B.xy + A.yy*B.yy + A.yz*B.zy + A.yw*B.wy;
		yz = A.yx*B.xz + A.yy*B.yz + A.yz*B.zz + A.yw*B.wz;
		yw = A.yx*B.xw + A.yy*B.yw + A.yz*B.zw + A.yw*B.ww;

		zx = A.zx*B.xx + A.zy*B.yx + A.zz*B.zx + A.zw*B.wx;
		zy = A.zx*B.xy + A.zy*B.yy + A.zz*B.zy + A.zw*B.wy;
		zz = A.zx*B.xz + A.zy*B.yz + A.zz*B.zz + A.zw*B.wz;
		zw = A.zx*B.xw + A.zy*B.yw + A.zz*B.zw + A.zw*B.ww;

		wx = A.wx*B.xx + A.wy*B.yx + A.wz*B.zx + A.ww*B.wx;
		wy = A.wx*B.xy + A.wy*B.yy + A.wz*B.zy + A.ww*B.wy;
		wz = A.wx*B.xz + A.wy*B.yz + A.wz*B.zz + A.ww*B.wz;
		ww = A.wx*B.xw + A.wy*B.yw + A.wz*B.zw + A.ww*B.ww;

		return *this;
	}

	F32 Mat4f::determinant() const
	{
		return
			wx*zy*yz*xw - zx*wy*yz*xw - wx*yy*zz*xw + yx*wy*zz*xw +
			zx*yy*wz*xw - yx*zy*wz*xw - wx*zy*xz*yw + zx*wy*xz*yw +
			wx*xy*zz*yw - xx*wy*zz*yw - zx*xy*wz*yw + xx*zy*wz*yw +
			wx*yy*xz*zw - yx*wy*xz*zw - wx*xy*yz*zw + xx*wy*yz*zw +
			yx*xy*wz*zw - xx*yy*wz*zw - zx*yy*xz*ww + yx*zy*xz*ww +
			zx*xy*yz*ww - xx*zy*yz*ww - yx*xy*zz*ww + xx*yy*zz*ww;
	}

}
