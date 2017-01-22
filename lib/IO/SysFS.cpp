#ifdef _WIN32
# undef WIN32_LEAN_AND_MEAN // this can't be defined because we need DeviceIoControl
# if !defined( _WIN32_WINNT ) || ( _WIN32_WINNT < 0x0601 )
#  undef _WIN32_WINNT
#  define _WIN32_WINNT 0x0601 // must be at least this
# endif
# include <Windows.h>
# include <ShlObj.h>
# undef min
# undef max
#else
# include <errno.h>
# include <sys/stat.h>
#endif

#include "doll/IO/SysFS.hpp"

#include "doll/Core/Memory.hpp"
#include "doll/Core/MemoryTags.hpp"

// FIXME: Add GNU/Linux and Mac OS X versions

namespace doll
{

#ifdef _WIN32
	enum class EWin32Path
	{
		File,
		FindWildcard
	};

	inline wchar_t *win32path( wchar_t *pwszDstBuf, UPtr cMaxDstChars, const Str &name, EWin32Path mode )
	{
		char szBuf[ kMaxPath*2 ];

		axstr_cpy( szBuf, name );
		for( char *p = axstr_findchr( szBuf, '/' ); p != nullptr; p = axstr_findchr( p + 1, '/' ) ) {
			*p = '\\';
		}

		switch( mode ) {
		case EWin32Path::File:
			break;

		case EWin32Path::FindWildcard:
			axstr_catpath( szBuf, "\\*" );
			break;
		}

		return Str( szBuf ).toWStr( pwszDstBuf, cMaxDstChars );
	}
	template< UPtr tMaxChars >
	inline wchar_t *win32path( wchar_t( &wszDstBuf )[ tMaxChars ], const Str &name, EWin32Path mode )
	{
		return win32path( wszDstBuf, tMaxChars, name, mode );
	}

	inline HANDLE win32h( OSFile f )
	{
		return ( HANDLE )( f );
	}

	inline U32 attribs_fromwin32( DWORD dwFileAttributes )
	{
		U32 r = kFileAttrib_Regular;

		if( dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
			r = kFileAttrib_Directory;
		} else if( dwFileAttributes & FILE_ATTRIBUTE_DEVICE ) {
			r = kFileAttrib_Device;
		}

		if( dwFileAttributes & FILE_ATTRIBUTE_READONLY ) {
			r |= kFileAttribF_ReadOnly;
		}
		if( dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) {
			r |= kFileAttribF_Hidden;
		}
		if( dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY ) {
			r |= kFileAttribF_Temporary;
		}
		if( dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ) {
			r |= kFileAttribF_System;
		}
		if( dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) {
			r |= kFileAttribF_SymLink;
		}
		if( dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE ) {
			r |= kFileAttribF_Archive;
		}

		if( dwFileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ) {
			r |= kFileAttribF_NoSearch;
		}
		if( dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED ) {
			r |= kFileAttribF_Compress;
		}
		if( dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED ) {
			r |= kFileAttribF_Encrypt;
		}

		return r;
	}
	inline DWORD attribs_towin32( U32 uAttribs )
	{
		DWORD dwFileAttributes = 0;

		switch( uAttribs & kFileAttribTypeMask ) {
		case kFileAttrib_Regular:
			dwFileAttributes = 0;
			break;

		case kFileAttrib_Directory:
			dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			break;

		case kFileAttrib_Device:
		case kFileAttrib_Socket:
			dwFileAttributes = FILE_ATTRIBUTE_DEVICE;
			break;
		}

		if( uAttribs & kFileAttribF_ReadOnly ) {
			dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
		}
		if( uAttribs & kFileAttribF_Temporary ) {
			dwFileAttributes |= FILE_ATTRIBUTE_TEMPORARY;
		}
		if( uAttribs & kFileAttribF_Hidden ) {
			dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
		}
		if( uAttribs & kFileAttribF_System ) {
			dwFileAttributes |= FILE_ATTRIBUTE_SYSTEM;
		}
		if( uAttribs & kFileAttribF_SymLink ) {
			dwFileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;
		}
		if( uAttribs & kFileAttribF_Archive ) {
			dwFileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
		}

		if( uAttribs & kFileAttribF_NoSearch ) {
			dwFileAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
		}
		if( uAttribs & kFileAttribF_Compress ) {
			dwFileAttributes |= FILE_ATTRIBUTE_COMPRESSED;
		}
		if( uAttribs & kFileAttribF_Encrypt ) {
			dwFileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
		}

		return dwFileAttributes;
	}

	inline U64 filetime_fromwin32( const FILETIME &t )
	{
		return U64( t.dwHighDateTime )<<32 | U64( t.dwLowDateTime );
	}
	inline FILETIME filetime_towin32( U64 t )
	{
		const FILETIME ft = { U32( t & 0xFFFFFFFF ), U32( (t>>32) & 0xFFFFFFFF ) };
		return ft;
	}

	inline DWORD win32_desiredaccess( U32 flags )
	{
		DWORD r = 0;

		if( flags & kFileOpenF_R ) {
			r |= GENERIC_READ;
		}
		if( flags & kFileOpenF_W ) {
			r |= GENERIC_WRITE;
		}

		return r;
	}
	inline DWORD win32_sharemode( U32 flags )
	{
		DWORD r = 0;

		if( ~flags & kFileOpenF_ExcludeR ) {
			r |= FILE_SHARE_READ;
		}

		if( flags & kFileOpenF_ShareW ) {
			r |= FILE_SHARE_WRITE;
		}

		return r;
	}
	inline DWORD win32_creationdisposition( U32 flags )
	{
		if( ( flags & kFileOpen_CreateMask ) == 0x00000000 ) {
			switch( flags & kFileOpen_AccessMask ) {
			case kFileOpenF_R:
				flags |= kFileOpenF_Existing;
				break;

			case kFileOpenF_W:
				flags |= kFileOpenF_NotExist;
				break;
			}
		}

		switch( flags & kFileOpen_CreateMask ) {
		case kFileOpenF_Existing:
			return OPEN_EXISTING;

		case kFileOpenF_NotExist:
			return CREATE_NEW;

		case kFileOpenF_Recreate:
			return CREATE_ALWAYS;
		}

		return OPEN_ALWAYS;
	}
	inline DWORD win32_flagsandattributes( U32 flags, U32 attribs )
	{
		DWORD r = attribs_towin32( attribs );

		if( flags & kFileOpenF_Temp ) {
			r |= FILE_ATTRIBUTE_TEMPORARY;
		}

		if( flags & kFileOpenF_Unbuffered ) {
			r |= FILE_FLAG_NO_BUFFERING;
		}
		if( flags & kFileOpenF_DeleteOnClose ) {
			r |= FILE_FLAG_DELETE_ON_CLOSE;
		}
		if( flags & kFileOpenF_RandomAccess ) {
			r |= FILE_FLAG_RANDOM_ACCESS;
		}
		if( flags & kFileOpenF_Sequential ) {
			r |= FILE_FLAG_SEQUENTIAL_SCAN;
		}
		if( flags & kFileOpenF_WriteThrough ) {
			r |= FILE_FLAG_WRITE_THROUGH;
		}

		return r;
	}

	class CWin32Dir: public TPoolObject< CWin32Dir, kTag_FileSys >
	{
	public:
		CWin32Dir()
		: m_handle( INVALID_HANDLE_VALUE )
		, m_findData()
		, m_bHaveData( false )
		{
		}
		~CWin32Dir()
		{
			if( m_handle != INVALID_HANDLE_VALUE ) {
				FindClose( m_handle );
				m_handle = INVALID_HANDLE_VALUE;
			}

			m_bHaveData = false;
		}

		EFileOpenResult open( const Str &name )
		{
			if( name.isEmpty() || name.find( '*' ) != -1 ) {
				return EFileOpenResult::InvalidFilename;
			}

			{
				wchar_t wszName[ kMaxPath*2 ];
				if( !win32path( wszName, name, EWin32Path::FindWildcard ) ) {
					return EFileOpenResult::InvalidFilename;
				}

				wszName[ kMaxPath*2 - 1 ] = L'\0';
				m_handle = FindFirstFileExW( wszName, FindExInfoBasic, &m_findData, FindExSearchNameMatch, nullptr, 0 );
			}

			if( m_handle == INVALID_HANDLE_VALUE ) {
				const DWORD dwErr = GetLastError();

				if( dwErr == ERROR_INVALID_NAME ) {
					return EFileOpenResult::InvalidFilename;
				}

				if( dwErr == ERROR_FILE_NOT_FOUND || dwErr == ERROR_PATH_NOT_FOUND ) {
					return EFileOpenResult::NoFile;
				}

				if( dwErr == ERROR_ACCESS_DENIED ) {
					return EFileOpenResult::NoPermission;
				}

				if( dwErr == ERROR_OUTOFMEMORY ) {
					return EFileOpenResult::NoMemory;
				}

				return EFileOpenResult::UnknownError;
			}

			m_bHaveData = true;
			return EFileOpenResult::Ok;
		}
		Bool read( SDirEntry &dstEntry )
		{
			AX_ASSERT( m_handle != INVALID_HANDLE_VALUE );

			if( !m_bHaveData ) {
				return false;
			}

			{
				MutStr x( MutStr::fromWStr( m_findData.cFileName ) );
				dstEntry.cNameBytes = U32( axstr_cpy( dstEntry.szName, x ) );
			}

			dstEntry.uAttributes   = attribs_fromwin32( m_findData.dwFileAttributes );
			dstEntry.uTimeCreated  = filetime_fromwin32( m_findData.ftCreationTime );
			dstEntry.uTimeModified = filetime_fromwin32( m_findData.ftLastWriteTime );
			dstEntry.uTimeAccessed = filetime_fromwin32( m_findData.ftLastAccessTime );
			dstEntry.cBytes        = U64( m_findData.nFileSizeHigh )<<32 | U64( m_findData.nFileSizeLow );

			m_bHaveData = !!FindNextFileW( m_handle, &m_findData );
			return true;
		}

	private:
		HANDLE           m_handle;
		WIN32_FIND_DATAW m_findData;
		Bool             m_bHaveData;
	};
#endif

	DOLL_FUNC U32 DOLL_API sysfs_getSectorSize( const Str &filename )
	{
		//
		//	NOTE: This function is *this* complicated because we require the
		//	`     physical sector size, not the logical sector size.
		//	`     GetDiskFreeSpace() returns the logical sector size, not the
		//	`     physical sector size.
		//

#ifdef _WIN32
		const Str root = ( filename.isEmpty() ? app_getPath() : filename ).getRoot();
		char szDrive[ 8 ] = { '\\', '\\', '.', '\\', 'C', ':', 0, 0 };

		if( root.isUsed() ) {
			szDrive[ 4 ] = ( char )axstr_toupper( root[ 0 ] );
		}

		STORAGE_PROPERTY_QUERY q;
		memset( &q, 0, sizeof( q ) );

		HANDLE hDrive = CreateFileA( szDrive, STANDARD_RIGHTS_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( hDrive == INVALID_HANDLE_VALUE ) {
			return 0;
		}

		q.QueryType = PropertyStandardQuery;
		q.PropertyId = StorageAccessAlignmentProperty;

		STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR desc;

		DWORD cBytes = 0;
		const Bool bResult = !!DeviceIoControl( hDrive, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof( q ), &desc, sizeof( desc ), &cBytes, nullptr );

		CloseHandle( hDrive );
		hDrive = INVALID_HANDLE_VALUE;

		if( !bResult ) {
			return 0;
		}

		return ( UPtr )desc.BytesPerPhysicalSector;
#else
		// Hard-coded assumption fallback
		return 512;
#endif
	}

	DOLL_FUNC Bool DOLL_API sysfs_stat( SFileStat &dst, const Str &filename )
	{
#ifdef _WIN32
		wchar_t wszName[ kMaxPath*2 ];
		if( !win32path( wszName, filename, EWin32Path::File ) ) {
			return false;
		}
		wszName[ kMaxPath*2 - 1 ] = L'\0';

		// FIXME: Use `GetFileAttributesEx()`
		// FIXME: Get `uDeviceId` and `uRecordId`

		WIN32_FIND_DATAW findData;
		HANDLE hFind = FindFirstFileExW( wszName, FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, 0 );
		if( hFind == INVALID_HANDLE_VALUE ) {
			return false;
		}

		dst.uAttributes   = attribs_fromwin32( findData.dwFileAttributes );
		dst.uTimeCreated  = filetime_fromwin32( findData.ftCreationTime );
		dst.uTimeModified = filetime_fromwin32( findData.ftLastWriteTime );
		dst.uTimeAccessed = filetime_fromwin32( findData.ftLastAccessTime );
		dst.cBytes        = U64( findData.nFileSizeHigh )<<32 | U64( findData.nFileSizeLow );
		dst.uDeviceId     = 0;
		dst.uRecordId     = 0;

		FindClose( hFind );
		return true;
#else
		return false;
#endif
	}
	DOLL_FUNC Bool DOLL_API sysfs_statHandle( SFileStat &dst, OSFile f )
	{
#ifdef _WIN32
		BY_HANDLE_FILE_INFORMATION info;

		if( !GetFileInformationByHandle( win32h( f ), &info ) ) {
			return false;
		}

		dst.uAttributes   = attribs_fromwin32( info.dwFileAttributes );
		dst.uDeviceId     = info.dwVolumeSerialNumber;
		dst.uRecordId     = U64(info.nFileIndexHigh)<<32 | U64(info.nFileIndexLow);
		dst.uTimeCreated  = filetime_fromwin32( info.ftCreationTime );
		dst.uTimeModified = filetime_fromwin32( info.ftLastWriteTime );
		dst.uTimeAccessed = filetime_fromwin32( info.ftLastWriteTime );
		dst.cBytes        = U64(info.nFileSizeHigh)<<32 | U64(info.nFileSizeLow);

		return true;
#else
		return false;
#endif
	}

	DOLL_FUNC EFileOpenResult DOLL_API sysfs_open( OSFile &f, const Str &filename, U32 flags, U32 attribs )
	{
#ifdef _WIN32
		wchar_t wszFilename[ kMaxPath*2 ];

		if( !win32path( wszFilename, filename, EWin32Path::File ) ) {
			return EFileOpenResult::InvalidFilename;
		}

		const DWORD dwDesiredAccess = win32_desiredaccess( flags );
		const DWORD dwShareMode     = win32_sharemode( flags );
		const DWORD dwCreatDisposit = win32_creationdisposition( flags );
		const DWORD dwFlagsAttribs  = win32_flagsandattributes( flags, attribs );

		wszFilename[ kMaxPath*2 - 1 ] = L'\0';
		const HANDLE h = CreateFileW( wszFilename, dwDesiredAccess, dwShareMode, nullptr, dwCreatDisposit, dwFlagsAttribs, INVALID_HANDLE_VALUE );

		if( h == INVALID_HANDLE_VALUE ) {
			switch( GetLastError() ) {
			case ERROR_FILE_NOT_FOUND:
				return EFileOpenResult::NoFile;

			case ERROR_ACCESS_DENIED:
				return EFileOpenResult::NoPermission;

			case ERROR_OUTOFMEMORY:
				return EFileOpenResult::NoMemory;
			}

			return EFileOpenResult::UnknownError;
		}

		f = ( OSFile )h;
		return EFileOpenResult::Ok;
#else
		return EFileOpenResult::UnknownError;
#endif
	}
	DOLL_FUNC NullPtr DOLL_API sysfs_close( OSFile f )
	{
#ifdef _WIN32
		if( win32h( f ) != INVALID_HANDLE_VALUE ) {
			CloseHandle( win32h( f ) );
		}
#endif

		return nullptr;
	}

	DOLL_FUNC U64 DOLL_API sysfs_size( OSFile f )
	{
#ifdef _WIN32
		LARGE_INTEGER fileSize;
		if( !GetFileSizeEx( win32h( f ), &fileSize ) ) {
			return 0;
		}

		return (U64)fileSize.QuadPart;
#else
		return 0;
#endif
	}

#ifdef _WIN32
	static EFileIOResult win32ioerr()
	{
		const DWORD dwLastErr = GetLastError();

		switch( dwLastErr ) {
		case ERROR_INVALID_HANDLE:
			return EFileIOResult::InvalidHandle;

		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_OUTOFMEMORY:
			return EFileIOResult::NoMemory;

		case ERROR_HANDLE_EOF:
			return EFileIOResult::NoData;
		}

		return EFileIOResult::UnknownError;
	}
#endif
	DOLL_FUNC EFileIOResult DOLL_API sysfs_write( OSFile f, const Void *pSrc, UPtr cBytes, UPtr &cBytesWritten )
	{
#ifdef _WIN32
		const DWORD dwReq = DWORD( cBytes & 0xFFFFFFFF );
		DWORD dwGot = 0;

		if( !WriteFile( win32h( f ), pSrc, dwReq, &dwGot, nullptr ) ) {
			return win32ioerr();
		}

		cBytesWritten = ( UPtr )dwGot;
		return EFileIOResult::Ok;
#else
		return EFileIOResult::UnknownError;
#endif
	}
	DOLL_FUNC EFileIOResult DOLL_API sysfs_read( OSFile f, Void *pDst, UPtr cBytes, UPtr &cBytesRead )
	{
#ifdef _WIN32
		const DWORD dwReq = DWORD( cBytes & 0xFFFFFFFF );
		DWORD dwGot = 0;

		if( !ReadFile( win32h( f ), pDst, dwReq, &dwGot, nullptr ) ) {
			return win32ioerr();
		}

		cBytesRead = ( UPtr )dwGot;
		return EFileIOResult::Ok;
#else
		return EFileIOResult::UnknownError;
#endif
	}

	DOLL_FUNC Bool DOLL_API sysfs_seek( OSFile f, S64 uOffset, ESeekMode mode )
	{
#ifdef _WIN32
		DWORD dwMoveMethod = 0;
		switch( mode ) {
		case ESeekMode::Absolute:
			dwMoveMethod = FILE_BEGIN;
			break;

		case ESeekMode::Relative:
			dwMoveMethod = FILE_CURRENT;
			break;

		case ESeekMode::End:
			dwMoveMethod = FILE_END;
			break;
		}

		LARGE_INTEGER offset;
		offset.QuadPart = uOffset;

		return SetFilePointerEx( win32h( f ), offset, nullptr, dwMoveMethod ) != FALSE;
#else
		return false;
#endif
	}
	DOLL_FUNC U64 DOLL_API sysfs_tell( const OSFile f )
	{
#ifdef _WIN32
		LARGE_INTEGER offset = { { 0, 0 } };
		if( !SetFilePointerEx( win32h( f ), offset, &offset, FILE_CURRENT ) ) {
			return 0;
		}
		return U64( offset.QuadPart );
#else
		return 0;
#endif
	}

	DOLL_FUNC EFileOpenResult DOLL_API sysfs_openDir( OSDir &dstDir, const Str &dirname )
	{
#ifdef _WIN32
		dstDir = nullptr;

		CWin32Dir *const pDir = new CWin32Dir();
		if( !AX_VERIFY_MEMORY( pDir ) ) {
			return EFileOpenResult::NoMemory;
		}

		const EFileOpenResult r = pDir->open( dirname );
		if( r != EFileOpenResult::Ok ) {
			delete pDir;
			return r;
		}

		dstDir = ( OSDir )pDir;
		return r;
#else
		return EFileOpenResult::UnknownError;
#endif
	}
	DOLL_FUNC NullPtr DOLL_API sysfs_closeDir( OSDir dir )
	{
#ifdef _WIN32
		delete ( CWin32Dir * )dir;
#endif
		return nullptr;
	}

	DOLL_FUNC Bool DOLL_API sysfs_readDir( OSDir dir, SDirEntry &dstEntry )
	{
		AX_ASSERT_NOT_NULL( dir );
#ifdef _WIN32
		return ( ( CWin32Dir * )dir )->read( dstEntry );
#else
		return false;
#endif
	}

	DOLL_FUNC UPtr DOLL_API sysfs_getDir( char *pszDst, UPtr cDstBytes )
	{
#ifdef _WIN32
		static CRITICAL_SECTION cs;
		static wchar_t wszBuf[ 32768 ];
		static bool didInit = false;
		DWORD dwNumChars;

		if( !didInit ) {
			InitializeCriticalSection( &cs );
			didInit = true;
		}

		AX_ASSERT_NOT_NULL( pszDst );
		AX_ASSERT( cDstBytes > 0 );

		EnterCriticalSection( &cs );
		do {
			dwNumChars = GetCurrentDirectoryW( sizeof(wszBuf)/sizeof(wszBuf[0]), wszBuf );
			if( !dwNumChars ) {
				break;
			}

			dwNumChars = DWORD( axstr_counted_utf16_to_utf8_n( ( axstr_utf8_t * )pszDst, axstr_size_t( cDstBytes ), ( const axstr_utf16_t * )wszBuf, ( const axstr_utf16_t * )&wszBuf[ dwNumChars ] ) );
			if( !dwNumChars ) {
				break;
			}
		} while( false );
		LeaveCriticalSection( &cs );

		return UPtr( dwNumChars );
#else
		AX_ASSERT_NOT_NULL( pszDst );
		AX_ASSERT( cDstBytes > 0 );

		*pszDst = '\0';
		return 0;
#endif
	}
	DOLL_FUNC Bool DOLL_API sysfs_setDir( const Str &dir )
	{
#ifdef _WIN32
		static CRITICAL_SECTION cs;
		static wchar_t wszBuf[ 32768 ];
		static bool didInit = false;
		Bool result = false;

		if( !didInit ) {
			InitializeCriticalSection( &cs );
			didInit = true;
		}

		EnterCriticalSection( &cs );
		do {
			if( !dir.toWStr( wszBuf ) ) {
				break;
			}

			if( !SetCurrentDirectoryW( wszBuf ) ) {
				break;
			}

			result = true;
		} while( false );
		LeaveCriticalSection( &cs );

		return result;
#else
		return false;
#endif
	}

	class MDirStack
	{
	public:
		static MDirStack &get();

		inline Bool enter( const Str &path )
		{
			CQuickMutexGuard guard( m_lock );

			auto iter = m_paths.addHead();
			if( !AX_VERIFY( iter != m_paths.end() ) ) {
				return false;
			}

			auto delIter = makeScopeGuard([&](){m_paths.remove(iter);});

			if( !AX_VERIFY_MEMORY( iter->tryAssign( path ) ) ) {
				return false;
			}

			if( !sysfs_setDir( path ) ) {
				return false;
			}

			delIter.commit();
			return true;
		}
		inline Void leave()
		{
			CQuickMutexGuard guard( m_lock );

			if( m_paths.isEmpty() ) {
				return;
			}

			auto iter = m_paths.first();
			sysfs_setDir( *iter );

			m_paths.remove( iter );
		}

	private:
		TList<MutStr> m_paths;
		CQuickMutex   m_lock;

		inline MDirStack()
		: m_paths()
		, m_lock()
		{
		}
		inline ~MDirStack()
		{
			if( m_paths.isUsed() ) {
				auto iter = m_paths.last();
				sysfs_setDir( *iter );
			}
		}
	};
	TManager<MDirStack> g_dirstack;
	MDirStack &MDirStack::get()
	{
		static MDirStack instance;
		return instance;
	}

	DOLL_FUNC Bool DOLL_API sysfs_enter( const Str &dir )
	{
		return g_dirstack->enter( dir );
	}
	DOLL_FUNC Void DOLL_API sysfs_leave()
	{
		g_dirstack->leave();
	}

	DOLL_FUNC Bool DOLL_API sysfs_mkdir( const Str &dir )
	{
#ifdef _WIN32
		wchar_t wszDir[ 4096 ];
#else
		char szDir[ 4096 ];
#endif

		// The root path, which we are skipping (e.g., "C:/" or "\\server\" or just "/")
		const Str rootPart( dir.getRoot() );
		// The directories to work on, after the root part
		SPtr lastDirSep = rootPart.isEmpty() ? -1 : rootPart.len();

		// Create directories while there are directories left
		do {
			// Find the next directory part
			const SPtr dirSep = dir.findDirSep( lastDirSep );
			lastDirSep = dirSep;

			// Get the next directory part
			const Str dirPart = dir.left( dirSep );

			// If the directory part is empty then there's probably an accidental double-/
			if( dirPart.isEmpty() ) {
				continue;
			}

#ifdef _WIN32
			if( !dirPart.toWStr( wszDir ) || ( !CreateDirectoryW( wszDir, nullptr ) && GetLastError() != ERROR_ALREADY_EXISTS ) ) {
				return false;
			}
#else
			axstr_cpy( szDir, dirPart );

			errno = 0;
			mkdir( szDir, 0740 );
			if( errno && errno != EEXIST ) {
				return false;
			}
#endif
		} while( lastDirSep != -1 );

		// Done
		return true;
	}

#ifdef _WIN32
	template< int tCSIDL >
	static Bool DOLL_API sysfs__getShellDir( Str &dst )
	{
		static char szBuf[ MAX_PATH*4 ];
		static UPtr cBuf = 0;

		if( !cBuf ) {
			wchar_t wszPath[ MAX_PATH ];

			if( !SHGetSpecialFolderPathW( NULL, wszPath, tCSIDL, TRUE ) ) {
				dst = Str();
				return false;
			}

			cBuf = axstr_counted_utf16_to_utf8( ( axstr_utf8_t * )szBuf, sizeof( szBuf ), ( const axstr_utf16_t * )wszPath );
		}

		dst = Str( szBuf, cBuf );
		return true;
	}
#endif

	DOLL_FUNC Bool DOLL_API sysfs_getAppDataDir( Str &dst )
	{
#ifdef _WIN32
		return sysfs__getShellDir< CSIDL_LOCAL_APPDATA >( dst );
#else
		// ~/.etc/
		dst = Str();
		return false;
#endif
	}
	DOLL_FUNC Bool DOLL_API sysfs_getMyDocsDir( Str &dst )
	{
#ifdef _WIN32
		return sysfs__getShellDir< CSIDL_MYDOCUMENTS >( dst );
#else
		// ~/Documents
		dst = Str();
		return false;
#endif
	}

}
