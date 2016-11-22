#include "doll/Util/FuncMgr.hpp"

namespace doll
{

	DOLL_FUNC Bool DOLL_API detail::initIState( Void *pStateData, Void *pUserData )
	{
		AX_ASSERT_NOT_NULL( pStateData );
		return reinterpret_cast< IState * >( pStateData )->init( pUserData );
	}
	DOLL_FUNC Void DOLL_API detail::finiIState( Void *pStateData, Void *pUserData )
	{
		AX_ASSERT_NOT_NULL( pStateData );
		reinterpret_cast< IState * >( pStateData )->fini( pUserData );
	}
	DOLL_FUNC Bool DOLL_API detail::stepIState( Void *pStateData, Void *pUserData )
	{
		AX_ASSERT_NOT_NULL( pStateData );
		return reinterpret_cast< IState * >( pStateData )->step( pUserData );
	}

}
