#define DOLL_TRACE_FACILITY doll::kLog_CoreConfig

#include "doll/Core/Config.hpp"
#include "doll/Core/Logger.hpp"

#include "doll/IO/File.hpp"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
# include <unistd.h>
#endif

#ifndef AX_SECURE_LIB_ENABLED
# ifdef _MSC_VER
#  define AX_SECURE_LIB_ENABLED 1
# else
#  define AX_SECURE_LIB_ENABLED 0
# endif
#endif

/*
===============================================================================

	AXSDK/UTILITY/TEXT.HPP

===============================================================================
*/
#ifndef AXSDK_UTILITY_TEXT_HPP
#define AXSDK_UTILITY_TEXT_HPP

#ifndef AX_PURE_FUNCTION
# define AX_PURE_FUNCTION
#endif

namespace ax {

	namespace Text {

		using namespace doll;

		inline Bool IsLower( char c ) AX_PURE_FUNCTION {
			return ( c >= 'a' && c <= 'z' ) ? true : false;
		}
		inline Bool IsUpper( char c ) AX_PURE_FUNCTION {
			return ( c >= 'A' && c <= 'Z' ) ? true : false;
		}

		inline char ToLower( char c ) AX_PURE_FUNCTION {
			if( IsUpper( c ) ) {
				return c - 'A' + 'a';
			}

			return c;
		}
		inline char ToUpper( char c ) AX_PURE_FUNCTION {
			if( IsLower( c ) ) {
				return c - 'a' + 'A';
			}

			return c;
		}

		UPtr Copy( char *dst, UPtr maxDst, const char *src );
		UPtr CopyN( char *dst, UPtr maxDst, const char *src, UPtr n );

		UPtr Append( char *dst, size_t maxDst, const char *src );
		UPtr AppendN( char *dst, size_t maxDst, const char *src, UPtr n );

		Void ConvertLower( char *buf );
		Void ConvertUpper( char *buf );
		Void ConvertLowerN( char *buf, UPtr n );
		Void ConvertUpperN( char *buf, UPtr n );

		Bool Compare( const char *a, const char *b );
		Bool CompareInsensitive( const char *a, const char *b );

		Bool CompareN( const char *a, const char *b, UPtr n );
		Bool CompareInsensitiveN( const char *a, const char *b, UPtr n );

		UPtr FormatV( char *dst, UPtr maxDst, const char *fmt, va_list args );
		inline UPtr Format( char *dst, UPtr maxDst, const char *fmt, ... ) {
			va_list args;

			va_start( args, fmt );
			const UPtr r = FormatV( dst, maxDst, fmt, args );
			va_end( args );

			return r;
		}

		template< UPtr _size_ >
		inline UPtr Copy( char ( &dst )[ _size_ ], const char *src ) {
			return Copy( &dst[ 0 ], _size_, src );
		}
		template< UPtr _size_ >
		inline UPtr CopyN( char ( &dst )[ _size_ ], const char *src,
		UPtr n ) {
			return CopyN( &dst[ 0 ], _size_, src, n );
		}

		template< UPtr _size_ >
		inline UPtr Append( char ( &dst )[ _size_ ], const char *src ) {
			return Append( &dst[ 0 ], _size_, src );
		}
		template< UPtr _size_ >
		inline UPtr AppendN( char ( &dst )[ _size_ ], const char *src,
		UPtr n ) {
			return AppendN( &dst[ 0 ], _size_, src, n );
		}

		template< UPtr _size_ >
		inline UPtr FormatV( char ( &dst )[ _size_ ], const char *fmt,
		va_list args ) {
			return FormatV( &dst[ 0 ], _size_, fmt, args );
		}
		template< UPtr _size_ >
		inline UPtr Format( char ( &dst )[ _size_ ], const char *fmt, ... ) {
			va_list args;

			va_start( args, fmt );
			const UPtr r = FormatV( &dst[ 0 ], _size_, fmt, args );
			va_end( args );

			return r;
		}

	}

}

#endif //AXSDK_UTILITY_TEXT_HPP


#ifndef SECURE_LIB_ENABLED
# if defined( _MSC_VER ) && defined( __STDC_WANT_SECURE_LIB__ )
#  define SECURE_LIB_ENABLED 1
# else
#  define SECURE_LIB_ENABLED 0
# endif
#endif

namespace ax {

	namespace Text {

		UPtr Copy( char *dst, UPtr maxDst, const char *src ) {
			AX_ASSERT_NOT_NULL( dst );
			AX_ASSERT( maxDst > 0 );

			if( !src ) {
				*dst = '\0';
				return 0;
			}
			
			return axstr_cpy( dst, maxDst, src );
		}
		UPtr CopyN( char *dst, UPtr maxDst, const char *src, UPtr n ) {
			AX_ASSERT_NOT_NULL( dst );
			AX_ASSERT( maxDst > 0 );

			if( !src ) {
				*dst = '\0';
				return 0;
			}

			return axstr_cpyn( dst, maxDst, src, n );
		}

		UPtr Append( char *dst, UPtr maxDst, const char *src ) {
			AX_ASSERT_NOT_NULL( dst );
			AX_ASSERT( maxDst > 0 );

			if( !src ) {
				return strlen( dst );
			}
			
			return axstr_cat( dst, maxDst, src );
		}
		UPtr AppendN( char *dst, size_t maxDst, const char *src, UPtr n ) {
			AX_ASSERT_NOT_NULL( dst );
			AX_ASSERT( maxDst > 0 );

			if( !src ) {
				return strlen( dst );
			}

			return axstr_catn( dst, maxDst, src, n );
		}

		Void ConvertLower( char *buf ) {
			AX_ASSERT_NOT_NULL( buf );

			while( *buf != '\0' ) {
				*buf = ToLower( *buf );
				++buf;
			}
		}
		Void ConvertUpper( char *buf ) {
			AX_ASSERT_NOT_NULL( buf );

			while( *buf != '\0' ) {
				*buf = ToUpper( *buf );
				++buf;
			}
		}
		Void ConvertLowerN( char *buf, UPtr n ) {
			AX_ASSERT_NOT_NULL( buf );

			while( *buf != '\0' && n > 0 ) {
				*buf = ToLower( *buf );
				++buf;
				--n;
			}
		}
		Void ConvertUpperN( char *buf, UPtr n ) {
			AX_ASSERT_NOT_NULL( buf );

			while( *buf != '\0' && n > 0 ) {
				*buf = ToUpper( *buf );
				++buf;
				--n;
			}
		}

		Bool Compare( const char *a, const char *b ) {
			AX_ASSERT_NOT_NULL( a );
			AX_ASSERT_NOT_NULL( b );

			return strcmp( a, b ) == 0;
		}
		Bool CompareInsensitive( const char *a, const char *b ) {
			AX_ASSERT_NOT_NULL( a );
			AX_ASSERT_NOT_NULL( b );
			
#if defined( _WIN32 )
			return _stricmp( a, b ) == 0;
#else
			return strcasecmp( a, b ) == 0;
#endif
		}

		Bool CompareN( const char *a, const char *b, UPtr n ) {
			AX_ASSERT_NOT_NULL( a );
			AX_ASSERT_NOT_NULL( b );
			
			return strncmp( a, b, n ) == 0;
		}
		Bool CompareInsensitiveN( const char *a, const char *b, UPtr n ) {
			AX_ASSERT_NOT_NULL( a );
			AX_ASSERT_NOT_NULL( b );
			
#if defined( _WIN32 )
			return _strnicmp( a, b, n ) == 0;
#else
			return strncasecmp( a, b, n ) == 0;
#endif
		}

		UPtr FormatV( char *dst, UPtr maxDst, const char *fmt, va_list args ) {
			AX_ASSERT_NOT_NULL( dst );
			AX_ASSERT_NOT_NULL( fmt );
			AX_ASSERT( maxDst > 0 );

			auto r = axspfv( dst, maxDst, fmt, args );

			if( r < 0 ) {
				return 0;
			}

			return ( UPtr )r;
		}

	}

}

/*
===============================================================================

	PARSING (BASE)

===============================================================================
*/

namespace Parsing {

	using namespace ax;
	using namespace doll;

	UPtr SkipSpaces( const char *&p ) {
		AX_ASSERT_NOT_NULL( p );

		const char *const base = p;

		while( *( const U8 * )p <= ' ' && *p != '\n' && *p != '\0' ) {
			++p;
		}

		return p - base;
	}

	UPtr SkipLine( const char *&p ) {
		AX_ASSERT_NOT_NULL( p );

		const char *const base = p;

		while( *p != '\n' && *p != '\0' ) {
			if( *p == '\r' ) {
				++p;
				break;
			}

			++p;
		}

		if( *p == '\n' ) {
			++p;
		}

		return p - base;
	}

	UPtr SkipNonwhite( const char *&p ) {
		AX_ASSERT_NOT_NULL( p );

		const char *const base = p;

		while( *( const U8 * )p > ' ' ) {
			++p;
		}

		return p - base;
	}
	UPtr SkipQuote( const char *&p ) {
		AX_ASSERT_NOT_NULL( p );

		if( *p != '\"' ) {
			return 0;
		}

		const char *const base = p;
		
		do {
			++p;
		} while( *p != '\"' && *p != '\0' );

		return p - base;
	}

	Void CalculateLineInfo( SConfigLineInfo &dst, Str src, const char *ptr )
	{
		AX_ASSERT_NOT_NULL( ptr );

		const char *const s = src.get();
		const char *const e = src.getEnd();

		const char *p = s;

		dst.line = 1;
		while( p < ptr && p < e ) {
			if( *p != '\n' ) {
				if( *p != '\r' ) {
					++p;
					continue;
				}

				if( *( p + 1 ) == '\n' ) {
					++p;
				}
			}

			++p;
			++dst.line;
			dst.lineOffset = p - s;
		}

		dst.fileOffset = p - s;
		dst.column = dst.fileOffset - dst.lineOffset + 1;
	}

}

/*
===============================================================================

	CONFIGURATION

===============================================================================
*/

namespace doll
{

#if 0
	static const char *skipEmptyLines( const char *p, const char *e )
	{
		if( !e ) {
			while( *( const U8 * )p <= ' ' && *p != '\0' ) {
				++p;
			}
		} else {
			while( p < e && *( const U8 * )p <= ' ' ) {
				++p;
			}
		}

		return p;
	}
#endif

	CConfiguration::CConfiguration()
	: mIncludeDepth( 0 )
	{
	}
	CConfiguration::~CConfiguration()
	{
		clear();
	}

	Void CConfiguration::clear()
	{
		while( mSections.isUsed() ) {
			removeVar( mSections.head() );
		}
	}

	Bool CConfiguration::loadFromMemory( Str filename, Str buffer )
	{
		//
		//	NOTE: Purposely not cleaning up when an error occurs
		//	-     Anything loaded up before the error can still be used, and it's
		//	-     better (for the end user) to at least get something
		//	-     This class cleans up after itself in the destructor
		//

		AX_ASSERT( filename.isUsed() );

		const char *p = buffer.get();
		const char *const fe = buffer.getEnd();

		Bool isSpecialSection = false;

		SConfigVar *rootSection = nullptr;
		rootSection = findVar( nullptr, "" );
		if( !rootSection ) {
			rootSection = addVar( nullptr, "" );
			if( !rootSection ) {
				return false;
			}
		}

		SConfigVar *section = rootSection;

		while( p < fe ) {
			p = axstr_skip_whitespace_e( p, fe );
			if( !p || p == fe ) {
				break;
			}

			// skip comments and blank lines
			if( *p == ';' || *p == '\r' || *p == '\n' ) {
				p = axstr_skip_line_e( p + 1, fe );
				continue;
			}

			// start of line
			const char *line_p = p;

			// is this a section?
			if( *p == '[' ) {
				++p;

				const char *s = p;
				const char *e = Str( p, fe ).findPtr( ']' );

				// make sure there's a matching close bracket
				if( !e ) {
					warnRaw( filename, buffer, p, "Expected matching ']'" );

					p = axstr_skip_line_e( p, fe );
					continue;
				}

				// note the location of the first character after that bracket
				const char *next_p = e + 1;

				// make sure there's no newline inside the region
				const char *const q = Str( s, e ).findPtr( '\n' );
				if( q != nullptr ) {
					warnRaw( filename, buffer, q, "Unexpected newline" );

					p = q + 1;
					continue;
				}

				// trim leading whitespace
				while( s < e && *( const U8 * )s <= ' ' ) {
					++s;
				}

				// trim trailing whitespace
				while( e > s && *( const U8 * )( e - 1 ) <= ' ' ) {
					--e;
				}

				// ensure the section name wasn't entirely whitespace
				if( e - s == 0 ) {
					warnRaw( filename, buffer, p, "Expected section name" );

					p = axstr_skip_line_e( p, fe );
					continue;
				}

				// save the section
				const Str sectName( s, e );

				// is this the special section? ("Configuration")
				isSpecialSection = sectName.caseCmp( "Configuration" );

				if( isSpecialSection ) {
					section = nullptr;
				} else {
					// check to see if the section exists
					section = findVar( nullptr, sectName );
					if( !section ) {
						// create the section
						section = addVar( nullptr, sectName );
						if( !section ) {
							errorRaw( filename, buffer, p, "Failed to allocate section" );
							return false;
						}
					}
				}

				// go to the just after the close bracket
				p = next_p;

				// find the next newline
				const char *const next_nl = axstr_skip_line_e( p, fe );

				// ensure that the only characters between here and the next newline
				// are whitespace (or comment)
				while( p < next_nl ) {
					// comment?
					if( *p == ';' ) {
						p = next_nl;
						break;
					}

					if( *( const U8 * )p > ' ' ) {
						break;
					}

					++p;
				}

				if( p < next_nl ) {
					warnRaw( filename, buffer, p, "Garbage at end of section declaration" );
				}

				// done with this line
				p = next_nl;
				continue;
			}

			if( !isSpecialSection && section == nullptr ) {
				errorRaw( filename, buffer, p, "Invalid internal state" );
				return false;
			}

			// set the default processing mode
			EProcessMode mode = EProcessMode::Set;

			// determine the processing mode if provided
			if( p + 1 < fe ) {
				if( *p == '+' ) {
					++p;
					mode = EProcessMode::Add;
				} else if( *p == '.' ) {
					++p;
					mode = EProcessMode::AddUnique;
				} else if( *p == '-' ) {
					++p;
					mode = EProcessMode::RemoveExact;
				} else if( *p == '!' ) {
					++p;
					mode = EProcessMode::RemoveInexact;
				}
			}

			// find the range of the key
			const char *key_s = axstr_skip_whitespace_e( p, fe );
			if( p == fe || *p == ';' ) {
				p = axstr_skip_line_e( p, fe );
				continue;
			}

			const char *key_e = Str( p, fe ).findPtr( '=' );

			// ensure an equal sign was provided or the mode was "RemoveInexact"
			if( !key_e ) {
				// not inexact mode, so not allowed
				if( mode != EProcessMode::RemoveInexact ) {
					warnRaw( filename, buffer, p, "Expected '=' after key" );

					p = axstr_skip_line_e( p, fe );
					continue;
				}

				// find the end of the key
				key_e = axstr_skip_line_e( p, fe );
			}

			// store the start of the next token
			const char *next_p = key_e;

			// trim whitespace
			while( key_e > key_s && *( const U8 * )( key_e - 1 ) <= ' ' ) {
				--key_e;
			}

			// check for an invalid value
			if( key_e - key_s == 0 ) {
				if( p < fe && *p == '=' ) {
					warnRaw( filename, buffer, p, "Expected key before '='" );
				} else {
					warnRaw( filename, buffer, p, "Expected key" );
				}

				( Void )Parsing::SkipLine( p );
				continue;
			}

			// store the name of the key
			const Str key( key_s, key_e );

			// go to the next location
			p = next_p;

			// value buffer
			Str value;

			// read the value
			if( p + 1 < fe && *p == '=' ) {
				++p;

				const char *value_s = p;
				while( p < fe && *p != '\n' ) {
					++p;
				}
				const char *value_e = p;

				// trim spaces
				while( value_e > value_s && *( const U8 * )( value_e - 1 ) <= ' ' ) {
					--value_e;
				}
				while( value_s < value_e && *( const U8 * )value_s <= ' ' ) {
					++value_s;
				}

				// add the value to the buffer
				value = Str( value_s, value_e );
			}

			// attempt to process
			process( section, mode, key, value, filename, buffer, line_p );
		}

		// success!
		return true;
	}
	Bool CConfiguration::loadFromFile( Str filename )
	{
#if 0
		U8 *pMem = nullptr;
		UPtr cMem = 0;

		if( !core_loadFile( filename, pMem, cMem ) ) {
			return false;
		}

		return loadFromMemory( filename, Str( ( const char * )pMem, cMem ) );
#else
		MutStr txt;

		if( !core_readText( txt, filename ) ) {
			return false;
		}

		return loadFromMemory( filename, txt );
#endif
	}

	Void CConfiguration::printVars()
	{
		for( const auto *prnt = mSections.head(); prnt != nullptr; prnt = prnt->sibling.next() ) {
			printVar( *prnt );
		}
	}
	Void CConfiguration::printVar( const SConfigVar &var )
	{
		char buf[ 2048 ];

		const UPtr  nn = var.name.len();
		const char *np = var.name.get();

		if( var.parent != nullptr ) {
			char pre[ 256 ];

			pre[ 0 ] = '\0';
			const SConfigVar *prnt = var.parent->parent;
			while( prnt != nullptr ) {
				ax::Text::Append( pre, "  " );
				prnt = prnt->parent;
			}

			const SConfigValue *val = var.values.head();

			if( val != var.values.tail() ) {
				ax::Text::Format( buf, "%s%.*s:", pre, nn,np );

				char tmp[ 512 ];
				while( val != nullptr ) {
					auto *next = val->link.next();

					if( val->value.isEmpty() ) {
						val = next;
						continue;
					}

					ax::Text::Format( tmp, "\n%s  %.*s", pre, val->value.len(), val->value.get() );
					ax::Text::Append( buf, tmp );

					val = next;
				}
			} else if( val != nullptr && val->value.isUsed() ) {
				ax::Text::Format( buf, "%s%.*s = %.*s", pre, nn,np, val->value.len(), val->value.get() );
			} else {
				ax::Text::Format( buf, "%s%.*s", pre, nn,np );
			}
		} else {
			ax::Text::Format( buf, "[%.*s]", nn,np );
		}

		g_InfoLog += buf;

		for( const auto *chld = var.children.head(); chld != nullptr; chld = chld->sibling.next() ) {
			printVar( *chld );
		}
	}

	SConfigVar *CConfiguration::addVar( SConfigVar *parent, Str name )
	{
		SConfigVar *var;

#if AX_DEBUG_ENABLED || AX_TEST_ENABLED
		// this should have a unique name
		var = findVar( parent, name );
		AX_ASSERT_MSG( var == NULL, "Variable already exists" );
#endif

		var = DOLL_NEW( SConfigVar );
		if( !AX_VERIFY_MEMORY( var ) ) {
			return nullptr;
		}

		if( !AX_VERIFY_MEMORY( var->name.tryAssign( name ) ) ) {
			DOLL_DELETE( var );
			return nullptr;
		}

		var->sibling.setNode( var );

		var->parent = parent;
		if( parent != nullptr ) {
			parent->children.addTail( var->sibling );
		} else {
			mSections.addTail( var->sibling );
		}

		var->config = this;

		return var;
	}
	SConfigVar *CConfiguration::findVar( SConfigVar *parent, Str name )
	{
		const TIntrList< SConfigVar > &list = ( parent != nullptr ) ? parent->children : mSections;

		for( const auto *var = list.head(); var != nullptr; var = var->sibling.next() ) {
			if( var->name.caseCmp( name ) ) {
				return const_cast< SConfigVar * >( var );
			}
		}

		return nullptr;
	}
	Void CConfiguration::removeVar( SConfigVar *var )
	{
		if( !var ) {
			return;
		}

		while( var->children.isUsed() ) {
			removeVar( var->children.head() );
		}

		TIntrList< SConfigVar > &list = ( var->parent != nullptr ) ? var->parent->children : mSections;
		list.unlink( var->sibling );

		while( var->values.isUsed() ) {
			removeValue( var, var->values.head() );
		}

		DOLL_DELETE( var );
	}

	SConfigValue *CConfiguration::addValue( SConfigVar *var, Str value )
	{
		AX_ASSERT_NOT_NULL( var );
		AX_ASSERT( value.isUsed() );

		SConfigValue *const val = DOLL_NEW( SConfigValue );
		if( !AX_VERIFY_MEMORY( val ) ) {
			return nullptr;
		}

		if( !AX_VERIFY_MEMORY( val->value.tryAssign( value ) ) ) {
			return DOLL_DELETE( val );
		}

		val->link.setNode( val );
		var->values.addTail( val->link );

		return val;
	}
	SConfigValue *CConfiguration::findValue( SConfigVar *var, Str value ) {
		AX_ASSERT_NOT_NULL( var );
		AX_ASSERT( value.isUsed() );

		const SConfigValue *val = var->values.head();
		while( val != nullptr ) {
			const auto *next = val->link.next();

			if( val->value.isEmpty() ) {
				val = next;
				continue;
			}

			if( val->value == value ) {
				return const_cast< SConfigValue * >( val );
			}

			val = next;
		}

		return nullptr;
	}
	Void CConfiguration::removeValue( SConfigVar *var, SConfigValue *val ) {
		AX_ASSERT_NOT_NULL( var );
		AX_ASSERT_NOT_NULL( val );

		var->values.unlink( val->link );
		DOLL_DELETE( val );

#if 0
		if( var->values.isEmpty() ) {
			removeVar( var );
		}
#endif
	}
	Void CConfiguration::removeAllValues( SConfigVar *var ) {
		AX_ASSERT_NOT_NULL( var );

		while( var->values.isUsed() ) {
			removeValue( var, var->values.head() );
		}
	}

	Void CConfiguration::process( SConfigVar *parent, EProcessMode mode, Str name, Str value, Str filename, Str buffer, const char *ptr )
	{
		AX_ASSERT_NOT_NULL( ptr );

		// special handling
		if( !parent ) {
			if( name.caseCmp( "BasedOn" ) == 0 ) {
				if( mIncludeDepth == 16 ) {
					warnRaw( filename, buffer, ptr, "Include depth too deep" );
					return;
				}

#ifdef _WIN32
				wchar_t curDir[ 270 ];
				if( !GetCurrentDirectoryW( sizeof(curDir)/sizeof(curDir[0]), &curDir[ 0 ] ) ) {
					curDir[ 0 ] = L'\0';
				}

				Str relDir( filename.getDirectory() );
				wchar_t wszRelDir[ 512 ];

				if( filename.getDirectory().toWStr( wszRelDir ) != nullptr ) {
					SetCurrentDirectoryW( wszRelDir );
				} else {
					char szRelDir[ 512 ];
					axstr_cpyn( szRelDir, relDir.get(), relDir.len() );
					SetCurrentDirectoryA( szRelDir );
				}
#else
				char curDir[ AXSTR_MAX_PATH ];
				if( !getcwd( curDir, sizeof(curDir)/sizeof(curDir[0]) ) ) {
					curDir[ 0 ] = '\0';
				}

				char relDir[ AXSTR_MAX_PATH ];
				axstr_cpy( relDir, filename.getDirectory() );

				chdir( relDir );
#endif

				++mIncludeDepth;
				const Bool r = loadFromFile( value );
				--mIncludeDepth;

#ifdef _WIN32
				if( curDir[ 0 ] != L'\0' ) {
					SetCurrentDirectoryW( curDir );
				}
#else
				if( curDir[ 0 ] != '\0' ) {
					chdir( curDir );
				}
#endif

				if( !r ) {
					char msg[ 512 ];

					axspf( msg, "Failed to load file: \"%.*s\"", value.len(), value.get() );
					warnRaw( filename, buffer, ptr, msg );
				}

				return;
			}

			warnRaw( filename, buffer, ptr, "Unrecognized command in special section 'Configuration'" );
			return;
		}

		// check if the variable exists
		SConfigVar *var = findVar( parent, name );
		SConfigValue *val = nullptr;

		// handle the mode
		switch( mode ) {
		case EProcessMode::Set:
			if( !var ) {
				var = addVar( parent, name );
			}

			removeAllValues( var );
			( Void )addValue( var, value );
			break;

		case EProcessMode::Add:
			if( !var ) {
				var = addVar( parent, name );
			}

			( Void )addValue( var, value );
			break;

		case EProcessMode::AddUnique:
			if( !var ) {
				var = addVar( parent, name );
			}

			val = findValue( var, value );
			if( !val ) {
				( Void )addValue( var, value );
			}
			break;

		case EProcessMode::RemoveExact:
			if( !var ) {
				break;
			}

			val = findValue( var, value );
			if( !val ) {
				break;
			}

			removeValue( var, val );
			var = nullptr;
			break;

		case EProcessMode::RemoveInexact:
			if( !var ) {
				break;
			}

			val = findValue( var, value );
			if( !val ) {
				val = var->values.head();
				if( !val ) {
					break;
				}
			}

			removeValue( var, val );
			var = nullptr;
			break;
		}
	}

	Void CConfiguration::error( const SConfigLineInfo &linfo, const char *message )
	{
		AX_ASSERT_NOT_NULL( message );

		g_ErrorLog( linfo.filename, (U32)linfo.line ) += message;
	}
	Void CConfiguration::errorRaw( Str filename, Str buffer, const char *p, const char *message )
	{
		SConfigLineInfo linfo;

		AX_ASSERT( filename.isUsed() );
		AX_ASSERT_NOT_NULL( p );
		AX_ASSERT_NOT_NULL( message );

		Parsing::CalculateLineInfo( linfo, buffer, p );
		linfo.filename = filename;

		warn( linfo, message );
	}

	Void CConfiguration::warn( const SConfigLineInfo &linfo, const char *message )
	{
		AX_ASSERT_NOT_NULL( message );

		g_WarningLog( linfo.filename, (U32)linfo.line ) += message;
	}
	Void CConfiguration::warnRaw( Str filename, Str buffer, const char *p, const char *message )
	{
		SConfigLineInfo linfo;

		AX_ASSERT( filename.isUsed() );
		AX_ASSERT_NOT_NULL( p );
		AX_ASSERT_NOT_NULL( message );

		Parsing::CalculateLineInfo( linfo, buffer, p );
		linfo.filename = filename;

		warn( linfo, message );
	}

	//--------------------------------------------------------------------//

#define EXPECT_CONFIG( cfg )\
	if( !AX_VERIFY_NOT_NULL( cfg ) )

	DOLL_FUNC CConfiguration *DOLL_API core_newConfig()
	{
		CConfiguration *const config = DOLL_NEW( CConfiguration );
		if( !AX_VERIFY_MEMORY( config ) ) {
			return nullptr;
		}

		return config;
	}
	DOLL_FUNC CConfiguration *DOLL_API core_deleteConfig( CConfiguration *config )
	{
		return DOLL_DELETE( config );
	}

	DOLL_FUNC CConfiguration *DOLL_API core_loadConfig( Str filename )
	{
		CConfiguration *config = core_newConfig();
		if( !config ) {
			return nullptr;
		}

		if( !config->loadFromFile( filename ) ) {
			return core_deleteConfig( config );
		}

		return config;
	}
	DOLL_FUNC Bool DOLL_API core_appendConfig( CConfiguration *config, Str filename )
	{
		EXPECT_CONFIG( config ) {
			return false;
		}
		if( !AX_VERIFY( filename.isUsed() ) ) {
			return false;
		}

		return config->loadFromFile( filename );
	}
	DOLL_FUNC Bool DOLL_API core_appendConfigString( CConfiguration *config, Str filename, Str source )
	{
		EXPECT_CONFIG( config ) {
			return false;
		}
		if( !AX_VERIFY( filename.isUsed() ) || !AX_VERIFY( source.isUsed() ) ) {
			return false;
		}

		return config->loadFromMemory( filename, source );
	}

	DOLL_FUNC Bool DOLL_API core_queryConfigValue( CConfiguration *config, Str &out_value, Str section, Str key )
	{
		EXPECT_CONFIG( config ) {
			return false;
		}
		if( !AX_VERIFY( section.isUsed() ) || !AX_VERIFY( key.isUsed() ) ) {
			return false;
		}

		SConfigVar *const sect = config->findVar( nullptr, section );
		if( !sect ) {
			return false;
		}

		SConfigVar *const var = config->findVar( sect, key );
		if( !var ) {
			return false;
		}

		out_value = core_getConfigVarValue( var );
		return true;
	}

	DOLL_FUNC Void DOLL_API core_clearConfig( CConfiguration *config )
	{
		EXPECT_CONFIG( config ) {
			return;
		}

		config->clear();
	}

	DOLL_FUNC SConfigVar *DOLL_API core_findConfigSection( CConfiguration *config, Str name )
	{
		EXPECT_CONFIG( config ) {
			return nullptr;
		}
		if( !AX_VERIFY( name.isUsed() ) ) {
			return nullptr;
		}

		return config->findVar( nullptr, name );
	}
	DOLL_FUNC SConfigVar *DOLL_API core_findConfigVar( SConfigVar *prnt, Str name )
	{
		if( !AX_VERIFY_NOT_NULL( prnt ) || !AX_VERIFY( name.isUsed() ) ) {
			return nullptr;
		}

		AX_ASSERT_NOT_NULL( prnt->config );

		return prnt->config->findVar( prnt, name );
	}
	DOLL_FUNC SConfigValue *DOLL_API core_findConfigVarValue( SConfigVar *var, Str value )
	{
		if( !AX_VERIFY_NOT_NULL( var ) || !AX_VERIFY( value.isUsed() ) ) {
			return nullptr;
		}

		AX_ASSERT_NOT_NULL( var->config );

		return var->config->findValue( var, value );
	}

	DOLL_FUNC SConfigVar *DOLL_API core_getFirstConfigSection( CConfiguration *config )
	{
		EXPECT_CONFIG( config ) {
			return nullptr;
		}

		return config->head();
	}
	DOLL_FUNC SConfigVar *DOLL_API core_getLastConfigSection( CConfiguration *config )
	{
		EXPECT_CONFIG( config ) {
			return nullptr;
		}

		return config->tail();
	}
	DOLL_FUNC SConfigVar *DOLL_API core_getFirstConfigVar( SConfigVar *prnt )
	{
		if( !AX_VERIFY_NOT_NULL( prnt ) ) {
			return nullptr;
		}

		return prnt->children.head();
	}
	DOLL_FUNC SConfigVar *DOLL_API core_getLastConfigVar( SConfigVar *prnt )
	{
		if( !AX_VERIFY_NOT_NULL( prnt ) ) {
			return nullptr;
		}

		return prnt->children.tail();
	}
	DOLL_FUNC SConfigVar *DOLL_API core_getConfigVarBefore( SConfigVar *var )
	{
		if( !AX_VERIFY_NOT_NULL( var ) ) {
			return nullptr;
		}

		return var->sibling.prev();
	}
	DOLL_FUNC SConfigVar *DOLL_API core_getConfigVarAfter( SConfigVar *var )
	{
		if( !AX_VERIFY_NOT_NULL( var ) ) {
			return nullptr;
		}

		return var->sibling.next();
	}
	DOLL_FUNC SConfigVar *DOLL_API core_getConfigVarParent( SConfigVar *var )
	{
		if( !AX_VERIFY_NOT_NULL( var ) ) {
			return nullptr;
		}

		return var->parent;
	}

	DOLL_FUNC Bool DOLL_API core_getConfigVarName( Str &dst, SConfigVar *var )
	{
		if( !AX_VERIFY_NOT_NULL( var ) ) {
			dst = Str();
			return false;
		}

		dst = var->name;
		return true;
	}
	DOLL_FUNC Bool DOLL_API core_getConfigVarValue( Str &dst, SConfigVar *var )
	{
		if( !AX_VERIFY_NOT_NULL( var ) ) {
			dst = Str();
			return false;
		}

		static MutStr r;

		r.clear();

		const auto *val = var->values.head();
		while( val != nullptr ) {
			const auto *next = val->link.next();

			if( !val->value ) {
				val = next;
				continue;
			}

			r.append( val->value );
			if( next != nullptr ) {
				r.append( Str( '\n' ) );
			}

			val = next;
		}

		dst = r;
		return true;
	}

	DOLL_FUNC SConfigValue *DOLL_API core_getFirstConfigVarValue( SConfigVar *var )
	{
		if( !AX_VERIFY_NOT_NULL( var ) ) {
			return nullptr;
		}

		return var->values.head();
	}
	DOLL_FUNC SConfigValue *DOLL_API core_getLastConfigVarValue( SConfigVar *var )
	{
		if( !AX_VERIFY_NOT_NULL( var ) ) {
			return nullptr;
		}

		return var->values.tail();
	}
	DOLL_FUNC SConfigValue *DOLL_API core_getConfigValueBefore( SConfigValue *val )
	{
		if( !AX_VERIFY_NOT_NULL( val ) ) {
			return nullptr;
		}

		return val->link.prev();
	}
	DOLL_FUNC SConfigValue *DOLL_API core_getConfigValueAfter( SConfigValue *val )
	{
		if( !AX_VERIFY_NOT_NULL( val ) ) {
			return nullptr;
		}

		return val->link.next();
	}
	DOLL_FUNC Bool DOLL_API core_getConfigValueString( Str &dst, SConfigValue *val )
	{
		if( !AX_VERIFY_NOT_NULL( val ) ) {
			dst = Str();
			return false;
		}

		dst = val->value;
		return true;
	}

	DOLL_FUNC SConfigVar *DOLL_API core_removeConfigVar( SConfigVar *var )
	{
		if( !var ) {
			return nullptr;
		}

		AX_ASSERT_NOT_NULL( var->config );

		var->config->removeVar( var );
		return nullptr;
	}

}
