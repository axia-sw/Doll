#pragma once

#include "doll/Core/Defs.hpp"
#include "doll/OS/Window.hpp"

namespace doll {

	template< typename TCallbackFn, typename... TArgs >
	inline Bool callDelegate( SWndDelegate *pDelegate, TCallbackFn *ppfnFunc, TArgs... args )
	{
		if( !pDelegate || !ppfnFunc ) {
			return false;
		}

		const UPtr uByteOffset = UPtr( ppfnFunc ) - UPtr( pDelegate );

		for(;;) {
			if( *ppfnFunc != nullptr && int( ( *ppfnFunc )( args... ) ) != 0 ) {
				return true;
			}

			pDelegate = pDelegate->pSuper;
			if( !pDelegate ) {
				break;
			}

			ppfnFunc = ( TCallbackFn * )( UPtr( pDelegate ) + uByteOffset );
		}

		return false;
	}

}
