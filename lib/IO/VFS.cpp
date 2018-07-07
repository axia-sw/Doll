#define DOLL_TRACE_FACILITY doll::kLog_CoreVFS
#include "../BuildSettings.hpp"

#include "doll/IO/VFS.hpp"
#include "doll/IO/VFS-SysFS.hpp"

#include "doll/Core/Engine.hpp"
#include "doll/Core/Logger.hpp"
#include "doll/Core/Memory.hpp"
#include "doll/Core/MemoryTags.hpp"

namespace doll
{

	enum EFindMode
	{
		kFindModeSearch,
		kFindModeCreate
	};
	static SFSPrefix *fs_findPrefix( const Str &prefix, EFindMode mode )
	{
		for( SFSPrefix *p = g_core.fs.prefixes.head(); p != nullptr; p = p->siblings.next() ) {
			if( p->prefix.caseCmp( prefix ) ) {
				return p;
			}
		}

		if( mode == kFindModeCreate ) {
			SFSPrefix *const pFSPrefix = new SFSPrefix();
			if( !AX_VERIFY_MEMORY( pFSPrefix ) ) {
				return nullptr;
			}

			if( !AX_VERIFY_MEMORY( pFSPrefix->prefix.tryAssign( prefix ) ) ) {
				delete pFSPrefix;
				return nullptr;
			}

			pFSPrefix->siblings.setNode( pFSPrefix );
			g_core.fs.prefixes.addTail( pFSPrefix->siblings );
			return pFSPrefix;
		}

		AX_ASSERT( mode == kFindModeSearch );
		return nullptr;
	}

	DOLL_FUNC Void DOLL_API fs_init()
	{
		AX_EXPECT( fs_addFileProvider( CFileProvider_Sysfs::get(), "sysfs" ) );
	}

	DOLL_FUNC Bool DOLL_API fs_addFileProvider( IFileProvider &provider, const Str &prefix )
	{
		SFSPrefix *const pFSPrefix = fs_findPrefix( prefix, kFindModeCreate );
		if( !pFSPrefix ) {
			return false;
		}

		if( !AX_VERIFY_MEMORY( pFSPrefix->providers.append( &provider ) ) ) {
			return false;
		}

		if( g_core.fs.cDefPrefixes < arraySize( g_core.fs.pDefPrefixes ) ) {
			g_core.fs.pDefPrefixes[ g_core.fs.cDefPrefixes++ ] = pFSPrefix;
		}

		return true;
	}
	DOLL_FUNC Void DOLL_API fs_removeFileProvider( IFileProvider &provider )
	{
		SFSPrefix *next;
		for( SFSPrefix *p = g_core.fs.prefixes.head(); p != nullptr; p = next ) {
			next = p->siblings.next();

			for( UPtr j = p->providers.num(); j > 0; --j ) {
				const UPtr i = j - 1;
				IFileProvider *const q = p->providers[ i ];

				if( q != &provider ) {
					continue;
				}

				p->providers.remove( i );
			}

			if( p->providers.isEmpty() ) {
				for( UPtr j = g_core.fs.cDefPrefixes; j > 0; --j ) {
					const UPtr i = j - 1;
					if( g_core.fs.pDefPrefixes[ i ] != p ) {
						continue;
					}

					if( i + 1 != g_core.fs.cDefPrefixes ) {
						g_core.fs.pDefPrefixes[ i ] = g_core.fs.pDefPrefixes[ g_core.fs.cDefPrefixes - 1 ];
					} else {
						g_core.fs.pDefPrefixes[ i ] = nullptr;
					}
					--g_core.fs.cDefPrefixes;
				}

				delete p;
			}
		}
	}

	DOLL_FUNC UPtr DOLL_API fs_findFileProviders( IFileProvider **ppDst, UPtr cMaxDstEntries, const Str &prefix )
	{
		AX_ASSERT_NOT_NULL( ppDst );
		AX_ASSERT( cMaxDstEntries > 0 );

		UPtr cDstEntries = 0;

		// If no prefix is passed, then assume the default chain
		if( prefix.isEmpty() ) {
			// Maximum number of prefixes we can loop through
			const UPtr cMaxPrefixes = arraySize( g_core.fs.pDefPrefixes );
			// Number of prefixes to loop through
			const UPtr cPrefixes = g_core.fs.cDefPrefixes < cMaxPrefixes ? g_core.fs.cDefPrefixes : cMaxPrefixes;

			// Add each prefix
			for( UPtr uPrefix = 0; uPrefix < cPrefixes; ++uPrefix ) {
				// Pointer to the prefix
				const SFSPrefix *const pFSPrefix = g_core.fs.pDefPrefixes[ uPrefix ];
				AX_ASSERT_NOT_NULL( pFSPrefix );

				// Number of providers for this prefix
				const UPtr cProviders = pFSPrefix->providers.num();
				// Raw array of providers for this prefix
				IFileProvider *const *const ppProviders = pFSPrefix->providers.pointer();

				// Add each provider for this prefix
				for( UPtr uProvider = 0; uProvider < cProviders; ++uProvider ) {
					// Terminate if we can't write anymore
					if( cDstEntries == cMaxDstEntries ) {
						return cDstEntries;
					}

					// Add the provider to the output
					AX_ASSERT_NOT_NULL( ppProviders[ uProvider ] );
					ppDst[ cDstEntries++ ] = ppProviders[ uProvider ];
				}
			}

			// Done
			return cDstEntries;
		}

		// Prefix string being parsed
		Str s( prefix );

		// Loop through the string until there's nothing left
		while( s.isUsed() ) {
			// Index of the next ','
			const SPtr i = s.find( ',' );
			// Token found (with spaces around it trimmed)
			const Str  t = ( i == -1 ? s : s.left( i ) ).trim();

			// Advance the string forward
			s = s.skip( i + ( i == -1 ? 0 : 1 ) );

			// Prefix found from the token
			const SFSPrefix *const pFSPrefix = fs_findPrefix( t, kFindModeSearch );
			if( !pFSPrefix ) {
				continue;
			}

			// Enumerate each provider within the prefix
			for( IFileProvider *p : pFSPrefix->providers ) {
				// Terminate if we can't write anymore
				if( cDstEntries == cMaxDstEntries ) {
					return cDstEntries;
				}

				// Add the provider to the output
				AX_ASSERT_NOT_NULL( p );
				ppDst[ cDstEntries++ ] = p;
			}
		}

		// Done
		return cDstEntries;
	}

	DOLL_FUNC IFile *DOLL_API fs_filteredOpen( const TArr<IFileProvider *> &providers, const Str &filename, U32 flags, U32 attribs )
	{
		for( IFileProvider *pFSProvider : providers ) {
			AX_ASSERT_NOT_NULL( pFSProvider );

			IFile *pFile = nullptr;
			const EFileOpenResult r = pFSProvider->open( pFile, filename, flags, attribs );

			char szBuf[ 512 ];
			const Str provname = pFSProvider->getName();
			axspf( szBuf, "%.*s%s%.*s%s",
				filename.len(), filename.get(),
				provname.isEmpty() ? "" : "<",
				provname.len(), provname.get(),
				provname.isEmpty() ? "" : ">" );

			if( r == EFileOpenResult::Ok ) {
				g_VerboseLog( szBuf ) += "File opened successfully.";
				return pFile;
			}

			if( r != EFileOpenResult::NoFile ) {
				char szTmp[ 32 ];
				g_ErrorLog( szBuf ) += ( axspf( szTmp, "Failed to open file. (%i)", ( S32 )r ), szTmp );
				return nullptr;
			}
		}

		g_ErrorLog( filename ) += "File not found in the given providers.";
		return nullptr;
	}
	DOLL_FUNC Bool DOLL_API fs_getPrefixAndName( const Str &src, Str &dstPrefix, Str &dstName )
	{
		const SPtr iPrefixSeq = src.find( "::" );
		dstPrefix = iPrefixSeq == -1 ? Str() : src.left( iPrefixSeq );
		dstName = iPrefixSeq == -1 ? src : src.skip( iPrefixSeq + 2 );
		return iPrefixSeq != -1;
	}
	DOLL_FUNC IFile *DOLL_API fs_open( const Str &filename, U32 flags, U32 attribs )
	{
		Str prefix, name;
		fs_getPrefixAndName( filename, prefix, name );

		if( name.isEmpty() ) {
			g_ErrorLog( filename ) += "Cannot open; invalid filename.";
			return nullptr;
		}

		IFileProvider *pFSProviders[ 64 ];
		const UPtr cFSProviders = fs_findFileProviders( pFSProviders, arraySize( pFSProviders ), prefix );
		if( !cFSProviders ) {
			g_ErrorLog( filename ) += "Cannot open; no providers available.";
			return nullptr;
		}

		return fs_filteredOpen( TArr<IFileProvider*>(pFSProviders,cFSProviders), name, flags, attribs );
	}
	DOLL_FUNC NullPtr DOLL_API fs_close( IFile *pFile )
	{
		if( pFile != nullptr ) {
			pFile->drop();
		}

		return nullptr;
	}

	DOLL_FUNC UPtr DOLL_API fs_getAlignReqs( const IFile *pFile )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->getAlignReqs();
	}
	DOLL_FUNC Bool DOLL_API fs_seek( IFile *pFile, S64 pos, ESeekMode mode )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->seek( pos, mode );
	}
	DOLL_FUNC U64 DOLL_API fs_tell( const IFile *pFile )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->tell();
	}
	DOLL_FUNC U64 DOLL_API fs_size( IFile *pFile )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->size();
	}
	DOLL_FUNC Bool DOLL_API fs_eof( const IFile *pFile )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->isEnd();
	}

	DOLL_FUNC UPtr DOLL_API fs_read( IFile *pFile, Void *pDstBuf, UPtr cBytes )
	{
		AX_ASSERT_NOT_NULL( pFile );
		AX_ASSERT_NOT_NULL( pDstBuf );
		AX_ASSERT( cBytes > 0 );

		return pFile->read( pDstBuf, cBytes );
	}
	DOLL_FUNC UPtr DOLL_API fs_write( IFile *pFile, const Void *pSrcBuf, UPtr cBytes )
	{
		AX_ASSERT_NOT_NULL( pFile );
		AX_ASSERT_NOT_NULL( pSrcBuf );
		AX_ASSERT( cBytes > 0 );

		return pFile->write( pSrcBuf, cBytes );
	}

	DOLL_FUNC IDir *DOLL_API fs_filteredOpenDir( const TArr<IFileProvider *> &providers, const Str &dirname )
	{
		for( IFileProvider *pFSProvider : providers ) {
			AX_ASSERT_NOT_NULL( pFSProvider );

			IDir *pDir = nullptr;
			const EFileOpenResult r = pFSProvider->openDir( pDir, dirname );

			char szBuf[ 512 ];
			const Str prvname = pFSProvider->getName();
			axspf( szBuf, "%.*s%s%.*s%s",
				dirname.len(), dirname.get(),
				prvname.isEmpty() ? "" : "<",
				prvname.len(), prvname.get(),
				prvname.isEmpty() ? "" : ">" );

			if( r == EFileOpenResult::Ok ) {
				g_VerboseLog( szBuf ) += "Directory opened successfully.";
				return pDir;
			}

			if( r != EFileOpenResult::NoFile ) {
				char szTmp[ 32 ];
				g_ErrorLog( szBuf ) += ( axspf( szTmp, "Failed to open directory. (%i)", ( S32 )r ), szTmp );
				return nullptr;
			}
		}

		g_ErrorLog( dirname ) += "Directory not found in the given providers.";
		return nullptr;
	}
	DOLL_FUNC IDir *DOLL_API fs_openDir( const Str &dirname )
	{
		Str prefix, name;
		fs_getPrefixAndName( dirname, prefix, name );

		if( name.isEmpty() ) {
			g_ErrorLog( dirname ) += "Cannot open directory; invalid filename.";
			return nullptr;
		}

		IFileProvider *pFSProviders[ 64 ];
		const UPtr cFSProviders = fs_findFileProviders( pFSProviders, arraySize( pFSProviders ), prefix );
		if( !cFSProviders ) {
			g_ErrorLog( dirname ) += "Cannot open directory; no providers available.";
			return nullptr;
		}

		return fs_filteredOpenDir( TArr<IFileProvider*>(pFSProviders,cFSProviders), name );
	}
	DOLL_FUNC NullPtr DOLL_API fs_closeDir( IDir *pDir )
	{
		if( pDir != nullptr ) {
			pDir->getProvider().closeDir( pDir );
		}

		return nullptr;
	}

	DOLL_FUNC Bool DOLL_API fs_readDir( IDir *pDir, SDirEntry &dstEntry )
	{
		AX_ASSERT_NOT_NULL( pDir );
		return pDir->read( dstEntry );
	}

	DOLL_FUNC Bool DOLL_API fs_statByFile( IFile *pFile, SFileStat &dstStat )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->stat( dstStat );
	}
	DOLL_FUNC Bool DOLL_API fs_filteredStat( const TArr<IFileProvider *> &providers, const Str &filename, SFileStat &dstStat )
	{
		for( IFileProvider *pFSProvider : providers ) {
			AX_ASSERT_NOT_NULL( pFSProvider );

			if( pFSProvider->stat( filename, dstStat ) ) {
				return true;
			}
		}

		return false;
	}
	DOLL_FUNC Bool DOLL_API fs_stat( const Str &filename, SFileStat &dstStat )
	{
		Str prefix, name;
		fs_getPrefixAndName( filename, prefix, name );

		if( name.isEmpty() ) {
			g_ErrorLog( filename ) += "Cannot stat file; invalid filename.";
			return false;
		}

		IFileProvider *pFSProviders[ 64 ];
		const UPtr cFSProviders = fs_findFileProviders( pFSProviders, arraySize( pFSProviders ), prefix );
		if( !cFSProviders ) {
			g_ErrorLog( filename ) += "Cannot stat file; no providers available.";
			return false;
		}

		return fs_filteredStat( TArr<IFileProvider*>(pFSProviders,cFSProviders), name, dstStat );
	}

}
