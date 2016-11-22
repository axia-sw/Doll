#pragma once

#include "../Core/Defs.hpp"
#include "../IO/File.hpp"

#include "Diagnostics.hpp"
#include "Ident.hpp"
#include "ProgramData.hpp"
#include "Token.hpp"

#include "Lexer.hpp"

namespace doll { namespace script {

	class Source;

	enum EVersion: U32;

	class CCompilerContext
	{
	friend class Source;
	public:
		CCompilerContext();
		~CCompilerContext();

		Bool init( EVersion ver );

		Bool openSource( const Str &filename );
		Bool nextSource();

		Bool pushSource( const Str &filename );
		Bool popSource();

		const Source *getActiveSource_const() const;

		inline Source *getActiveSource() {
			return const_cast< Source * >( getActiveSource_const() );
		}
		inline const Source *getActiveSource() const {
			return getActiveSource_const();
		}

		CDiagnosticEngine &getDiagnosticEngine();
		CProgramData &getProgramData();
		IdentDictionary &getSymbolMap();

		Bool testCurrentSource();

	private:
		// Master collection of source files
		TMutArr<Source *> m_pSources;
		// Current source stack (from the source we're presently operating on)
		TMutArr<U16>       m_srcStack;
		// Queued source files to operate upon after the current stack is exhausted
		TList<U16>         m_srcQueue;

		// Diagnostics system
		CDiagnosticEngine  m_diagEngine;
		
		// Data relevant to the program this context represents
		CProgramData       m_progData;

		// Identifier dictionary (used to speedup identifier/symbol lookups)
		IdentDictionary    m_symDict;

		Source *allocSource();
		NullPtr freeSource( Source *pSrc );

		Bool loadSourceFile( Source &, const Str &filename );

		template< typename T, typename... TArgs >
		Bool addBuiltinType( Str name, TArgs && ... args )
		{
			AX_ASSERT( name.len() > 0 );
			AX_ASSERT( name[ 0 ] >= 'A' && name[ 0 ] <= 'Z' );

			T *const pType = new T( *this, move( args )... );
			if( !AX_VERIFY_MEMORY( pType ) ) {
				return false;
			}

			auto *const pEntry = m_symDict.lookup( name );
			if( !AX_VERIFY_MEMORY( pEntry ) ) {
				delete pType;
				return false;
			}

			AX_ASSERT_IS_NULL( pEntry->pData );

			Ident *const pIdent = new Ident( *this );
			if( !AX_VERIFY_MEMORY( pIdent ) ) {
				delete pType;
				return false;
			}

			pEntry->pData = pIdent;

			pIdent->name  = name;
			pIdent->pType = pType;

			return true;
		}

		CCompilerContext( const CCompilerContext & ) = delete;
		CCompilerContext &operator=( const CCompilerContext & ) = delete;
	};

}}
