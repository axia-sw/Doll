#pragma once

#include "../Core/Defs.hpp"

namespace doll
{

	template< typename TDst, typename TSrc >
	Bool isA( const TSrc &x )
	{
		/*

			Example implementation of isClass:

				static Bool isClass( const T *p )
				{
					return p->getKind() == T::kind;
				}

		*/
		return TDst::isClass( &x );
	}

	template< typename TDst, typename TSrc >
	TDst *cast( TSrc *p )
	{
		AX_ASSERT_NOT_NULL( p );
		AX_ASSERT_MSG( isA< TDst >( *p ), "Cast fail: TSrc is not a TDst" );

		return ( TDst * )( p );
	}

	template< typename TDst, typename TSrc >
	TDst *castOrNull( TSrc *p )
	{
		if( !p || !isA< TDst >( *p ) ) {
			return nullptr;
		}

		return cast< TDst, TSrc >( p );
	}

}
