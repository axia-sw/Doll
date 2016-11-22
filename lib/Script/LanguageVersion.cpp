#include "doll/Script/LanguageVersion.hpp"

namespace doll { namespace script {

	struct SKeyword
	{
		const Str &            key;
		const ESubtokenKeyword value;
	};

	static Bool addKeyword( CCompilerContext &ctx, IdentDictionary &dstDict, const SKeyword &kw )
	{
		IdentDictionary::SEntry *pEntry;

		pEntry = dstDict.lookup( kw.key );
		if( !AX_VERIFY_MEMORY( pEntry ) ) {
			return false;
		}

		AX_ASSERT_IS_NULL( pEntry->pData );

		if( !AX_VERIFY_MEMORY( pEntry->pData = DOLL__SCRIPTOBJ_NEW Ident( ctx ) ) ) {
			return false;
		}

		pEntry->pData->name      = kw.key;
		pEntry->pData->isKeyword = true;
		pEntry->pData->keyword   = kw.value;

		return true;
	}
	template< UPtr tNumKeys >
	static Bool addKeywords( CCompilerContext &ctx, IdentDictionary &dstDict, const SKeyword( &keys )[ tNumKeys ] )
	{
		for( const SKeyword &key : keys ) {
			if( !addKeyword( ctx, dstDict, key ) ) {
				return false;
			}
		}

		return true;
	}

	static Bool addKeywordsV1p0( CCompilerContext &ctx, IdentDictionary &dstDict )
	{
		static const SKeyword keywords[] = {
			{ "null"        , kKW_Null         },
			{ "false"       , kKW_False        },
			{ "true"        , kKW_True         },
			{ "if"          , kKW_If           },
			{ "elseif"      , kKW_ElseIf       },
			{ "else"        , kKW_Else         },
			{ "menu"        , kKW_Menu         },
			{ "loop"        , kKW_Loop         },
			{ "while"       , kKW_While        },
			{ "do"          , kKW_Do           },
			{ "for"         , kKW_For          },
			{ "in"          , kKW_In           },
			{ "struct"      , kKW_Struct       },
			{ "class"       , kKW_Class        },
			{ "extension"   , kKW_Extension    },
			{ "interface"   , kKW_Interface    },
			{ "protocol"    , kKW_Protocol     },
			{ "func"        , kKW_Func         },
			{ "var"         , kKW_Var          },
			{ "let"         , kKW_Let          },
			{ "goto"        , kKW_Goto         },
			{ "return"      , kKW_Return       },
			{ "switch"      , kKW_Switch       },
			{ "case"        , kKW_Case         },
			{ "default"     , kKW_Default      },
			{ "break"       , kKW_Break        },
			{ "continue"    , kKW_Continue     },
			{ "defer"       , kKW_Defer        },
			{ "multimessage", kKW_Multimessage }
		};

		return addKeywords( ctx, dstDict, keywords );
	}

	Bool initCompilerDictionary( CCompilerContext &ctx, IdentDictionary &dstDict, EVersion ver )
	{
		if( !dstDict.init( AX_DICT_IDENT AX_DICT_UNICODE "." ) ) {
			return false;
		}

		switch( ver ) {
		case kVer_1_0:
			return addKeywordsV1p0( ctx, dstDict );
		}

		AX_ASSERT_MSG( false, "Unknown version" );
		return false;
	}

}}
