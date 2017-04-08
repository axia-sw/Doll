#pragma once

#include "../Core/Defs.hpp"
#include "../Core/Memory.hpp"

namespace doll { namespace script {

	class CCompilerContext;

#define DOLL__SCRIPTOBJ_LINFO  __FILE__, __LINE__, AX_FUNCTION
#define DOLL__SCRIPTOBJ_NEW    new   ( DOLL__SCRIPTOBJ_LINFO )
#define DOLL__SCRIPTOBJ_DELETE delete( DOLL__SCRIPTOBJ_LINFO )

	class CompilerObject
	{
	public:
		CompilerObject( CCompilerContext &ctx );
		virtual ~CompilerObject();

		inline CCompilerContext &getContext() const
		{
			return m_context;
		}

		static Void *operator new( SizeType cBytes );
		static Void operator delete( Void *pBytes );

		static Void *operator new[]( SizeType cBytes );
		static Void operator delete[]( Void *pBytes );

		static Void *operator new( SizeType cBytes, const char *pszFilename, U32 uLine, const char *pszFunc );
		static Void operator delete( Void *pBytes, const char *pszFilename, U32 uLine, const char *pszFunc );

		static Void *operator new[]( SizeType cBytes, const char *pszFilename, U32 uLine, const char *pszFunc );
		static Void operator delete[]( Void *pBytes, const char *pszFilename, U32 uLine, const char *pszFunc );

		// placement new
		static Void *operator new( SizeType cBytes, Void *pExistingObj );
		// placement new (dummy delete)
		static Void operator delete( Void *pBytes, Void *pExistingObj );

	private:
		CCompilerContext &m_context;
	};

}}
