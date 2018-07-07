#include "../BuildSettings.hpp"

#include "doll/Script/DiagnosticsFmt.hpp"
#include "doll/Script/Ident.hpp"

namespace doll { namespace script {

	Bool DiagnosticArgument::appendToString( MutStr &dst, Bool isPlural ) const
	{
		char szBuf[ 256 ];
		Bool r = true;

		// FIXME: Use `isPlural`
		((void)isPlural);

		switch( m_kind ) {
		case EDiagnosticArgumentKind::String:
			r = r && dst.tryAppend( '\"' );
			r = r && dst.tryAppend( m_stringValue );
			r = r && dst.tryAppend( '\"' );
			break;

		case EDiagnosticArgumentKind::IntegerValue:
			r = r && dst.tryAppend( axff( szBuf, "%I64i", m_integerValue ) );
			break;

		case EDiagnosticArgumentKind::UnsignedValue:
			r = r && dst.tryAppend( axff( szBuf, "%I64u", m_unsignedValue ) );
			break;

		case EDiagnosticArgumentKind::FloatValue:
			r = r && dst.tryAppend( axff( szBuf, "%g", m_floatValue ) );
			break;

		case EDiagnosticArgumentKind::Identifier:
			if( !m_pIdentValue ) {
				r = r && dst.tryAppend( "(missing-identifier)" );
			} else if( m_pIdentValue->name.isUsed() ) {
				r = r && dst.tryAppend( '\'' );
				r = r && dst.tryAppend( m_pIdentValue->name );
				r = r && dst.tryAppend( '\'' );
			} else {
				r = r && dst.tryAppend( "(invalid-identifier)" );
				AX_ASSERT_MSG( false, "Invalid identifier" );
			}
			break;

		case EDiagnosticArgumentKind::Codepoint:
			// FIXME: If codepoint is printable then display the codepoint next to the value
			r = r && dst.tryAppend( axff( szBuf, "U+%.4X", m_codepointValue ) );
			break;
		}

		return r;
	}

	static Str extractUnformattedRange( Str &fmt )
	{
		axstr_ptrdiff_t i = fmt.find( '%' );
		if( i < 0 ) {
			i = axstr_ptrdiff_t( fmt.len() );
		}

		return fmt.extractLeft( i );
	}
	Bool scr_formatDiagnostic( MutStr &dst, Str fmt, TArr<DiagnosticArgument> args )
	{
		AX_ASSERT( args.num() < 10 );

		Str s( fmt );
		for(;;) {
			const Str range( extractUnformattedRange( s ) );
			if( range.isUsed() && !AX_VERIFY_MEMORY( dst.tryAppend( range ) ) ) {
				return false;
			}

			if( s.isEmpty() ) {
				break;
			}

			AX_ASSERT( s.startsWith( '%' ) );

			s = s.skip();

			const axstr_utf32_t chSpec = s.readChar();
			switch( chSpec ) {
			case '%':
				if( !AX_VERIFY_MEMORY( dst.tryAppend( '%' ) ) ) {
					return false;
				}
				break;

			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				{
					const Bool isPlural = s.checkByte( 's' );
					const UPtr index = UPtr( chSpec - '0' );

					if( index < args.num() ) {
						if( !AX_VERIFY_MEMORY( args[ index ].appendToString( dst, isPlural ) ) ) {
							return false;
						}
					} else if( !AX_VERIFY_MEMORY( dst.tryAppend( "(null)" ) ) ) {
						return false;
					}
				}
				break;
			}
		}

		return true;
	}

}}
