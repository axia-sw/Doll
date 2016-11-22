#pragma once

#include "../Core/Defs.hpp"

#include "Token.hpp"
#include "CompilerMemory.hpp"

namespace doll { namespace script {

	class Type;

	class Ident: public CompilerObject
	{
	public:
		// Reference to the identifier's text (this can be directly from the loaded source file)
		Str              name;
		// Whether this is a keyword
		Bool             isKeyword;
		// If this is a keyword, then which keyword?
		ESubtokenKeyword keyword;
		// If this is a type, then which type?
		Type *           pType;

		Ident( CCompilerContext &ctx );
		virtual ~Ident();

		Ident( const Ident & ) = delete;
		Ident &operator=( const Ident & ) = delete;
	};

	typedef TDictionary<Ident> IdentDictionary;

}}
