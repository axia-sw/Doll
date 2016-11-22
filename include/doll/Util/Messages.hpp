#pragma once

#include "../Core/Defs.hpp"

namespace doll
{

	// Localization messages manager [available through: NG::Messages->FunctionName()]
	//
	// Responsible for retrieving the correct text of a given message
	class MMessages
	{
	friend class CMessageLoader;
	public:
		static MMessages &get();

		// Load a localized message
		const char *load( uint32 uMessageId ) const;

		// Export a localization template file
		Bool exportTemplate( Str filename );

	private:
		// Describes a range of messages and the default strings for them
		struct SMessageSection
		{
			U32                uCodeStart;
			U32                uCodeEnd;

			U32                uArrayOffset;

			TArr<const char *> defaultStrings;
		};

		// Array of **translated** message strings
		//
		// These strings will likely be sourced from a file
		TMutArr<MutStr>          m_strings;
		// Collection of sections
		//
		// Used to convert a message id from, e.g., 1000 to the array index the
		// message is stored in, and to hold the built-in default messages
		TMutArr<SMessageSection> m_sections;

		MMessages();
		~MMessages();

		// Create a string section
		Void makeSection( uint32 uCodeStart, TArr<const char *> strings );

		AX_DELETE_COPYFUNCS( MMessages );
	};
#ifdef DOLL__BUILD
	static TManager<MMessages> g_messages;
#endif

#ifdef DOLL__BUILD
# define DOLL__UTIL_MESSAGES MMessages::get()
#else
# define DOLL__UTIL_MESSAGES doll__util_getMessages()
#endif

	//! \internal
	DOLL_FUNC MMessages *DOLL_API doll__util_getMessages_ptr();
	inline MMessages &DOLL_API doll__util_getMessages() {
		return *doll__util_getMessages_ptr();
	}

	// Automatically registers a set of messages
	//
	// Allows a separate source file to define a section of messages without
	// requiring this file (or other files including it) to be rebuilt
	class CMessageLoader
	{
	public:
		// Register a set of messages, starting with the given code
		inline CMessageLoader( uint32 uBaseCode, const char *const *ppszMessages, uintptr cMessages )
		{
			DOLL__UTIL_MESSAGES.makeSection( uBaseCode, TArr< const char * >( ppszMessages, cMessages ) );
		}

		template< uintptr tMessages >
		inline CMessageLoader( uint32 uBaseCode, const char *const( &pszMessages )[ tMessages ] )
		: CMessageLoader( uBaseCode, pszMessages, tMessages )
		{
		}

	private:
		AX_DELETE_COPYFUNCS( CMessageLoader );
	};

#define DOLL__MSGNAME(Name_,Text_) Name_
#define DOLL__MSGTEXT(Name_,Text_) Text_

	// Load a localized message
	DOLL_FUNC const char *DOLL_API msg_load( U32 uMessageId );

	// Format a message
	DOLL_FUNC Bool DOLL_API msg_fmtArr( char *pszDst, UPtr cDstBytes, const char *pszMessage, TArr<Str> args );
	template< UPtr tMaxDstBytes >
	inline Bool DOLL_API msg_fmtArr( char( &szDst )[ tMaxDstBytes ], const char *pszMessage, TArr<Str> args )
	{
		return msg_fmtArr( szDst, tMaxDstBytes, pszMessage, args );
	}

	inline Bool DOLL_API msg_fmt( char *pszDst, UPtr cDstBytes, const char *pszMessage, Str arg1=Str(), Str arg2=Str(), Str arg3=Str(), Str arg4=Str() )
	{
		const Str argRefs[ 4 ] = { arg1, arg2, arg3, arg4 };
		return msg_fmtArr( pszDst, cDstBytes, pszMessage, argRefs );
	}
	template< UPtr tMaxDstBytes >
	inline Bool DOLL_API msg_fmt( char( &szDst )[ tMaxDstBytes ], const char *pszMessage, Str arg1=Str(), Str arg2=Str(), Str arg3=Str(), Str arg4=Str() )
	{
		return msg_fmt( szDst, tMaxDstBytes, pszMessage, arg1, arg2, arg3, arg4 );
	}

}
