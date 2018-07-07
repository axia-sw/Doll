#include "../BuildSettings.hpp"
#include "doll/Script/Ident.hpp"

namespace doll { namespace script {

	Ident::Ident( CCompilerContext &ctx )
	: CompilerObject( ctx )
	, name()
	, isKeyword( false )
	, keyword()
	, pType( nullptr )
	{
	}
	Ident::~Ident()
	{
	}

}}
