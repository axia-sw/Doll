#define DOLL_TRACE_FACILITY doll::kLog_SndFile
#include "../BuildSettings.hpp"

#include "doll/Snd/WaveFile.hpp"

#include "doll/Core/Logger.hpp"
#include "doll/Core/Memory.hpp"
#include "doll/Core/MemoryTags.hpp"
#include "doll/IO/File.hpp"
#include "doll/IO/VFS.hpp"
#include "doll/IO/AsyncIO.hpp"

namespace doll
{

	// TODO: Evaluate whether memory mapped files might be a better way to go
	// `     for wave files

#define WAV_CHUNKID(A_,B_,C_,D_)\
	( \
		( U32((A_)&0xFF)<< 0 ) | \
		( U32((B_)&0xFF)<< 8 ) | \
		( U32((C_)&0xFF)<<16 ) | \
		( U32((D_)&0xFF)<<24 )   \
	)
#define CHR0(X_)          ( U8(((X_)>> 0)&0xFF) )
#define CHR1(X_)          ( U8(((X_)>> 8)&0xFF) )
#define CHR2(X_)          ( U8(((X_)>>16)&0xFF) )
#define CHR3(X_)          ( U8(((X_)>>24)&0xFF) )
#define CHRS(X_)          CHR0(X_), CHR1(X_), CHR2(X_), CHR3(X_)
#define WAV_CHUNKID_S(X_) WAV_CHUNKID((X_)[0],(X_)[1],(X_)[2],(X_)[3])

	CWaveFile::CWaveFile()
	: m_chunks()
	, m_pFile( nullptr )
	, m_wf()
	, m_pDataChunk( nullptr )
	{
	}
	CWaveFile::~CWaveFile()
	{
		fini();
	}

	Bool CWaveFile::init( Str filename )
	{
		AX_ASSERT_IS_NULL( m_pFile );
		AX_ASSERT_IS_NULL( m_pDataChunk );

		m_chunks.clear();
		m_chunks.reserve( 4 );

		m_pDataChunk = nullptr;

#if DOLL__WAV_USE_RSTREAMFILE
		if( !( m_pFile = core_sfopen( filename ) ) ) {
			return false;
		}
#else
		if( !( m_pFile = fs_open( filename, kFileOpenF_R | kFileOpenF_Sequential ) ) ) {
			return false;
		}
#endif

		char szDbgTmp[256];
		for(;;) {
			// Current position within the file
#if DOLL__WAV_USE_RSTREAMFILE
			const U64 uSFPos = core_sftell( m_pFile );
#else
			const U64 uSFPos = fs_tell( m_pFile );
#endif

			DOLL_DEBUG_LOG += (axspf(szDbgTmp, "Current file position: %llu (0x%llX)", uSFPos, uSFPos), szDbgTmp);

			// Number of bytes read from the file
			UPtr cBytes = 0;

			// Read from the file
#if DOLL__WAV_USE_RSTREAMFILE
			const Void *p = core_sfread( m_pFile, cBytes );
#else
			U8 uReadBuf[ 4096 ];
			cBytes = fs_read( m_pFile, &uReadBuf[0], sizeof(uReadBuf) );
			const Void *p = cBytes > 0 ? &uReadBuf[0] : nullptr;
#endif

			DOLL_DEBUG_LOG += (axspf(szDbgTmp, "Read %zu (0x%.4zX) byte%s", cBytes, cBytes, cBytes==1 ? "" : "s"), szDbgTmp);

			// If we were unable to read or have too little data then do an
			// early exit (REMEMBER: core_sfread() will go out of its way to
			// ensure we have 64 bytes of data, so if it can't do that then
			// there was an error or the file ended early)
			if( !p || cBytes < 8 ) {
				break;
			}

			// Offset within the chunk
			UPtr uOffset = 0;

			// Continue searching within our block of memory
			while( uOffset + 8 <= cBytes ) {
				// WAV chunk header
				const U8 *const pHdr = ( const U8 * )p + uOffset;

				// Identifier of the chunk
				const U32 uChunkId = WAV_CHUNKID( pHdr[ 0 ], pHdr[ 1 ], pHdr[ 2 ], pHdr[ 3 ] );
				// Size of the chunk
				const U32 cChunkSz = WAV_CHUNKID( pHdr[ 4 ], pHdr[ 5 ], pHdr[ 6 ], pHdr[ 7 ] );

				DOLL_DEBUG_LOG += (axspf(szDbgTmp, "Found 0x%.8X ('%c%c%c%c') @ offset %zu (0x%.4zX); size=%u (0x%.8X)", uChunkId, CHRS( uChunkId ), uOffset, uOffset, cChunkSz, cChunkSz), szDbgTmp);

				// Allocate the chunk
				if( !AX_VERIFY_MEMORY( m_chunks.append() ) ) {
#if DOLL__WAV_USE_RSTREAMFILE
					core_sfclose( m_pFile );
#else
					fs_close( m_pFile );
#endif
					m_pFile = nullptr;

					g_ErrorLog(filename) += "Failed to allocate chunk";
					return false;
				}

				// Record the chunk
				SChunk &chunk = m_chunks.last();

				chunk.uType = uChunkId;
				chunk.cBytes = cChunkSz;
				chunk.pData = nullptr;
				chunk.uOffset = uSFPos + uOffset;

				// Increment the offset to the next check position
				if( uSFPos == 0 && uOffset == 0 ) {
					// RIFF chunk requires special handling
					uOffset += 12;
				} else {
					// Align to 16-bit boundary
					uOffset += cChunkSz + 8 + 1;
					uOffset -= uOffset%2;
				}
			}

			// Make the offset relative to the next chunk
			uOffset -= cBytes;

			// Skip ahead if necessary
#if DOLL__WAV_USE_RSTREAMFILE
			core_sfskip( m_pFile, uOffset );
#else
			fs_seek( m_pFile, S64(uOffset), ESeekMode::Relative );
#endif
		}

		if( m_chunks.isEmpty() ) {
			g_ErrorLog( filename ) += "Load failed";

			fini();
			return false;
		}

		if( m_chunks.first().uType != WAV_CHUNKID_S( "RIFF" ) || m_chunks.first().cBytes < 12 ) {
			g_ErrorLog( filename ) += "Invalid WAV file; expected 'RIFF' chunk";

			fini();
			return false;
		}

		m_chunks.first().cBytes = 12;

		SChunk *const pFmtChunk = findChunk( WAV_CHUNKID_S( "fmt " ) );
		if( !pFmtChunk ) {
			g_ErrorLog( filename ) += "Invalid WAV file; missing 'fmt ' chunk";

			fini();
			return false;
		}

		if( pFmtChunk->cBytes < sizeof( SWaveBase ) ) {
			g_ErrorLog( filename ) += "Invalid WAV file; 'fmt ' chunk is too small";

			fini();
			return false;
		}
		if( pFmtChunk->cBytes > 4096 ) {
			g_ErrorLog( filename ) += "Invalid WAV file; 'fmt ' chunk is too big";

			fini();
			return false;
		}

		if( !loadChunk( *pFmtChunk ) ) {
			g_ErrorLog( filename ) += "Failed to load 'fmt ' chunk";

			fini();
			return false;
		}

		memset( &m_wf, 0, sizeof( m_wf ) );
		const UPtr cWFBytes = pFmtChunk->cBytes < sizeof( m_wf ) ? pFmtChunk->cBytes : sizeof( m_wf );
		memcpy( &m_wf, pFmtChunk->pData, cWFBytes );

		if( !m_wf.isValid( filename ) ) {
			fini();
			return false;
		}

		if( !( m_pDataChunk = findChunk( WAV_CHUNKID_S( "data" ) ) ) ) {
			g_ErrorLog( filename ) += "Invalid WAV file; missing 'data' chunk";

			fini();
			return false;
		}

		return true;
	}
	Bool CWaveFile::loadData()
	{
		AX_ASSERT_NOT_NULL( m_pFile );
		AX_ASSERT( m_wf.isValid() );
		AX_ASSERT_NOT_NULL( m_pDataChunk );
		AX_ASSERT_IS_NULL( m_pDataChunk->pData );
		
		return loadChunk( *m_pDataChunk );
	}
	CAsyncOp *CWaveFile::loadDataAsync()
	{
#if DOLL__WAV_USE_RSTREAMFILE
		AX_ASSERT_MSG( false, "Not supported" );
		return nullptr;
#else
		AX_ASSERT_NOT_NULL( m_pFile );
		AX_ASSERT( m_wf.isValid() );
		AX_ASSERT_NOT_NULL( m_pDataChunk );
		AX_ASSERT_IS_NULL( m_pDataChunk->pData );
		AX_ASSERT( m_pDataChunk->cBytes > 8 );

		if( !fs_seek( m_pFile, S64( m_pDataChunk->uOffset + 8 ), ESeekMode::Absolute ) ) {
			DOLL_ERROR_LOG += "Failed to seek to data chunk.";
			return nullptr;
		}

		Void *const p = DOLL_ALLOC( *gDefaultAllocator, m_pDataChunk->cBytes, kTag_Sound );
		if( !AX_VERIFY_MEMORY( p ) ) {
			return nullptr;
		}

		m_pDataChunk->pData = p;

		// FIXME: Need name of file and should provide a read callback

		CAsyncOp *const pOp = async_readFile( m_pFile, "BGM", p, m_pDataChunk->cBytes, nullptr );
		if( !pOp ) {
			m_pDataChunk->pData = nullptr;
			DOLL_DEALLOC( *gDefaultAllocator, p );
			return nullptr;
		}

		return pOp;
#endif
	}
	Void CWaveFile::fini()
	{
		m_pDataChunk = nullptr;

		for( SChunk &chunk : m_chunks ) {
			freeChunk( chunk );
		}
		m_chunks.purge();

#if DOLL__WAV_USE_RSTREAMFILE
		m_pFile = core_sfclose( m_pFile );
#else
		m_pFile = fs_close( m_pFile );
#endif
	}

	const SWaveFormat &CWaveFile::getFormat() const
	{
		AX_ASSERT_NOT_NULL( m_pFile );
		return m_wf;
	}

	const Void *CWaveFile::getData() const
	{
		AX_ASSERT_NOT_NULL( m_pDataChunk );
		AX_ASSERT_NOT_NULL( m_pDataChunk->pData );

		return m_pDataChunk->pData;
	}
	UPtr CWaveFile::getDataSize() const
	{
		AX_ASSERT_NOT_NULL( m_pDataChunk );
		AX_ASSERT_NOT_NULL( m_pDataChunk->pData );

		return UPtr( m_pDataChunk->cBytes );
	}

	Bool CWaveFile::loadChunk( SChunk &chunk )
	{
		AX_ASSERT_NOT_NULL( m_pFile );
		AX_ASSERT_IS_NULL( chunk.pData );
		AX_ASSERT( chunk.cBytes > 8 );

#if DOLL__WAV_USE_RSTREAMFILE
		if( !core_sfseek( m_pFile, chunk.uOffset + 8 ) ) {
			return false;
		}
#else
		if( !fs_seek( m_pFile, chunk.uOffset + 8, ESeekMode::Absolute ) ) {
			return false;
		}
#endif

		Void *const p = DOLL_ALLOC( *gDefaultAllocator, chunk.cBytes, kTag_Sound );
		if( !AX_VERIFY_MEMORY( p ) ) {
			return false;
		}

		U32 cReadBytes = 0;
		do {
			UPtr cGotBytes;
#if DOLL__WAV_USE_RSTREAMFILE
			const Void *pReadBuf = core_sfread( m_pFile, cGotBytes );
#else
			U8 uReadBuf[ 4096 ];
			cGotBytes = fs_read( m_pFile, &uReadBuf[0], sizeof(uReadBuf) );
			const Void *pReadBuf = cGotBytes > 0 ? &uReadBuf[0] : nullptr;
#endif
			if( !pReadBuf ) {
				DOLL_DEALLOC( *gDefaultAllocator, p );
				return false;
			}

			if( cReadBytes + cGotBytes > chunk.cBytes ) {
				cGotBytes = chunk.cBytes - cReadBytes;
			}

			memcpy( ( Void * )( (U8*)p + cReadBytes ), pReadBuf, cGotBytes );
			cReadBytes += U32( cGotBytes );
		} while( cReadBytes < chunk.cBytes );

		chunk.pData = p;
		return true;
	}
	Void CWaveFile::freeChunk( SChunk &chunk )
	{
		DOLL_DEALLOC( *gDefaultAllocator, chunk.pData );
		chunk.pData = nullptr;
	}
	CWaveFile::SChunk *CWaveFile::findChunk( U32 uTag ) const
	{
		for( const SChunk &x : m_chunks ) {
			if( x.uType == uTag ) {
				return const_cast< SChunk * >( &x );
			}
		}

		return nullptr;
	}

}
