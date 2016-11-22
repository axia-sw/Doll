#pragma once

#include "../Core/Defs.hpp"
#include "../IO/File.hpp"
#include "WaveFmt.hpp"

// Enabling this disables asynchronous loads
#ifndef DOLL__WAV_USE_RSTREAMFILE
# define DOLL__WAV_USE_RSTREAMFILE 0
#endif

namespace doll
{

	class IFile;
	class CAsyncOp;

	class CWaveFile
	{
	public:
		CWaveFile();
		~CWaveFile();

		Bool init( Str filename );
		Bool loadData();
		CAsyncOp *loadDataAsync();
		Void fini();

		const SWaveFormat &getFormat() const;

		const Void *getData() const;
		UPtr getDataSize() const;

	private:
		struct SChunk
		{
			U32   uType;
			U32   cBytes;
			Void *pData;
			U64   uOffset;
		};

		TMutArr<SChunk> m_chunks;
#if DOLL__WAV_USE_RSTREAMFILE
		RStreamFile *   m_pFile;
#else
		IFile *         m_pFile;
#endif
		SWaveFormat     m_wf;
		SChunk *        m_pDataChunk;

		Bool loadChunk( SChunk &chunk );
		Void freeChunk( SChunk &chunk );
		SChunk *findChunk( U32 uTag ) const;
	};

}
