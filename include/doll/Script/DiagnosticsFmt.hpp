#pragma once

#include "../Core/Defs.hpp"

namespace doll { namespace script {

	class Ident;
	class Diagnostic;

	enum class EDiagnosticArgumentKind
	{
		String,
		IntegerValue,
		UnsignedValue,
		FloatValue,
		Identifier,
		Codepoint
	};

	template< typename T >
	struct TDiagnosticArgKind
	{
	};

	template<> struct TDiagnosticArgKind< Str >
	{
		static const EDiagnosticArgumentKind kind = EDiagnosticArgumentKind::String;
	};
	template<> struct TDiagnosticArgKind< S64 >
	{
		static const EDiagnosticArgumentKind kind = EDiagnosticArgumentKind::IntegerValue;
	};
	template<> struct TDiagnosticArgKind< U64 >
	{
		static const EDiagnosticArgumentKind kind = EDiagnosticArgumentKind::UnsignedValue;
	};
	template<> struct TDiagnosticArgKind< F64 >
	{
		static const EDiagnosticArgumentKind kind = EDiagnosticArgumentKind::FloatValue;
	};
	template<> struct TDiagnosticArgKind< const Ident * >
	{
		static const EDiagnosticArgumentKind kind = EDiagnosticArgumentKind::Identifier;
	};
	template<> struct TDiagnosticArgKind< U32 >
	{
		static const EDiagnosticArgumentKind kind = EDiagnosticArgumentKind::Codepoint;
	};

	class DiagnosticArgument
	{
	public:
		DiagnosticArgument( const DiagnosticArgument &x )
		: m_kind( x.m_kind )
		, m_stringValue( x.m_stringValue )
		{
		}
		DiagnosticArgument( Str x )
		: m_kind( EDiagnosticArgumentKind::String )
		, m_stringValue( x )
		{
		}
		DiagnosticArgument( S64 x )
		: m_kind( EDiagnosticArgumentKind::IntegerValue )
		, m_integerValue( x )
		{
		}
		DiagnosticArgument( U64 x )
		: m_kind( EDiagnosticArgumentKind::UnsignedValue )
		, m_unsignedValue( x )
		{
		}
		DiagnosticArgument( F64 x )
		: m_kind( EDiagnosticArgumentKind::FloatValue )
		, m_floatValue( x )
		{
		}
		DiagnosticArgument( Ident *pIdent )
		: m_kind( EDiagnosticArgumentKind::Identifier )
		, m_pIdentValue( pIdent )
		{
		}
		DiagnosticArgument( U32 utf32Codepoint )
		: m_kind( EDiagnosticArgumentKind::Codepoint )
		, m_codepointValue( utf32Codepoint )
		{
		}

		EDiagnosticArgumentKind kind() const
		{
			return m_kind;
		}
		Bool is( EDiagnosticArgumentKind kind ) const
		{
			return m_kind == kind;
		}

		const Str &asString() const
		{
			AX_ASSERT( m_kind == EDiagnosticArgumentKind::String );
			return m_stringValue;
		}
		S64 asInteger() const
		{
			AX_ASSERT( m_kind == EDiagnosticArgumentKind::IntegerValue );
			return m_integerValue;
		}
		U64 asUnsigned() const
		{
			AX_ASSERT( m_kind == EDiagnosticArgumentKind::UnsignedValue );
			return m_unsignedValue;
		}
		F64 asFloat() const
		{
			AX_ASSERT( m_kind == EDiagnosticArgumentKind::FloatValue );
			return m_floatValue;
		}
		const Ident *asIdent() const
		{
			AX_ASSERT( m_kind == EDiagnosticArgumentKind::Identifier );
			return m_pIdentValue;
		}
		U32 asCodepoint() const
		{
			AX_ASSERT( m_kind == EDiagnosticArgumentKind::Codepoint );
			return m_codepointValue;
		}

		Bool appendToString( MutStr &dst, Bool isPlural ) const;

	private:
		EDiagnosticArgumentKind m_kind;
		union
		{
			Str          m_stringValue;
			S64          m_integerValue;
			U64          m_unsignedValue;
			F64          m_floatValue;
			const Ident *m_pIdentValue;
			U32          m_codepointValue;
		};
	};

	Bool scr_formatDiagnostic( MutStr &dst, Str fmt, TArr<DiagnosticArgument> args );

}}
