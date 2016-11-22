#pragma once

#include "Defs.hpp"
#include "Memory.hpp"
#include "MemoryTags.hpp"

namespace doll
{

	/*
	===============================================================================

		INTERNAL CLASSES

	===============================================================================
	*/
	struct SConfigLineInfo
	{
		Str filename;

		UPtr fileOffset;
		UPtr lineOffset;

		UPtr line;
		UPtr column;

		inline SConfigLineInfo()
		: filename( nullptr )
		, fileOffset( 0 )
		, lineOffset( 0 )
		, line( 0 )
		, column( 0 )
		{
		}
	};

	struct SConfigValue
	{
		static const int kTag = kTag_Config;

		MutStr value;
		TIntrLink< SConfigValue > link;

		inline SConfigValue()
		: value()
		, link()
		{
		}
	};
	struct SConfigVar
	{
		static const int kTag = kTag_Config;

		class CConfiguration *config;

		MutStr name;
		TIntrList< SConfigValue > values;

		SConfigVar *parent;
		TIntrList< SConfigVar > children;
		TIntrLink< SConfigVar > sibling;

		inline SConfigVar()
		: config( nullptr )
		, name()
		, values()
		, parent( nullptr )
		, children()
		, sibling()
		{
		}
	};

	class CConfiguration
	{
	public:
		static const int kTag = kTag_Config;

		CConfiguration();
		~CConfiguration();

		Void clear();

		Bool loadFromMemory( Str filename, Str buffer );
		Bool loadFromFile( Str filename );

		Void printVars();
		Void printVar( const SConfigVar &var );

		SConfigVar *addVar( SConfigVar *parent, Str name );
		SConfigVar *findVar( SConfigVar *parent, Str name );
		Void removeVar( SConfigVar *var );

		SConfigValue *addValue( SConfigVar *var, Str value );
		SConfigValue *findValue( SConfigVar *var, Str value );
		Void removeValue( SConfigVar *var, SConfigValue *value );
		Void removeAllValues( SConfigVar *var );

		inline SConfigVar *head()
		{
			return mSections.head();
		}
		inline SConfigVar *tail()
		{
			return mSections.tail();
		}

	private:
		enum class EProcessMode
		{
			// sets the property to a new value if it exists; create if not exist
			// Key=Value
			Set,
			// adds a line to an existing property; create if not exist
			// +Key=Value
			Add,
			// adds a unique line to an existing property; create if not exist
			// .Key=Value
			AddUnique,
			// remove an exact match
			// -Key=Value
			RemoveExact,
			// remove based only on the name
			// !Key=Value
			RemoveInexact
		};

		TIntrList< SConfigVar > mSections;
		UPtr                    mIncludeDepth;

		Void process( SConfigVar *parent, EProcessMode mode, Str name, Str value, Str filename, Str buffer, const char *p );

		Void error( const SConfigLineInfo &linfo, const char *message );
		Void errorRaw( Str filename, Str buffer, const char *p, const char *message );

		Void warn( const SConfigLineInfo &linfo, const char *message );
		Void warnRaw( Str filename, Str buffer, const char *p, const char *message );
	};

	DOLL_FUNC CConfiguration *DOLL_API core_newConfig();
	DOLL_FUNC CConfiguration *DOLL_API core_deleteConfig( CConfiguration *config );

	DOLL_FUNC CConfiguration *DOLL_API core_loadConfig( Str filename );
	DOLL_FUNC Bool DOLL_API core_appendConfig( CConfiguration *config, Str filename );
	DOLL_FUNC Bool DOLL_API core_appendConfigString( CConfiguration *config, Str filename, Str source );

	DOLL_FUNC Bool DOLL_API core_queryConfigValue( CConfiguration *config, Str &out_value, Str section, Str key );
	inline Str DOLL_API core_getConfigValue( CConfiguration *config, Str section, Str key )
	{
		Str t;
		core_queryConfigValue( config, t, section, key );
		return t;
	}

	DOLL_FUNC Void DOLL_API core_clearConfig( CConfiguration *config );

	DOLL_FUNC SConfigVar *DOLL_API core_findConfigSection( CConfiguration *config, Str name );
	DOLL_FUNC SConfigVar *DOLL_API core_findConfigVar( SConfigVar *prnt, Str name );
	DOLL_FUNC SConfigValue *DOLL_API core_findConfigVarValue( SConfigVar *var, Str value );

	DOLL_FUNC SConfigVar *DOLL_API core_getFirstConfigSection( CConfiguration *config );
	DOLL_FUNC SConfigVar *DOLL_API core_getLastConfigSection( CConfiguration *config );
	DOLL_FUNC SConfigVar *DOLL_API core_getFirstConfigVar( SConfigVar *prnt );
	DOLL_FUNC SConfigVar *DOLL_API core_getLastConfigVar( SConfigVar *prnt );
	DOLL_FUNC SConfigVar *DOLL_API core_getConfigVarBefore( SConfigVar *var );
	DOLL_FUNC SConfigVar *DOLL_API core_getConfigVarAfter( SConfigVar *var );
	DOLL_FUNC SConfigVar *DOLL_API core_getConfigVarParent( SConfigVar *var );

	DOLL_FUNC Bool DOLL_API core_getConfigVarName( Str &dst, SConfigVar *var );
	DOLL_FUNC Bool DOLL_API core_getConfigVarValue( Str &dst, SConfigVar *var );
	inline Str DOLL_API core_getConfigVarName( SConfigVar *var )
	{
		Str r;
		return core_getConfigVarName( r, var ), r;
	}
	inline Str DOLL_API core_getConfigVarValue( SConfigVar *var )
	{
		Str r;
		return core_getConfigVarValue( r, var ), r;
	}

	DOLL_FUNC SConfigValue *DOLL_API core_getFirstConfigVarValue( SConfigVar *var );
	DOLL_FUNC SConfigValue *DOLL_API core_getLastConfigVarValue( SConfigVar *var );
	DOLL_FUNC SConfigValue *DOLL_API core_getConfigValueBefore( SConfigValue *val );
	DOLL_FUNC SConfigValue *DOLL_API core_getConfigValueAfter( SConfigValue *val );
	DOLL_FUNC Bool DOLL_API core_getConfigValueString( Str &dst, SConfigValue *val );
	inline Str DOLL_API core_getConfigValueString( SConfigValue *val )
	{
		Str r;
		return core_getConfigValueString( r, val ), r;
	}

	DOLL_FUNC SConfigVar *DOLL_API core_removeConfigVar( SConfigVar *var );

}
