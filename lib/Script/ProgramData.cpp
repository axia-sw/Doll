#include "../BuildSettings.hpp"

#include "doll/Script/ProgramData.hpp"
#include "doll/Util/Hash.hpp"

namespace doll { namespace script {
	
	CProgramData::CProgramData()
	: m_strings()
	, m_blob()
	{
	}
	CProgramData::~CProgramData()
	{
	}

	Bool CProgramData::addString( const Str &s )
	{
		const UPtr uOff = m_blob.num();
		AX_ASSERT( uOff < 0xFFFFFFFF );

		if( s.len() + 1 > 0xFFFF ) {
			// FIXME: ERROR: STRING IS LARGER THAN 64K!
			return false;
		}

		if( uOff + s.len() + 1 > 0xFFFFFFFF ) {
			// FIXME: ERROR: MORE THAN 4GB OF STRING DATA!
			return false;
		}

		const U64 uA = hashCRC32( s );
		const U64 uB = hashSimple( s );

		const U64 uHash = ( ( uA & 0xFFFF0000 )<<32 ) | ( uB << 16 ) | ( uA & 0x0000FFFF );

		axpf( "<<\"%.*s\":%.16llX\n", s.lenInt(), s.get(), uHash );

		// FIXME: Use better search
		for( UPtr i = 0; i < m_strings.num(); ++i ) {
			if( m_strings[ i ].uHash == uHash ) {
				return true;
			}
		}

		if( !AX_VERIFY_MEMORY( m_blob.append( s.len(), ( const U8 * )s.get() ) ) ) {
			return false;
		}
		if( !AX_VERIFY_MEMORY( m_blob.append( U8( 0 ) ) ) ) {
			return false;
		}

		if( !AX_VERIFY_MEMORY( m_strings.append() ) ) {
			return false;
		}

		SStringData &strdat = m_strings.last();

		strdat.uHash   = uHash;
		strdat.uOffset = U32( uOff );
		strdat.cBytes  = U16( s.len() + 1 );
		strdat.uFlags  = 0;

		return true;
	}
	/*
	struct SStringData
	{
		// Hash of the string: Used for both lookup and localization
		// High 32-bits is a CRC32 hash, low 32-bits is a custom hash
		U64 uHash;
		// Offset in binary blob to actual data
		U32 uOffset;
		// Number of bytes pointed to
		U16 cBytes;
		// Flags on this string (e.g., to localize) -- see `ESubtokenString`
		U16 uFlags;
	};
	*/

}}
