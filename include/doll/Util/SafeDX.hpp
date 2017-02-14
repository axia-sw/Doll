#pragma once

#include "../Core/Defs.hpp"
#include "../Core/Logger.hpp"

#if !DOLL_DX_AVAILABLE
# error "doll-util-safedx.hpp" included, but DirectX is not available.
#endif

namespace doll
{

	namespace detail
	{

		static inline Bool handleSafeDX( HRESULT hr, Str expr, Str file, U32 uLine, Str func )
		{
			if( SUCCEEDED( hr ) ) {
				return true;
			}

			char szBuf[ 512 ];
			g_ErrorLog[ kLog_UtilSafeDX ]( file, uLine, func ) += (axspf(szBuf, "DX error 0x%.8X from %.*s", U32(hr), expr.lenInt(), expr.get() ), szBuf);

			return false;
		}

	}

#define DOLL_SAFE_DX( dxcall )\
	if( !::doll::detail::handleSafeDX( (dxcall), #dxcall, __FILE__, __LINE__, AX_FUNCTION ) )

}
