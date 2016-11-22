#pragma once

#include "../Core/Defs.hpp"

namespace doll
{

	// Maximum path length (in bytes)
	static const UPtr kMaxPath
		=
#if defined(_MAX_PATH)
			_MAX_PATH
#elif defined(MAX_PATH)
			MAX_PATH
#elif defined(PATH_MAX)
			PATH_MAX
#else
			512
#endif
		;

	namespace detail
	{
		struct SOSFile;
		struct SOSDir;
	}

	typedef detail::SOSFile *OSFile;
	typedef detail::SOSDir  *OSDir;

	enum class ESeekMode: U32
	{
		Absolute,
		Relative,
		End
	};

	enum class EFileOpenResult: S32
	{
		Ok = 1,

		NoFile = 0,
		NoPermission = -1,
		NoMemory = -2,

		InvalidFilename = -10,

		UnknownError = -100
	};
	enum class EFileIOResult: S32
	{
		Ok = 1,

		// EOF
		NoData = 0,
		NoLargeIO = -1,
		NoMemory = -2,

		InvalidHandle = -10,

		UnknownError = -100
	};

	enum EFileOpenFlags: U32
	{
		// Open file for reading only
		kFileOpenF_R  = 0x00000001,
		// Open file for writing only
		kFileOpenF_W  = 0x00000002,
		// Open file for reading and writing
		kFileOpenF_RW = 0x00000003,
		// Access mask
		kFileOpen_AccessMask = 0x00000003,

		// Disallow other programs from reading the file
		kFileOpenF_ExcludeR = 0x00000004,
		// Allow other programs to write to the file
		kFileOpenF_ShareW   = 0x00000008,

		// Open the file only if it exists (implied by kFileOpenF_R)
		kFileOpenF_Existing = 0x00000010,
		// Open the file only if it doesn't exist
		kFileOpenF_NotExist = 0x00000020,
		// Create the file if it doesn't exist; overwrite if it does
		kFileOpenF_Recreate = 0x00000030,
		// Creation mask
		kFileOpen_CreateMask = 0x00000030,

		// Hint that the file should not be flushed to disk
		kFileOpenF_Temp       = 0x00000040,
		// Open the file for unbuffered (no caching) access
		kFileOpenF_Unbuffered = 0x00000080,
		// Delete the file upon closing it
		kFileOpenF_DeleteOnClose = 0x00000100,
		// Hint that the file will be accessed in random orders
		kFileOpenF_RandomAccess  = 0x00000200,
		// Hint that the file will be accessed sequentially
		kFileOpenF_Sequential    = 0x00000400,
		// Hint that write operations should go straight to disk, not cached
		kFileOpenF_WriteThrough  = 0x00000800
	};

	enum EFileAttributes: U32
	{
		// File is a regular file
		kFileAttrib_Regular    = 0x00000000,
		// File is a directory
		kFileAttrib_Directory  = 0x00000001,
		// File is a device
		kFileAttrib_Device     = 0x00000002,
		// File is a socket (or pipe)
		kFileAttrib_Socket     = 0x00000003,
		// Mask used to get the file type
		kFileAttribTypeMask    = 0x00000003,

		// File is intended for read-only access (app interpreted)
		kFileAttribF_ReadOnly  = 0x00000004,
		// File should be hidden by default
		kFileAttribF_Hidden    = 0x00000008,
		// File should not be flushed to disk unless necessary
		kFileAttribF_Temporary = 0x00000010,
		// File is used by the OS
		kFileAttribF_System    = 0x00000020,
		// File is actually just a link to another file
		kFileAttribF_SymLink   = 0x00000040,
		// File marked for archiving (app interpreted)
		kFileAttribF_Archive   = 0x00000080,

		// File content should not be indexed for searching
		kFileAttribF_NoSearch  = 0x00000100,
		// File system compression should be used if available
		kFileAttribF_Compress  = 0x00000200,
		// File system encryption should be used if available
		kFileAttribF_Encrypt   = 0x00000400
	};

	struct SFileStat
	{
		// Attributes of the file
		U32 uAttributes;
		// Device identifier for the file
		U32 uDeviceId;
		// Unique identifier (relative to the device identifier) for the file
		U64 uRecordId;

		// Time the file was created (platform-specific measurement)
		U64 uTimeCreated;
		// Time the file was modified (platform-specific measurement)
		U64 uTimeModified;
		// Time the file was accessed (platform-specific measurement)
		U64 uTimeAccessed;

		// Size of the file in bytes
		U64 cBytes;
	};
	struct SDirEntry
	{
		// Name of the file
		char szName[ kMaxPath ];
		// Length, in bytes, of `szName` (prior to the NUL terminator)
		U32  cNameBytes;

		// Attributes of the file
		U32  uAttributes;

		// Time the file was created (platform-specific measurement)
		U64  uTimeCreated;
		// Time the file was modified (platform-specific measurement)
		U64  uTimeModified;
		// Time the file was accessed (platform-specific measurement)
		U64  uTimeAccessed;

		// Size of the file in bytes
		U64  cBytes;

		// Get the name of this entry as a string reference
		inline Str getName() const
		{
			AX_ASSERT( cNameBytes < kMaxPath );
			return Str( &szName[0], &szName[cNameBytes] );
		}
	};

	DOLL_FUNC U32 DOLL_API sysfs_getSectorSize( const Str &filename );

	DOLL_FUNC Bool DOLL_API sysfs_stat( SFileStat &dst, const Str &filename );
	DOLL_FUNC Bool DOLL_API sysfs_statHandle( SFileStat &dst, OSFile  );

	DOLL_FUNC EFileOpenResult DOLL_API sysfs_open( OSFile &, const Str &filename, U32 flags, U32 attribs );
	DOLL_FUNC NullPtr DOLL_API sysfs_close( OSFile );

	DOLL_FUNC U64 DOLL_API sysfs_size( OSFile );

	DOLL_FUNC EFileIOResult DOLL_API sysfs_write( OSFile, const Void *pSrc, UPtr cBytes, UPtr &cBytesWritten );
	DOLL_FUNC EFileIOResult DOLL_API sysfs_read( OSFile, Void *pDst, UPtr cBytes, UPtr &cBytesRead );

	DOLL_FUNC Bool DOLL_API sysfs_seek( OSFile, S64 uOffset, ESeekMode );
	DOLL_FUNC U64 DOLL_API sysfs_tell( const OSFile );

	DOLL_FUNC EFileOpenResult DOLL_API sysfs_openDir( OSDir &, const Str &dirname );
	DOLL_FUNC NullPtr DOLL_API sysfs_closeDir( OSDir );

	DOLL_FUNC Bool DOLL_API sysfs_readDir( OSDir, SDirEntry &dstEntry );

	DOLL_FUNC UPtr DOLL_API sysfs_getDir( char *pszDst, UPtr cDstBytes );
	template< UPtr tDstBytes >
	inline UPtr DOLL_API sysfs_getDir( char( &szDst )[ tDstBytes ] )
	{
		return sysfs_getDir( szDst, tDstBytes );
	}
	DOLL_FUNC Bool DOLL_API sysfs_setDir( const Str &dir );

	DOLL_FUNC Bool DOLL_API sysfs_enter( const Str &dir );
	DOLL_FUNC Void DOLL_API sysfs_leave();

	DOLL_FUNC Bool DOLL_API sysfs_mkdir( const Str &dir );

	DOLL_FUNC Bool DOLL_API sysfs_getAppDataDir( Str &dst );
	DOLL_FUNC Bool DOLL_API sysfs_getMyDocsDir( Str &dst );

	inline Str DOLL_API sysfs_getAppDataDir()
	{
		Str r;
		return sysfs_getAppDataDir( r ), r;
	}
	inline Str DOLL_API sysfs_getMyDocsDir()
	{
		Str r;
		return sysfs_getMyDocsDir( r ), r;
	}

}
