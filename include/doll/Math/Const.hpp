#pragma once

#include "../Core/Defs.hpp"

namespace doll {

	/// \def DOLL_PI
	/// Constant value of pi (3.1415926535~).
#ifdef _M_PI
# define DOLL_PI _M_PI
#else
# define DOLL_PI ( ( float )3.14159265358979323846f )
#endif
	/// \def DOLL_TAU
	/// Constant value of tau (pi*2).
#define DOLL_TAU ( DOLL_PI*2.0f )
	/// \def DOLL_EPSILON
	/// Margin for approximate float equality tests
#define DOLL_EPSILON 1e-6f

	/// Symbolic constant for pi (3.1415926535~).
	AX_GLOBAL_CONST float PI = DOLL_PI;
	/// Symbolic constant for tau (pi*2).
	AX_GLOBAL_CONST float TAU = DOLL_TAU;
	/// Symbolic constant for half the value of pi.
	AX_GLOBAL_CONST float HALF_PI = DOLL_PI/2.0f;

	/// Beginning of the upper right quadrant of a radian angle.
	AX_GLOBAL_CONST float RADIAN_QUAD1 = DOLL_TAU*0.00f;
	/// Beginning of the bottom right quadrant of a radian angle.
	AX_GLOBAL_CONST float RADIAN_QUAD2 = DOLL_TAU*0.25f;
	/// Beginning of the bottom left quadrant of a radian angle.
	AX_GLOBAL_CONST float RADIAN_QUAD3 = DOLL_TAU*0.50f;
	/// Beginning of the upper left quadrant of a radian angle.
	AX_GLOBAL_CONST float RADIAN_QUAD4 = DOLL_TAU*0.75f;

	/// Beginning of the upper right quadrant of a degree angle.
	AX_GLOBAL_CONST float DEGREE_QUAD1 = 360.0f*0.00f;
	/// Beginning of the bottom right quadrant of a degree angle.
	AX_GLOBAL_CONST float DEGREE_QUAD2 = 360.0f*0.25f;
	/// Beginning of the bottom left quadrant of a degree angle.
	AX_GLOBAL_CONST float DEGREE_QUAD3 = 360.0f*0.50f;
	/// Beginning of the upper left quadrant of a degree angle.
	AX_GLOBAL_CONST float DEGREE_QUAD4 = 360.0f*0.75f;

	namespace detail
	{
		AX_GLOBAL_CONST uint32 ind_int = 0xFFC00000;
		AX_GLOBAL_CONST uint32 inf_int = 0x7F800000;
		AX_GLOBAL_CONST uint32 nan_int = 0x7FFFFFFF;
	}
		
#undef INDEFINITE
#undef INFINITY
#undef NAN
	
	/// Floating-point indefinite value.
	AX_GLOBAL_CONST float INDEFINITE = *( const float * )&detail::ind_int;
	/// Floating-point infinity value.
	AX_GLOBAL_CONST float INFINITY = *( const float * )&detail::inf_int;
	/// Floating-point not-a-number (NAN) value.
	AX_GLOBAL_CONST float NAN = *( const float * )&detail::nan_int;

	/// \def AX_INDEFINITE
	/// Floating-point indefinite value
	/// \def AX_INFINITY
	/// Floating-point infinity value.
	/// \def AX_NAN
	/// Floating-point not-a-number (NAN) value.
#define AX_INDEFINITE ( ::doll::INDEFINITE )
#define AX_INFINITY ( ::doll::INFINITY )
#define AX_NAN ( ::doll::NAN )

}
