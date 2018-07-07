#pragma once

#include "doll/Gfx/API.hpp"
#include "doll/Core/Logger.hpp"

namespace doll
{

	class DummyDiag: public IGfxDiagnostic
	{
	public:
		DummyDiag() {}
		virtual ~DummyDiag() {}

		virtual Void error( Str filename, U32 line, U32 column, Str message ) override
		{
			g_ErrorLog(filename, line, column) += message;
		}
		virtual Void diagnostic( Str filename, U32 line, U32 column, Str message ) override
		{
			g_InfoLog(filename, line, column) += message;
		}
	};

}
