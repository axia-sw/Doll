#include "../BuildSettings.hpp"
#include "doll/IO/VFS-SysFS.hpp"

namespace doll
{

	/*
	===========================================================================

		CFILE_SYSFS

	===========================================================================
	*/

	CFile_Sysfs::CFile_Sysfs( CFileProvider_Sysfs &provider, OSFile f, UPtr alignReq )
	: IFile( provider )
	, m_file( f )
	, m_alignReq( alignReq )
	, m_bDidEnd( false )
	{
		AX_ASSERT_NOT_NULL( f );
	}
	CFile_Sysfs::~CFile_Sysfs()
	{
		sysfs_close( m_file );
	}

	UPtr CFile_Sysfs::getAlignReqs() const
	{
		return m_alignReq;
	}

	Bool CFile_Sysfs::seek( S64 pos, ESeekMode mode )
	{
		if( mode == ESeekMode::End && pos >= 0 ) {
			m_bDidEnd = true;
		} else if( m_bDidEnd && ( mode != ESeekMode::Relative || pos < 0 ) ) {
			m_bDidEnd = false;
		}

		return sysfs_seek( m_file, pos, mode );
	}
	U64 CFile_Sysfs::tell() const
	{
		return sysfs_tell( m_file );
	}

	U64 CFile_Sysfs::size()
	{
		return sysfs_size( m_file );
	}

	Bool CFile_Sysfs::isEnd() const
	{
		return m_bDidEnd;
	}

	UPtr CFile_Sysfs::read( Void *pDstBuf, UPtr cBytes )
	{
		UPtr cGotBytes = 0;
		const EFileIOResult r = sysfs_read( m_file, pDstBuf, cBytes, cGotBytes );
		if( r == EFileIOResult::NoData ) {
			m_bDidEnd = true;
		}
		return cGotBytes;
	}
	UPtr CFile_Sysfs::write( const Void *pSrcBuf, UPtr cBytes )
	{
		UPtr cOutBytes = 0;
		( Void )sysfs_write( m_file, pSrcBuf, cBytes, cOutBytes );
		return cOutBytes;
	}

	Bool CFile_Sysfs::stat( SFileStat &dstStat )
	{
		return sysfs_statHandle( dstStat, m_file );
	}


	/*
	===========================================================================

		CDIR_SYSFS

	===========================================================================
	*/

	CDir_Sysfs::CDir_Sysfs( CFileProvider_Sysfs &provider, OSDir dir )
	: IDir( provider )
	, m_dir( dir )
	{
	}
	CDir_Sysfs::~CDir_Sysfs()
	{
		sysfs_closeDir( m_dir );
	}
	
	Bool CDir_Sysfs::read( SDirEntry &dst )
	{
		return sysfs_readDir( m_dir, dst );
	}


	/*
	===========================================================================

		CFILEPROVIDER_SYSFS

	===========================================================================
	*/

	CFileProvider_Sysfs::CFileProvider_Sysfs()
	: IFileProvider()
	{
	}
	CFileProvider_Sysfs::~CFileProvider_Sysfs()
	{
	}

	Str CFileProvider_Sysfs::getName() const
	{
		static const Str name;
		return name;
	}

	EFileOpenResult CFileProvider_Sysfs::open( IFile *&dst, const Str &filename, U32 flags, U32 attribs )
	{
		OSFile f = nullptr;
		const EFileOpenResult r = sysfs_open( f, filename, flags, attribs );
		if( r != EFileOpenResult::Ok ) {
			dst = nullptr;
			return r;
		}

		UPtr alignReq = 1;
		if( flags & kFileOpenF_Unbuffered ) {
			alignReq = ( U32 )sysfs_getSectorSize( filename );
		}

		CFile_Sysfs *const p = new CFile_Sysfs( *this, f, alignReq );
		if( !AX_VERIFY_MEMORY( p ) ) {
			sysfs_close( f );
			dst = nullptr;
			return EFileOpenResult::NoMemory;
		}

		dst = p;
		return r;
	}
	Void CFileProvider_Sysfs::close( IFile *pFile )
	{
		if( !pFile ) {
			return;
		}

		CFile_Sysfs *const pSysfsFile = virtualCast< CFile_Sysfs >( pFile );
		delete pSysfsFile;
	}

	EFileOpenResult CFileProvider_Sysfs::openDir( IDir *&dst, const Str &dirname )
	{
		OSDir d = nullptr;
		const EFileOpenResult r = sysfs_openDir( d, dirname );
		if( r != EFileOpenResult::Ok ) {
			dst = nullptr;
			return r;
		}

		CDir_Sysfs *const p = new CDir_Sysfs( *this, d );
		if( !AX_VERIFY_MEMORY( p ) ) {
			sysfs_closeDir( d );
			dst = nullptr;
			return EFileOpenResult::NoMemory;
		}

		dst = p;
		return r;
	}
	Void CFileProvider_Sysfs::closeDir( IDir *pDir )
	{
		if( !pDir ) {
			return;
		}

		CDir_Sysfs *const pSysfsDir = virtualCast< CDir_Sysfs >( pDir );
		delete pSysfsDir;
	}

	Bool CFileProvider_Sysfs::stat( const Str &filename, SFileStat &dstStat )
	{
		return sysfs_stat( dstStat, filename );
	}

	CFileProvider_Sysfs &CFileProvider_Sysfs::get()
	{
		static CFileProvider_Sysfs instance;
		return instance;
	}

}
