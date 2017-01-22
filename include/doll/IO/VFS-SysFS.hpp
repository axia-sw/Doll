#pragma once

#include "../Core/Defs.hpp"
#include "VFS.hpp"
#include "SysFS.hpp"

namespace doll
{

	class CFile_Sysfs;
	class CDir_Sysfs;
	class CFileProvider_Sysfs;

	class CFile_Sysfs: public virtual IFile
	{
	friend class CFileProvider_Sysfs;
	public:
		virtual UPtr getAlignReqs() const override;

		virtual Bool seek( S64 pos, ESeekMode mode ) override;
		virtual U64 tell() const override;

		virtual U64 size() override;

		virtual Bool isEnd() const override;

		virtual UPtr read( Void *pDstBuf, UPtr cBytes ) override;
		virtual UPtr write( const Void *pSrcBuf, UPtr cBytes ) override;

		virtual Bool stat( SFileStat &dstStat ) override;

	protected:
		CFile_Sysfs( CFileProvider_Sysfs &provider, OSFile f, UPtr alignReq );
		virtual ~CFile_Sysfs();

	private:
		const OSFile m_file;
		const UPtr   m_alignReq;
		Bool         m_bDidEnd;
	};

	class CDir_Sysfs: public virtual IDir
	{
	friend class CFileProvider_Sysfs;
	public:
		virtual Bool read( SDirEntry &dst ) override;

	protected:
		CDir_Sysfs( CFileProvider_Sysfs &provider, OSDir dir );
		virtual ~CDir_Sysfs();

	private:
		const OSDir m_dir;
	};

	class CFileProvider_Sysfs: public virtual IFileProvider
	{
	public:
#ifdef DOLL__BUILD
		static CFileProvider_Sysfs &get();
#endif

		virtual Str getName() const override;

		virtual EFileOpenResult open( IFile *&dst, const Str &filename, U32 flags, U32 attribs ) override;
		virtual Void close( IFile *pFile ) override;

		virtual EFileOpenResult openDir( IDir *&dst, const Str &dirname ) override;
		virtual Void closeDir( IDir *pDir ) override;

		virtual Bool stat( const Str &filename, SFileStat &dstStat ) override;

	private:
		CFileProvider_Sysfs();
		virtual ~CFileProvider_Sysfs();
	};

}
