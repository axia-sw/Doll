#pragma once

#include "../Core/Defs.hpp"
#include "SysFS.hpp"

#include <stdarg.h>

namespace doll
{

	class IFileProvider;

	class IFile
	{
	friend class IFileProvider;
	public:
		virtual IFile *grab()
		{
			atomicInc( &m_cRefs );
			return this;
		}
		virtual NullPtr drop();

		inline IFileProvider &getProvider() const
		{
			return m_fileProvider;
		}

		// Retrieves the memory buffer alignment requirements in bytes for IO and
		// seek operations on this file.
		//
		// If this returns a nonzero value then the value is guaranteed to be a
		// power of two, and the file is unbuffered.
		//
		// Returns zero if the file is buffered.
		virtual UPtr getAlignReqs() const = 0;

		// Seek to a specific position in the file
		//
		// If this file is unbuffered then `pos` must be a multiple of
		// `getAlignReqs()`
		virtual Bool seek( S64 pos, ESeekMode = ESeekMode::Relative ) = 0;
		// Retrieve the current position in the file
		virtual U64 tell() const = 0;

		// Retrieve the size of this file
		virtual U64 size()
		{
			const U64 curPos = tell();
			if( !seek( 0, ESeekMode::End ) ) {
				return 0;
			}
			const U64 cBytes = tell();
			seek( S64( curPos ), ESeekMode::Absolute );

			return cBytes;
		}

		// Determine whether the end of the file was reached
		virtual Bool isEnd() const = 0;

		// Read from the file
		//
		// If this file is unbuffered then `pDstBuf` must be aligned to--and
		// `cBytes` must be a multiple of,--the value returned by `getAlignReqs()`.
		//
		// Returns number of bytes read (or 0 if there was an error or the end of
		// the file was reached)
		virtual UPtr read( Void *pDstBuf, UPtr cBytes ) = 0;
		// Write to the file
		virtual UPtr write( const Void *pSrcBuf, UPtr cBytes ) = 0;

		// Retrieve file status information (size, attributes, times, unique id)
		virtual Bool stat( SFileStat & )
		{
			return false;
		}

	protected:
		IFileProvider &m_fileProvider;
		U32            m_cRefs;

		IFile( IFileProvider &provider )
		: m_fileProvider( provider )
		, m_cRefs( 1 )
		{
		}
		virtual ~IFile()
		{
		}
	};

	class IDir
	{
	friend class IFileProvider;
	public:
		virtual ~IDir()
		{
		}

		inline IFileProvider &getProvider() const
		{
			return m_fileProvider;
		}

		virtual Bool read( SDirEntry &dst ) = 0;

	protected:
		IFileProvider &m_fileProvider;

		IDir( IFileProvider &provider )
		: m_fileProvider( provider )
		{
		}
	};

	class IFileProvider
	{
	public:
		IFileProvider() {}
		virtual ~IFileProvider() {}

		// Returns a name identifying this file provider (e.g., "media00.zip")
		virtual Str getName() const = 0;

		virtual EFileOpenResult open( IFile *&, const Str &filename, U32 flags, U32 attribs ) = 0;
		virtual Void close( IFile * ) = 0;

		virtual EFileOpenResult openDir( IDir *&, const Str &dirname ) = 0;
		virtual Void closeDir( IDir * ) = 0;

		virtual Bool stat( const Str &filename, SFileStat &dstStat )
		{
			IFile *p;
			if( open( p, filename, kFileOpenF_R | kFileOpenF_ShareW, 0 ) != EFileOpenResult::Ok || !p ) {
				return false;
			}
			const Bool r = p->stat( dstStat );
			close( p );
			return r;
		}
	};

	inline NullPtr IFile::drop()
	{
		if( atomicDec( &m_cRefs ) == 1 ) {
			getProvider().close( this );
		}

		return nullptr;
	}

	DOLL_FUNC Void DOLL_API fs_init();

	DOLL_FUNC Bool DOLL_API fs_addFileProvider( IFileProvider &, const Str &prefix );
	DOLL_FUNC Void DOLL_API fs_removeFileProvider( IFileProvider & );

	DOLL_FUNC UPtr DOLL_API fs_findFileProviders( IFileProvider **ppDst, UPtr cMaxDstEntries, const Str &prefix );

	DOLL_FUNC IFile *DOLL_API fs_filteredOpen( const TArr<IFileProvider *> &providers, const Str &filename, U32 flags = kFileOpenF_R, U32 attribs = 0 );
	DOLL_FUNC IFile *DOLL_API fs_open( const Str &filename, U32 flags = kFileOpenF_R, U32 attribs = 0 );
	DOLL_FUNC NullPtr DOLL_API fs_close( IFile * );

	DOLL_FUNC UPtr DOLL_API fs_getAlignReqs( const IFile * );
	DOLL_FUNC Bool DOLL_API fs_seek( IFile *, S64 pos, ESeekMode = ESeekMode::Relative );
	DOLL_FUNC U64 DOLL_API fs_tell( const IFile * );
	DOLL_FUNC U64 DOLL_API fs_size( IFile * );
	DOLL_FUNC Bool DOLL_API fs_eof( const IFile * );

	DOLL_FUNC UPtr DOLL_API fs_read( IFile *, Void *pDstBuf, UPtr cBytes );
	DOLL_FUNC UPtr DOLL_API fs_write( IFile *, const Void *pSrcBuf, UPtr cBytes );

	inline Bool DOLL_API fs_pfv( IFile *pFile, const char *pszFormat, va_list args )
	{
		char szBuf[ 8192 ];

		const axpf_ptrdiff_t cBytes = axspfv( szBuf, pszFormat, args );
		if( cBytes <= 0 ) {
			return 0;
		}

		return fs_write( pFile, &szBuf[0], UPtr( cBytes ) ) == UPtr( cBytes );
	}
	inline UPtr fs_pf( IFile *pFile, const char *pszFormat, ... )
	{
		va_list args;

		va_start( args, pszFormat );
		const Bool bSucceeded = fs_pfv( pFile, pszFormat, args );
		va_end( args );

		return bSucceeded;
	}

	DOLL_FUNC IDir *DOLL_API fs_filteredOpenDir( const TArr<IFileProvider *> &providers, const Str &dirname );
	DOLL_FUNC IDir *DOLL_API fs_openDir( const Str &dirname );
	DOLL_FUNC NullPtr DOLL_API fs_closeDir( IDir * );

	DOLL_FUNC Bool DOLL_API fs_readDir( IDir *, SDirEntry &dstEntry );

	DOLL_FUNC Bool DOLL_API fs_statByFile( IFile *, SFileStat &dstStat );
	DOLL_FUNC Bool DOLL_API fs_filteredStat( const TArr<IFileProvider *> &providers, const Str &filename, SFileStat &dstStat );
	DOLL_FUNC Bool DOLL_API fs_stat( const Str &filename, SFileStat &dstStat );

	inline Bool DOLL_API fs_pathExists( const Str &filename )
	{
		SFileStat s;
		return fs_stat( filename, s );
	}
	inline Bool DOLL_API fs_pathExistsAs( const Str &filename, EFileAttributes typeAttrib )
	{
		SFileStat s;

		if( !fs_stat( filename, s ) ) {
			return false;
		}

		return ( s.uAttributes & kFileAttribTypeMask ) == U32( typeAttrib );
	}
	inline Bool DOLL_API fs_fileExists( const Str &filename )
	{
		return fs_pathExistsAs( filename, kFileAttrib_Regular );
	}
	inline Bool DOLL_API fs_dirExists( const Str &filename )
	{
		return fs_pathExistsAs( filename, kFileAttrib_Directory );
	}

}
