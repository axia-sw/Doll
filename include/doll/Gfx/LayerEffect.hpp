#pragma once

#include "../Core/Defs.hpp"

namespace doll
{

	class RLayer;

	class ILayerEffect
	{
	public:
		ILayerEffect() {}
		virtual ~ILayerEffect() {}

		virtual void run( RLayer &layer ) = 0;
	};

}
