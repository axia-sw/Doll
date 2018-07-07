#include "../BuildSettings.hpp"

#include "doll/Script/CompilerMemory.hpp"
#include "doll/Core/MemoryTags.hpp"

namespace doll { namespace script {

	CompilerObject::CompilerObject( CCompilerContext &ctx )
	: m_context( ctx )
	{
	}
	CompilerObject::~CompilerObject()
	{
	}

	Void *CompilerObject::operator new( SizeType cBytes )
	{
		return DOLL__TEMP_ALLOCATOR->alloc( cBytes, kTag_Script, nullptr, 0, nullptr );
	}
	Void CompilerObject::operator delete( Void *pBytes )
	{
		DOLL__TEMP_ALLOCATOR->dealloc( pBytes, nullptr, 0, nullptr );
	}

	Void *CompilerObject::operator new[]( SizeType cBytes )
	{
		return DOLL__TEMP_ALLOCATOR->alloc( cBytes, kTag_Script, nullptr, 0, nullptr );
	}
	Void CompilerObject::operator delete[]( Void *pBytes )
	{
		DOLL__TEMP_ALLOCATOR->dealloc( pBytes, nullptr, 0, nullptr );
	}

	Void *CompilerObject::operator new( SizeType cBytes, const char *pszFilename, U32 uLine, const char *pszFunc )
	{
		return DOLL__TEMP_ALLOCATOR->alloc( cBytes, kTag_Script, pszFilename, int( uLine ), pszFunc );
	}
	Void CompilerObject::operator delete( Void *pBytes, const char *pszFilename, U32 uLine, const char *pszFunc )
	{
		DOLL__TEMP_ALLOCATOR->dealloc( pBytes, pszFilename, int( uLine ), pszFunc );
	}

	Void *CompilerObject::operator new[]( SizeType cBytes, const char *pszFilename, U32 uLine, const char *pszFunc )
	{
		return DOLL__TEMP_ALLOCATOR->alloc( cBytes, kTag_Script, pszFilename, int( uLine ), pszFunc );
	}
	Void CompilerObject::operator delete[]( Void *pBytes, const char *pszFilename, U32 uLine, const char *pszFunc )
	{
		DOLL__TEMP_ALLOCATOR->dealloc( pBytes, pszFilename, int( uLine ), pszFunc );
	}

	Void *CompilerObject::operator new( SizeType cBytes, Void *pExistingObj )
	{
		( Void )cBytes;
		( Void )pExistingObj;

		return pExistingObj;
	}
	Void CompilerObject::operator delete( Void *pBytes, Void *pExistingObj )
	{
		( Void )pBytes;
		( Void )pExistingObj;
	}

}}
