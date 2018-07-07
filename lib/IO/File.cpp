#ifdef _WIN32
# undef WIN32_LEAN_AND_MEAN // this can't be defined because we need DeviceIoControl
# if !defined( _WIN32_WINNT ) || ( _WIN32_WINNT < 0x0601 )
#  undef _WIN32_WINNT
#  define _WIN32_WINNT 0x0601 // must be at least this
# endif
# include <Windows.h>
# undef min
# undef max
#endif

#include "../BuildSettings.hpp"

#include "doll/IO/File.hpp"
#include "doll/IO/VFS.hpp"

#include "doll/Core/Memory.hpp"
#include "doll/Core/MemoryTags.hpp"
#include "doll/Core/Logger.hpp"
#include "doll/Core/Engine.hpp"

namespace doll
{

	// ------------------------------------------------------------------ //

	DOLL_FUNC Bool DOLL_API core_loadFile( Str filename, U8 *&out_pDst, UPtr &out_cDstBytes, S32 tag )
	{
		IFile *const f = fs_open( filename, kFileOpenF_R | kFileOpenF_Sequential );
		if( !f ) {
			return false;
		}

		const U64 fileSize64 = fs_size( f );

		if( fileSize64 >= U64(1)<<( sizeof(UPtr)*8 - 1 ) ) {
			fs_close( f );

			g_ErrorLog( filename ) += "File is too big";
			return false;
		}

		const UPtr fileSize = UPtr( fileSize64 );
		if( !fileSize ) {
			fs_close( f );

			out_pDst = nullptr;
			out_cDstBytes = 0;

			return true;
		}

		Void *const p = gDefaultAllocator->alloc( fileSize, tag, __FILE__, __LINE__, AX_FUNCTION );
		if( !p ) {
			fs_close( f );

			g_ErrorLog( filename ) += "Out of memory";
			return false;
		}

		UPtr n = fileSize;
		U8 *q = ( U8 * )p;
		do {
			const UPtr cGotBytes = fs_read( f, ( Void * )q, n );
			if( !cGotBytes ) {
				fs_close( f );
				gDefaultAllocator->dealloc( p, __FILE__, __LINE__, AX_FUNCTION );

				g_ErrorLog( filename ) +=
					axf
					(
						"Read failed at %f%% (%zu/%zu bytes)",
						F64( fileSize - n )/F64( fileSize )*100.0,
						fileSize - n,
						fileSize
					);
				return false;
			}

			n -= cGotBytes;
			q += cGotBytes;
		} while( n > 0 );

		fs_close( f );

		out_pDst = ( U8 * )p;
		out_cDstBytes = fileSize;

		return true;
	}
	DOLL_FUNC Void DOLL_API core_freeFile( U8 *pSrc )
	{
		AX_ASSERT_NOT_NULL( gDefaultAllocator );
		gDefaultAllocator->dealloc( ( Void * )pSrc, __FILE__, __LINE__, AX_FUNCTION );
	}

	DOLL_FUNC Bool DOLL_API core_readText( MutStr &outText, const Str &inFilename )
	{
		IFile *const fp = fs_open( inFilename, kFileOpenF_R );
		if( !fp ) {
			return false;
		}

		const U64 uFileSize = fs_size( fp );
		if( uFileSize > 0x3FFFFFFF ) {
			g_ErrorLog( inFilename ) += "File is too big.";

			fs_close( fp );
			return false;
		}

		// Encoding of the file (UTF-8 by default)
		axstr_encoding_t enc = axstr_enc_utf8;

		// BOM buffer (used to determine the encoding)
		char BOM[ 4 ] = { '\0', '\0', '\0', '\0' };
		// Size of the detected BOM in bytes (so we can skip it)
		axstr_size_t cBOMBytes = 0;

		// Read the BOM
		const UPtr cBOMBytesRead = fs_read( fp, &BOM[ 0 ], sizeof( BOM ) );

		// If the BOM was read then process it
		if( cBOMBytesRead > 0 ) {
			// Detect the encoding of the file from the BOM we read
			enc = axstr_detect_encoding( &BOM[ 0 ], &cBOMBytes );

			// If undetectable then just go with UTF-8
			if( enc == axstr_enc_unknown ) {
				enc = axstr_enc_utf8;
			}

			// Jump to just after the BOM
			fs_seek( fp, cBOMBytes, ESeekMode::Absolute );
		}

		// Size of the file in bytes
		const UPtr cBytes = UPtr( uFileSize ) - cBOMBytes;

		// If the encoding is not UTF-8 then need to convert
		if( enc != axstr_enc_utf8 ) {
			// Data to hold the entire file in memory (at least temporarily)
			void *const pData = malloc( cBytes );
			if( !AX_VERIFY_MEMORY( pData ) ) {
				outText.purge();
				fs_close( fp );
				return false;
			}

			// Read the data into memory
			if( !fs_read( fp, pData, cBytes ) ) {
				free( pData );
				outText.purge();
				fs_close( fp );

				g_ErrorLog( inFilename ) += "Could not read from file.";
				return false;
			}

			// Done with the file pointer
			fs_close( fp );

			// Prepare a sufficiently large buffer for UTF-8 processing
			//
			// ### TODO ### Figure out what the maximum size would need to be
			if( !AX_VERIFY_MEMORY( outText.reserve( cBytes*2 - cBytes/2 ) ) ) {
				return false;
			}

			// Decode the file into UTF-8
			const axstr_size_t cUTF8Bytes = axstr_from_encoding( ( void * )outText.get(), outText.max(), pData, cBytes, enc );

			// Free the temporary data
			free( pData );

			// Record the length of the decoded UTF-8 data
			outText.reserveAndSetLen( cUTF8Bytes );
		} else {
			// Reserve enough space to read the file in
			if( !AX_VERIFY_MEMORY( outText.reserveAndSetLen( cBytes ) ) ) {
				fs_close( fp );
				return false;
			}

			// The encoding matches the string encoding, so no need to do any processing
			const axstr_size_t cUTF8Bytes = fs_read( fp, ( Void * )outText.get(), outText.len() );

			if( cUTF8Bytes != cBytes ) {
				outText.purge();
				fs_close( fp );

				g_ErrorLog( inFilename ) += "Could not read whole file.";
				return false;
			}

			// Done with the file pointer
			fs_close( fp );
		}

		// Done
		return true;
	}

	class RStreamFile: public TPoolObject< RStreamFile, kTag_FileSys >
	{
	public:
		RStreamFile();
		~RStreamFile();

		Bool open( Str filename, UPtr cBufferBytes, UPtr cMinSectors );
		Void close();

		U64 size();

		UPtr getSectorCount() const;
		UPtr getSectorSize() const;
		const Void *read( UPtr &cOutBytes, UPtr uDstSector );

		Bool eof() const;

		Bool seek( U64 uOffset );
		Bool skip( S64 iOffset );
		U64 tell() const;

	private:
		// Handle to the file
		IFile *m_pFile;
		// Capacity of the buffer
		UPtr   m_cMaxBytes;
		// Current position within the file
		U64    m_uPos;
		// Pointer to the allocated memory for the buffer
		Void * m_pBuffer;
		// Last position used to write into the buffer
		U32    m_uBufPosW;
		// Sector alignment for the disk this file is stored on
		U32    m_uSectorAlignment;
		// Whether the end of the file has been reached (or an error occurred)
		Bool   m_bDidEnd;

		const Void *readOnce( UPtr &cOutBytes, UPtr uDstSector );
		UPtr getSector( UPtr uDstSector ) const;
	};

	RStreamFile::RStreamFile()
	: m_pFile( nullptr )
	, m_cMaxBytes( 0 )
	, m_uPos( 0 )
	, m_pBuffer( nullptr )
	, m_uBufPosW( 0 )
	, m_uSectorAlignment( 0 )
	, m_bDidEnd( false )
	{
	}
	RStreamFile::~RStreamFile()
	{
		if( m_pFile != nullptr ) {
			close();
		}
	}

#ifndef _WIN32
	union AlignPtr {
		Void  *p;
		Void **a;
		UPtr   n;
	};
#endif
	static Void *alignedAlloc( UPtr cBytes, UPtr uAlignment )
	{
#ifdef _WIN32
		return _aligned_malloc( cBytes, uAlignment );
#else
		AlignPtr x;

		Void *const p = malloc( cBytes + uAlignment + sizeof(Void*) );
		if( !p ) {
			return nullptr;
		}

		x.p  = p;
		x.a += 1;
		x.n += uAlignment - x.n%uAlignment;

		*( x.a - 1 ) = p;

		return x.p;
#endif
	}
	static Void alignedFree( Void *p )
	{
#ifdef _WIN32
		_aligned_free( p );
#else
		AlignPtr x;

		if( !p ) {
			return;
		}

		x.p  = p;
		p = *( x.a - 1 );

		free( p );
#endif
	}

	Bool RStreamFile::open( Str filename, UPtr cBufferBytes, UPtr cMinSectors )
	{
		//
		//	### TODO ### Optimize so that if the whole file would fit within the
		//	`            requested buffer size then the buffer size would be
		//	`            reduced to the size of the file and the file loaded
		//	`            entirely into memory (as reads occur).
		//

		AX_ASSERT_IS_NULL( m_pFile );

		m_pFile = fs_open( filename, kFileOpenF_R | kFileOpenF_Unbuffered );
		if( !m_pFile ) {
			g_ErrorLog( filename ) += "Could not open file to read";
			return false;
		}

		if( !( m_uSectorAlignment = ( U32 )fs_getAlignReqs( m_pFile ) ) ) {
			m_pFile = fs_close( m_pFile );

			g_ErrorLog( filename ) += "Could not determine sector size for the given file";
			return false;
		}

		m_cMaxBytes = cBufferBytes > 0 && cBufferBytes%m_uSectorAlignment != 0 ? cBufferBytes + ( m_uSectorAlignment - cBufferBytes%m_uSectorAlignment ) : cBufferBytes;
		if( m_cMaxBytes/m_uSectorAlignment < cMinSectors ) {
			m_cMaxBytes = m_uSectorAlignment*cMinSectors;
		}

		AX_ASSERT( m_cMaxBytes > 0 );

		if( !AX_VERIFY_MEMORY( m_pBuffer = alignedAlloc( m_cMaxBytes, m_uSectorAlignment ) ) ) {
			m_cMaxBytes = 0;

			m_pFile = fs_close( m_pFile );
			return false;
		}

		g_DebugLog( filename ) += axf( "Opened streaming file with %ux%zu (%zu) sized buffer", m_uSectorAlignment, cMinSectors, m_cMaxBytes );

		return true;
	}
	Void RStreamFile::close()
	{
		if( m_pBuffer != nullptr ) {
			alignedFree( m_pBuffer );
			m_pBuffer = nullptr;
		}

		AX_ASSERT_NOT_NULL( m_pFile );
		m_pFile = fs_close( m_pFile );
	}

	U64 RStreamFile::size()
	{
		return fs_size( m_pFile );
	}

	inline UPtr RStreamFile::getSectorCount() const
	{
		return m_cMaxBytes/m_uSectorAlignment;
	}
	inline UPtr RStreamFile::getSectorSize() const
	{
		return m_uSectorAlignment;
	}
	const Void *RStreamFile::read( UPtr &cOutBytes, UPtr uDstSector )
	{
		//
		//	### TODO ### Optimize so that reads will not overwrite a section of
		//	`            the buffer that already has fresh data
		//

		AX_ASSERT_NOT_NULL( m_pFile );
		AX_ASSERT( m_uSectorAlignment != 0 );

		const UPtr cSectors = getSectorCount();
		UPtr uSectorId = getSector( uDstSector );

		if( uSectorId + 1 == cSectors && uDstSector == ~UPtr(0) && cSectors >= 2 && m_uPos%m_uSectorAlignment > m_uSectorAlignment - 64 ) {
			uSectorId = 0;
		}

		UPtr cBytes;
		const Void *const p = readOnce( cBytes, uSectorId );
		if( !p ) {
			return nullptr;
		}

		if( cBytes >= 64 || uSectorId + 1 == cSectors || m_bDidEnd ) {
			cOutBytes = cBytes;
			return p;
		}

		UPtr cExtraBytes;
		if( readOnce( cExtraBytes, uSectorId + 1 ) != nullptr ) {
			cBytes += cExtraBytes;
		}

		cOutBytes = cBytes;
		return p;
	}
	const Void *RStreamFile::readOnce( UPtr &cOutBytes, UPtr uDstSector )
	{
		AX_ASSERT_NOT_NULL( m_pFile );
		AX_ASSERT( m_uSectorAlignment != 0 );

		if( m_bDidEnd ) {
			return nullptr;
		}

		const UPtr cReqBytes = m_uSectorAlignment;
		UPtr cGotBytes;

		const UPtr uBufPosW = getSector( uDstSector );
		U8 *const pDstBuf = ( U8 * )m_pBuffer + uBufPosW*m_uSectorAlignment;

		cGotBytes = fs_read( m_pFile, ( Void * )pDstBuf, cReqBytes );
		if( !cGotBytes || fs_eof( m_pFile ) ) {
			m_bDidEnd = true;
			return nullptr;
		}

		m_uBufPosW = ( U32 )uBufPosW;

		// SectorAlignment = 512 (0x200)
		// Pos = 128 (0x080)
		// Req = 512 (0x200)
		//
		// Read of sector at 0x000 produces 0x200 bytes
		// Only 0x200 - 0x080 == 0x180 (512 - 128 == 384) are of interest
		//
		// The position will then be realigned:
		//
		// Pos = 128 (0x080)
		// Got = 384 (0x180)
		//
		// Pos = Pos(128;0x080) + Got(384;0x180) == 512

		const UPtr uOffset = U32( m_uPos%m_uSectorAlignment );
		const UPtr cNewBytes = cGotBytes - uOffset;

		m_uPos += cNewBytes; //align( dwGotBytes, m_uSectorAlignment );

		AX_ASSERT( cGotBytes != cReqBytes || m_uPos%m_uSectorAlignment == 0 );

		cOutBytes = cNewBytes;
		return ( const Void * )( pDstBuf + uOffset );
	}
	UPtr RStreamFile::getSector( UPtr uDstSector ) const
	{
		return ( uDstSector == ~UPtr( 0 ) ? m_uBufPosW : uDstSector )%getSectorCount();
	}

	Bool RStreamFile::eof() const
	{
		AX_ASSERT_NOT_NULL( m_pFile );
		return m_bDidEnd;
	}

	Bool RStreamFile::seek( U64 uOffset )
	{
		//
		//	### TODO ### Optimize so that no operations occur if this would
		//	`            address valid cached data.
		//

		AX_ASSERT_NOT_NULL( m_pFile );

		if( !fs_seek( m_pFile, uOffset - uOffset%m_uSectorAlignment, ESeekMode::Absolute ) ) {
			return false;
		}

		// Unaligned because caller explicitly wants to ignore a section of the
		// data
		m_uPos = uOffset;
		m_bDidEnd = false;
		return true;
	}
	Bool RStreamFile::skip( S64 iOffset )
	{
		if( !iOffset ) {
			return true;
		}

		// 240 - 241 (0xF0 - 0xF1) == 0xFF, 0xFF > 0xF0 (overflowed)
		// 2 - 1 (0x02 - 0x01) == 0x01, 0x01 < 0x02 (no overflow)
		//
		// The following checks for an overflow when iOffset is indicating a
		// backward direction, and clamps to zero accordingly
		if( iOffset < 0 && U64( m_uPos + iOffset ) > m_uPos ) {
			return seek( 0 );
		}

		// 240 + 241 (0xF0 + 0xF1) == 0x[1]E1, 0xE1 < 0xF0 (overflowed)
		// 2 + 1 (0x02 + 0x01) == 0x03, 0x03 > 0x02 (no overflow)
		//
		// The following checks for an overflow when iOffset is indicating a
		// forward direction and clamps to the maximum value accordingly
		if( iOffset > 0 && U64( m_uPos + iOffset ) < m_uPos ) {
			return seek( ~U64( 0 ) );
		}

		// A normal seek operation! (But by the given offset)
		return seek( m_uPos + iOffset );
	}
	U64 RStreamFile::tell() const
	{
		AX_ASSERT_NOT_NULL( m_pFile );
		return m_uPos;
	}

	//--------------------------------------------------------------------//

	DOLL_FUNC RStreamFile *DOLL_API core_sfopen( Str filename, UPtr cBufferBytes, UPtr cMinSectors )
	{
		RStreamFile *const pSF = new RStreamFile();
		if( !AX_VERIFY_MEMORY( pSF ) ) {
			return nullptr;
		}

		if( !pSF->open( filename, cBufferBytes, cMinSectors ) ) {
			delete pSF;
			return nullptr;
		}

		return pSF;
	}
	DOLL_FUNC RStreamFile *DOLL_API core_sfclose( RStreamFile *pFile )
	{
		delete pFile;
		return nullptr;
	}

	DOLL_FUNC U64 DOLL_API core_sfsize( RStreamFile *pFile )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->size();
	}

	DOLL_FUNC UPtr DOLL_API core_sfsectorcount( const RStreamFile *pFile )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->getSectorCount();
	}
	DOLL_FUNC UPtr DOLL_API core_sfsectorsize( const RStreamFile *pFile )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->getSectorSize();
	}
	DOLL_FUNC const Void *DOLL_API core_sfread( RStreamFile *pFile, UPtr &cOutBytes, UPtr uDstSector )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->read( cOutBytes, uDstSector );
	}

	DOLL_FUNC Bool DOLL_API core_sfeof( const RStreamFile *pFile )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->eof();
	}

	DOLL_FUNC Bool DOLL_API core_sfseek( RStreamFile *pFile, U64 uOffset )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->seek( uOffset );
	}
	DOLL_FUNC Bool DOLL_API core_sfskip( RStreamFile *pFile, S64 iOffset )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->skip( iOffset );
	}
	DOLL_FUNC U64 DOLL_API core_sftell( const RStreamFile *pFile )
	{
		AX_ASSERT_NOT_NULL( pFile );
		return pFile->tell();
	}

}
