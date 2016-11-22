#pragma once

#include "../Core/Defs.hpp"

/// \def DOLL_VECTOR_ALIGNMENT
/// \brief The required alignment for vectors.
///
/// \def DOLL_VECALIGN
/// \brief Same as <tt>AX_ALIGN( DOLL_VECTOR_ALIGNMENT )</tt>.

// include the appropriate intrinsics headers
#if AX_INTRIN_SSE
# include <mmintrin.h>
# include <emmintrin.h>
# include <xmmintrin.h>
# ifndef DOLL_VECTOR_ALIGNMENT
#  define DOLL_VECTOR_ALIGNMENT 16
# endif
#else
# ifndef DOLL_VECTOR_ALIGNMENT
#  define DOLL_VECTOR_ALIGNMENT 0
# endif
#endif

// figure out vector alignment
#ifndef DOLL_VECALIGN
# if DOLL_VECTOR_ALIGNMENT > 1
#  define DOLL_VECALIGN AX_ALIGN( DOLL_VECTOR_ALIGNMENT )
# else
#  define DOLL_VECALIGN
# endif
#endif
