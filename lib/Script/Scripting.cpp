#include "../BuildSettings.hpp"

#include "doll/Script/Scripting.hpp"
#include "doll/Script/Compiler.hpp"
#include "doll/Script/Diagnostics.hpp"
#include "doll/Script/LanguageVersion.hpp"
#include "doll/Script/Lexer.hpp"
#include "doll/Script/ProgramData.hpp"
#include "doll/Script/RuntimeConf.hpp"
#include "doll/Script/Source.hpp"
#include "doll/Script/Token.hpp"
#include "doll/Script/Type.hpp"
#include "doll/Core/Memory.hpp"
#include "doll/Core/MemoryTags.hpp"

namespace doll {

using namespace script;

class RCompiler
: public TPoolObject< RCompiler, kTag_Script >
{
	CCompilerContext m_compiler;

public:
	RCompiler()
	: m_compiler()
	{
	}
	~RCompiler() {
	}

	Bool init() {
		if( !AX_VERIFY( m_compiler.init( kVer_1_0 ) ) ) {
			return false;
		}

		return true;
	}
	Bool openSource( const Str &filename ) {
		return m_compiler.openSource( filename );
	}
	Bool nextSource() {
		return m_compiler.nextSource();
	}

	Bool hasOpenSource() const {
		return m_compiler.getActiveSource() != nullptr;
	}
	Bool setSourceName( const Str &filename ) {
		Source *const source = m_compiler.getActiveSource();
		AX_ASSERT_NOT_NULL( source );

		source->filename.assign( filename );
		return true;
	}
	Bool getSourceName( Str &dstFilename ) const {
		const Source *const src = m_compiler.getActiveSource();
		if( !src ) {
			dstFilename = Str();
			return false;
		}

		dstFilename = src->filename;
		return true;
	}

	Bool testCurrentSource() {
		return m_compiler.testCurrentSource();
	}
};

DOLL_FUNC RCompiler *DOLL_API sc_new() {
	RCompiler *compiler;

	if( !AX_VERIFY_MEMORY( compiler = new RCompiler() ) ) {
		return nullptr;
	}
	if( !compiler->init() ) {
		delete compiler;
		return nullptr;
	}

	return compiler;
}
DOLL_FUNC NullPtr DOLL_API sc_delete( RCompiler *compiler ) {
	delete compiler;
	return nullptr;
}

DOLL_FUNC Bool DOLL_API sc_openSource( RCompiler *compiler, const Str &filename ) {
	AX_ASSERT_NOT_NULL( compiler );
	return compiler->openSource( filename );
}
DOLL_FUNC Bool DOLL_API sc_nextSource( RCompiler *compiler ) {
	AX_ASSERT_NOT_NULL( compiler );
	return compiler->nextSource();
}
DOLL_FUNC Bool DOLL_API sc_isOpen( const RCompiler *compiler ) {
	AX_ASSERT_NOT_NULL( compiler );
	return compiler->hasOpenSource();
}
DOLL_FUNC Bool DOLL_API sc_setSourceName( RCompiler *compiler, const Str &filename ) {
	AX_ASSERT_NOT_NULL( compiler );
	AX_ASSERT_MSG( compiler->hasOpenSource(), "Cannot set the name of a nonexistant source file" );
	return compiler->setSourceName( filename );
}
DOLL_FUNC Bool DOLL_API sc_getSourceName( const RCompiler *compiler, Str &dstFilename ) {
	AX_ASSERT_NOT_NULL( compiler );

	return compiler->getSourceName( dstFilename );
}

DOLL_FUNC Bool DOLL_API sc_testCurrentSource( RCompiler *compiler ) {
	AX_ASSERT_NOT_NULL( compiler );
	AX_ASSERT_MSG( compiler->hasOpenSource(), "Cannot test if there is no open source file" );

	return compiler->testCurrentSource();
}

}
