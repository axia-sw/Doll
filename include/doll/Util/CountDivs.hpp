#pragma once

#include "../Core/Defs.hpp"

namespace doll
{

	namespace detail
	{

		template< unsigned tDenom, U64 tMaxValue = ~U64( 0 ) - ( tDenom - 1 ), unsigned tIter = 0 >
		struct TCountDivs
		{
			static_assert( tDenom > 0, "Division by zero" );

			static const unsigned count =
				( tMaxValue >= tDenom )
				? TCountDivs< tDenom, tMaxValue/tDenom, tIter + 1 >::count
				: tIter;
		};
		template< unsigned tDenom, unsigned tIter >
		struct TCountDivs< tDenom, 0, tIter >
		{
			static const unsigned count = tIter;
		};

	}

	template< unsigned tDenom, U64 tMaxValue = ~U64( 0 ) >
	struct TCountDivs: public detail::TCountDivs< tDenom, tMaxValue, 0 >
	{
	};

}
