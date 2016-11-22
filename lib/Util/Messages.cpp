#include "doll/Util/Messages.hpp"

#include "doll/Core/Logger.hpp"
#include "doll/IO/VFS.hpp"

namespace doll
{

	/*
	===========================================================================

		MESSAGES MANAGER

	===========================================================================
	*/

	MMessages &MMessages::get()
	{
		static MMessages instance;
		return instance;
	}
	DOLL_FUNC MMessages *DOLL_API doll__util_getMessages_ptr()
	{
		return &MMessages::get();
	}

	MMessages::MMessages()
	: m_strings()
	, m_sections()
	{
	}
	MMessages::~MMessages()
	{
	}

	Void MMessages::makeSection( U32 uCodeStart, TArr<const char *> strings )
	{
		AX_EXPECT_MEMORY( m_sections.append() );

		SMessageSection &sect = m_sections.last();

		sect.uCodeStart     = uCodeStart;
		sect.uCodeEnd       = uCodeStart + U32( strings.num() );
		sect.uArrayOffset   = U32( m_strings.num() );
		sect.defaultStrings = strings;

		AX_EXPECT_MEMORY( m_strings.resize( m_strings.num() + strings.num() ) );
	}

	const char *MMessages::load( U32 uMessageId ) const
	{
		static char szInvalidMsg[ 128 ];

		// Search for the appropriate section
		for( const SMessageSection &sect : m_sections ) {
			// Skip this section if the requested message isn't in it
			if( uMessageId < sect.uCodeStart || uMessageId >= sect.uCodeEnd ) {
				continue;
			}

			// Find the array index for this message
			const uint32 uMessageIndex = sect.uArrayOffset + ( uMessageId - sect.uCodeStart );

			// All messages must fit within the strings array
			AX_ASSERT( uMessageIndex <= m_strings.num() );

			// If the message in the strings array isn't blank then it can be returned
			if( m_strings[ uMessageIndex ].isUsed() ) {
				return m_strings[ uMessageIndex ].get();
			}

			// Index of the message within the section's DefaultStrings array
			const uint32 uDefMsgIndex = uMessageId - sect.uCodeStart;

			// A default message must not be null
			AX_ASSERT_NOT_NULL( sect.defaultStrings[ uDefMsgIndex ] );

			// Return that message
			return sect.defaultStrings[ uDefMsgIndex ];
		}

		// The message identifier given is invalid
		axspf( szInvalidMsg, "Invalid message id (%u); please file a report.", uMessageId );
		return szInvalidMsg;
	}

	Bool MMessages::exportTemplate( Str filename )
	{
		IFile *const pFile = fs_open( filename, kFileOpenF_W | kFileOpenF_Sequential );
		if( !pFile ) {
			g_ErrorLog( filename ) += "Failed to open file for writing.";
			return false;
		}

		makeScopeGuard([pFile](){fs_close(pFile);});

		static const U8 bom[ 3 ] = { 0xEF, 0xBB, 0xBF };
		if( fs_write( pFile, &bom[ 0 ], sizeof( bom ) ) != sizeof( bom ) ) {
			return false;
		}

		// Export each section
		for( const SMessageSection &sect : m_sections ) {
			Bool r = true;

			// Number of strings in the section
			const U32 n = U32( sect.defaultStrings.num() );
			AX_ASSERT( sect.uCodeEnd - sect.uCodeStart == n );

			// Mark the current section
			r = r && fs_pf( pFile, "// =========================== //\n" );
			r = r && fs_pf( pFile, "// ==== Section %.4u-%.4u ==== //\n", sect.uCodeStart, sect.uCodeEnd );
			r = r && fs_pf( pFile, "// =========================== //\n\n" );

			// Write each string from this section
			for( U32 i = 0; i < n; ++i ) {
				AX_ASSERT_NOT_NULL( sect.defaultStrings[ i ] );
				r = r && fs_pf( pFile, "// %s\n%.4u = \n\n", sect.defaultStrings[ i ], i );
			}
			r = r && fs_pf( pFile, "\n\n" );

			// Fail?
			if( !r ) {
				return false;
			}
		}

		// Done
		return true;
	}

	DOLL_FUNC const char *DOLL_API msg_load( U32 uMessageId )
	{
		return g_messages->load( uMessageId );
	}

	DOLL_FUNC Bool DOLL_API msg_fmtArr( char *pszDst, uintptr cDstBytes, const char *pszMessage, TArr<Str> args )
	{
		static const UPtr kMaxArgs = 16;

		AX_ASSERT( args.num() < kMaxArgs );

		axconf_stringref_t stringRefs[ kMaxArgs ];

		for( UPtr i = 0; i < args.num(); ++i ) {
			stringRefs[ i ].s = args[ i ].get();
			stringRefs[ i ].n = args[ i ].lenInt();
		}

		// ax_config has a function that does exactly what we want, so use it
		const char *const r = axconf_format_msg( pszDst, cDstBytes, pszMessage, U32( args.len() ), stringRefs );
		if( !r ) {
			return false;
		}

		return true;
	}


}
