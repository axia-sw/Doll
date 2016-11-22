#pragma once

#include "../Core/Defs.hpp"
#include "Vector.hpp"

namespace doll
{

	class Mat3f;
	class Mat4f;

	// 3x3 matrix (with column-major memory layout)
	class Mat3f
	{
	public:
		static const unsigned kRows = 3;
		static const unsigned kColumns = 3;
		static const unsigned kDimensions = kRows*kColumns;
		typedef F32 ElementType;

		F32 xx, yx, zx;
		F32 xy, yy, zy;
		F32 xz, yz, zz;

		inline Mat3f()
		: xx( 1 ), yx( 0 ), zx( 0 )
		, xy( 0 ), yy( 1 ), zy( 0 )
		, xz( 0 ), yz( 0 ), zz( 1 )
		{
		}
		inline Mat3f( const Vec3f &cx, const Vec3f &cy, const Vec3f &cz )
		: xx( cx.x ), yx( cy.x ), zx( cz.x )
		, xy( cx.y ), yy( cy.y ), zy( cz.y )
		, xz( cx.z ), yz( cy.z ), zz( cz.z )
		{
		}
		Mat3f( const Mat4f &M );

		inline Vec3f columnX() const { return Vec3f( xx, xy, xz ); }
		inline Vec3f columnY() const { return Vec3f( yx, yy, yz ); }
		inline Vec3f columnZ() const { return Vec3f( zx, zy, zz ); }

		inline Vec3f rowX() const { return Vec3f( xx, yx, zx ); }
		inline Vec3f rowY() const { return Vec3f( xy, yy, zy ); }
		inline Vec3f rowZ() const { return Vec3f( xz, yz, zz ); }

		Mat3f &loadIdentity();
		Mat3f &loadTranspose();
		Mat3f &loadTranspose( const Mat3f &Other );
		inline Mat3f transposed() const { return Mat3f().loadTranspose( *this ); }

		Mat3f &loadRotation( const Vec3f &AnglesDegrees );
		Mat3f &loadScaling( const Vec3f &Scaling );

		Mat3f &applyRotation( const Vec3f &AnglesDegrees );
		Mat3f &applyXRotation( F32 fXAngleDegrees );
		Mat3f &applyYRotation( F32 fYAngleDegrees );
		Mat3f &applyZRotation( F32 fZAngleDegrees );
		Mat3f &applyScaling( const Vec3f &Scaling );
		inline Mat3f rotated( const Vec3f &AnglesDegrees ) const { return Mat3f( *this ).applyRotation( AnglesDegrees ); }
		inline Mat3f xrotated( F32 fXAngleDegrees ) const { return Mat3f( *this ).applyXRotation( fXAngleDegrees ); }
		inline Mat3f yrotated( F32 fYAngleDegrees ) const { return Mat3f( *this ).applyYRotation( fYAngleDegrees ); }
		inline Mat3f zrotated( F32 fZAngleDegrees ) const { return Mat3f( *this ).applyZRotation( fZAngleDegrees ); }
		inline Mat3f scaled( const Vec3f &Scaling ) const { return Mat3f( *this ).applyScaling( Scaling ); }

		Mat3f &loadMultiply( const Mat3f &A, const Mat3f &B );
		inline Mat3f &operator*=( const Mat3f &B ) { return loadMultiply( Mat3f( *this ), B ); }
		inline Mat3f operator*( const Mat3f &B ) const { return Mat3f().loadMultiply( *this, B ); }

		F32 determinant() const;

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
			AX_ASSERT( i < 9 && "Index out of bounds" );
			return reinterpret_cast< F32 * >( this )[ i ];
		}
		inline const F32 &operator[]( size_t i ) const
		{
			AX_ASSERT( i < 9 && "Index out of bounds" );
			return reinterpret_cast< const F32 * >( this )[ i ];
		}
	};

	// 4x4 matrix (with column-major memory layout)
	class Mat4f
	{
	public:
		static const unsigned kRows = 4;
		static const unsigned kColumns = 4;
		static const unsigned kDimensions = kRows*kColumns;
		typedef F32 ElementType;

		F32 xx, yx, zx, wx;
		F32 xy, yy, zy, wy;
		F32 xz, yz, zz, wz;
		F32 xw, yw, zw, ww;

		inline Mat4f()
		: xx( 1 ), yx( 0 ), zx( 0 ), wx( 0 )
		, xy( 0 ), yy( 1 ), zy( 0 ), wy( 0 )
		, xz( 0 ), yz( 0 ), zz( 1 ), wz( 0 )
		, xw( 0 ), yw( 0 ), zw( 0 ), ww( 1 )
		{
		}
		inline Mat4f( const Vec4f &cx, const Vec4f &cy, const Vec4f &cz, const Vec4f &cw )
		: xx( cx.x ), yx( cy.x ), zx( cz.x ), wx( cw.x )
		, xy( cx.y ), yy( cy.y ), zy( cz.y ), wy( cw.y )
		, xz( cx.z ), yz( cy.z ), zz( cz.z ), wz( cw.z )
		, xw( cx.w ), yw( cy.w ), zw( cz.w ), ww( cw.w )
		{
		}
		inline Mat4f( const Mat3f &M )
#if !SUUGAKU_REVERSE_ORDER_ENABLED
			: xx( M.xx ), yx( M.yx ), zx( M.zx ), wx( 0 )
			, xy( M.xy ), yy( M.yy ), zy( M.zy ), wy( 0 )
			, xz( M.xz ), yz( M.yz ), zz( M.zz ), wz( 0 )
			, xw( 0 ), yw( 0 ), zw( 0 ), ww( 1 )
#else
			: xx( M.xx ), xy( M.xy ), xz( M.xz ), xw( 0 )
			, yx( M.yx ), yy( M.yy ), yz( M.yz ), yw( 0 )
			, zx( M.zx ), zy( M.zy ), zz( M.zz ), zw( 0 )
			, wx( 0 ), wy( 0 ), wz( 0 ), ww( 1 )
#endif
		{
		}

		inline Vec4f columnX() const { return Vec4f( xx, xy, xz, xw ); }
		inline Vec4f columnY() const { return Vec4f( yx, yy, yz, yw ); }
		inline Vec4f columnZ() const { return Vec4f( zx, zy, zz, zw ); }
		inline Vec4f columnW() const { return Vec4f( wx, wy, wz, ww ); }

		inline Vec4f rowX() const { return Vec4f( xx, yx, zx, wx ); }
		inline Vec4f rowY() const { return Vec4f( xy, yy, zy, wy ); }
		inline Vec4f rowZ() const { return Vec4f( xz, yz, zz, wz ); }
		inline Vec4f rowW() const { return Vec4f( xw, yw, zw, ww ); }

		Mat4f &loadIdentity();
		Mat4f &loadTranspose();
		Mat4f &loadTranspose( const Mat4f &Other );
		inline Mat4f transposed() const { return Mat4f().loadTranspose( *this ); }

		Mat4f &loadPerspProj( F32 fFovDeg, F32 fAspect, F32 fZNear, F32 fZFar );
		Mat4f &loadOrthoProj( F32 fLeft, F32 fRight, F32 fBottom, F32 fTop, F32 fZNear, F32 fZFar );

		Bool isProjPersp() const;
		Bool isProjOrtho() const;
		F32 getPerspProjFov() const;
		F32 getPerspZNear() const;
		F32 getPerspZFar() const;

		Mat4f &loadTranslation( const Vec3f &Translation );
		Mat4f &loadRotation( const Vec3f &AnglesDegrees );
		Mat4f &loadScaling( const Vec3f &Scaling );

		Mat4f &loadAffine( const Mat3f &Rotation, const Vec3f &Translation );

		Mat4f &setTranslation( const Vec3f &Translation );

		Mat4f &applyTranslation( const Vec3f &Translation );
		Mat4f &applyRotation( const Vec3f &AnglesDegrees );
		Mat4f &applyXRotation( F32 fXAngleDegrees );
		Mat4f &applyYRotation( F32 fYAngleDegrees );
		Mat4f &applyZRotation( F32 fZAngleDegrees );
		Mat4f &applyScaling( const Vec3f &Scaling );
		inline Mat4f translated( const Vec3f &Translation ) const { return Mat4f( *this ).applyTranslation( Translation ); }
		inline Mat4f rotated( const Vec3f &AnglesDegrees ) const { return Mat4f( *this ).applyRotation( AnglesDegrees ); }
		inline Mat4f xrotated( F32 fXAngleDegrees ) const { return Mat4f( *this ).applyXRotation( fXAngleDegrees ); }
		inline Mat4f yrotated( F32 fYAngleDegrees ) const { return Mat4f( *this ).applyYRotation( fYAngleDegrees ); }
		inline Mat4f zrotated( F32 fZAngleDegrees ) const { return Mat4f( *this ).applyZRotation( fZAngleDegrees ); }
		inline Mat4f scaled( const Vec3f &Scaling ) const { return Mat4f( *this ).applyScaling( Scaling ); }

		Mat4f &loadAffineInverse( const Mat4f &AffineMatrix );
		Mat4f &loadAffineMultiply( const Mat4f &AffineA, const Mat4f &AffineB );
		Mat4f &loadMultiply( const Mat4f &A, const Mat4f &B );
		inline Mat4f affineInverse() const { return Mat4f().loadAffineInverse( *this ); }
		inline Mat4f &operator*=( const Mat4f &B ) { return loadMultiply( Mat4f( *this ), B ); }
		inline Mat4f operator*( const Mat4f &B ) const { return Mat4f().loadMultiply( *this, B ); }

		F32 determinant() const;

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
			AX_ASSERT( i < 16 && "Index out of bounds" );
			return reinterpret_cast< F32 * >( this )[ i ];
		}
		inline const F32 &operator[]( size_t i ) const
		{
			AX_ASSERT( i < 16 && "Index out of bounds" );
			return reinterpret_cast< const F32 * >( this )[ i ];
		}
	};

	inline Mat3f transpose( const Mat3f &A ) { return A.transposed(); }
	inline Mat4f transpose( const Mat4f &A ) { return A.transposed(); }

	inline Mat3f rotationMat3( const Vec3f &AnglesDegrees ) { return Mat3f().loadRotation( AnglesDegrees ); }
	inline Mat3f scalingMat3( const Vec3f &Scaling ) { return Mat3f().loadScaling( Scaling ); }

	inline Mat4f translationMat4( const Vec3f &Translation ) { return Mat4f().setTranslation( Translation ); }
	inline Mat4f rotationMat4( const Vec3f &AnglesDegrees ) { return Mat4f().loadRotation( AnglesDegrees ); }
	inline Mat4f scalingMat4( const Vec3f &Scaling ) { return Mat4f().loadScaling( Scaling ); }

	inline Mat3f multiply( const Mat3f &A, const Mat3f &B ) { return Mat3f().loadMultiply( A, B ); }

	inline Mat4f affineMultiply( const Mat4f &A, const Mat4f &B ) { return Mat4f().loadAffineMultiply( A, B ); }
	inline Mat4f multiply( const Mat4f &A, const Mat4f &B ) { return Mat4f().loadMultiply( A, B ); }

	inline Mat4f affineInverse( const Mat4f &A ) { return Mat4f().loadAffineInverse( A ); }

	//--------------------------------------------------------------------//

	inline Mat3f::Mat3f( const Mat4f &M )
	: xx( M.xx ), yx( M.yx ), zx( M.zx )
	, xy( M.xy ), yy( M.yy ), zy( M.zy )
	, xz( M.xz ), yz( M.yz ), zz( M.zz )
	{
	}

}
