#define DOLL_TRACE_FACILITY doll::kLog_GfxTexture

#include "doll/Gfx/Texture.hpp"
#include "doll/Gfx/API-GL.hpp"
#include "doll/IO/File.hpp"
#include "doll/Core/Logger.hpp"
#include "doll/Core/Memory.hpp"
#include "doll/Core/MemoryTags.hpp"
#include "doll/Math/Math.hpp"

#include <png.h>

static void *doll__stbi_malloc( size_t n, const char *f, unsigned l, const char *fn )
{
	AX_ASSERT_NOT_NULL( doll::gDefaultAllocator );
	doll::IAllocator &a = *doll::gDefaultAllocator;

	void *const p = a.alloc( n + sizeof( size_t ), doll::kTag_Texture, f, l, fn );
	if( !p ) {
		return ( void * )0;
	}

	size_t *const pn = ( size_t * )p;
	*pn = n;

	return ( void * )( pn + 1 );
}
static void *doll__stbi_realloc( void *p, size_t n, const char *f, unsigned l, const char *fn )
{
	AX_ASSERT_NOT_NULL( doll::gDefaultAllocator );
	doll::IAllocator &a = *doll::gDefaultAllocator;

	void *const q = a.alloc( n + sizeof( size_t ), doll::kTag_Texture, f, l, fn );
	if( !q ) {
		return ( void * )0;
	}

	if( p != ( void * )0 ) {
		const size_t *pn = ( ( const size_t * )p ) - 1;
		const size_t oldn = *pn;
		const size_t cpyn = oldn < n ? oldn : n;

		doll::Mem::copy( q, p, cpyn );
		a.dealloc( ( void * )pn, f, l, fn );
	}

	size_t *const qn = ( size_t * )q;
	*qn = n;

	return ( void * )( qn + 1 );
}
static void doll__stbi_free( void *p, const char *f, unsigned l, const char *fn )
{
	AX_ASSERT_NOT_NULL( doll::gDefaultAllocator );
	doll::IAllocator &a = *doll::gDefaultAllocator;

	if( !p ) {
		return;
	}

	size_t *const pn = ( ( size_t * )p ) - 1;
	a.dealloc( ( void * )pn, f, l, fn );
}

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_ASSERT\
	AX_ASSERT
#define STBI_MALLOC(N_)\
	doll__stbi_malloc((size_t)(N_),__FILE__,__LINE__,AX_FUNCTION)
#define STBI_REALLOC(P_,N_)\
	doll__stbi_realloc((void*)(P_),(size_t)(N_),__FILE__,__LINE__,AX_FUNCTION)
#define STBI_FREE(P_)\
	doll__stbi_free((void*)(P_),__FILE__,__LINE__,AX_FUNCTION)

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4312)
# pragma warning(disable:4456)
# pragma warning(disable:4457)
# pragma warning(disable:6001)
# pragma warning(disable:6011)
# pragma warning(disable:6246)
# pragma warning(disable:6262)
# pragma warning(disable:6385)
#endif
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# pragma GCC diagnostic ignored "-Wunused-function"
# pragma GCC diagnostic ignored "-Wunused-variable"
#endif
#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wint-to-pointer-cast"
#endif

#include <stb_image.h>

#ifdef __clang__
# pragma clang diagnostic pop
#endif
#ifdef __GNUC__
//# pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
# pragma warning(pop)
#endif

namespace doll
{

	MTextures MTextures::instance;
	MTextures &g_textureMgr = MTextures::instance;

	DOLL_FUNC Bool DOLL_API gfx_isTextureResolutionValid( U16 resX, U16 resY )
	{
		// must be square
		if( resX!=resY ) {
			return false;
		}

		// must be in range
		if( !resX || resX>16384 ) {
			return false;
		}

		// must be a power of two
		if( !bitIsPowerOfTwo( resX ) ) {
			return false;
		}

		// meets requirements; this is valid
		return true;
	}

	class LoadingTexture {
		enum class ELoadResult {
			success,
			failure,
			keepSearching
		};
		enum class ELoadType {
			none,

			png,
			stb
		};
		ELoadResult loadPNG( const Str &filename, const TArr<U8> &data );
		ELoadResult loadViaSTB( const Str &filename, const TArr<U8> &data );

		U32 m_res_x, m_res_y;
		U8 *m_data;
		ELoadType m_loadedType;
		TMutArr<U8> m_ownedBuffer;
		ETextureFormat m_format;
		U32 m_channels;

		struct LibpngReadData {
			TArr<U8> data;
			UPtr offset;
		};
		static void libpngRead_f( png_structp pngptr, png_bytep data, png_size_t length ) {
			LibpngReadData *const io = reinterpret_cast< LibpngReadData * >( png_get_io_ptr( pngptr ) );
			AX_ASSERT_NOT_NULL( io );

			AX_ASSERT( io->data.isUsed() );

			if( io->offset + length > io->data.num() || io->offset + length < io->offset ) {
				png_error( pngptr, "Read error" );
			}

			memcpy( data, io->data.get() + io->offset, length );
			io->offset += length;
		}

		struct LibpngMessageData {
			const LoadingTexture *loading;
			Str filename;
		};
		static void libpngError_f( png_structp pngptr, png_const_charp message ) {
			const LibpngMessageData *const data = reinterpret_cast< LibpngMessageData * >( png_get_error_ptr( pngptr ) );
			AX_ASSERT_NOT_NULL( data );

			g_ErrorLog( data->filename ) += Str( message );
			longjmp( png_jmpbuf( pngptr ), 1 );
		}
		static void libpngWarning_f( png_structp pngptr, png_const_charp message ) {
			const LibpngMessageData *const data = reinterpret_cast< LibpngMessageData * >( png_get_error_ptr( pngptr ) );
			AX_ASSERT_NOT_NULL( data );

			g_WarningLog( data->filename ) += Str( message );
		}

	public:
		LoadingTexture()
		: m_res_x( 0 )
		, m_res_y( 0 )
		, m_data( nullptr )
		, m_loadedType( ELoadType::none )
		, m_ownedBuffer()
		, m_format( kTexFmtRGB8 )
		, m_channels( 0 )
		{
		}
		~LoadingTexture() {
			fini();
		}

		Bool load( Str filename, TArr<U8> data ) {
			ELoadResult r;

			AX_ASSERT_MSG( m_loadedType == ELoadType::none, "Already initialized" );

			if( ( r = loadPNG( filename, data ) ) != ELoadResult::keepSearching ) {
				AX_ASSERT( r == ELoadResult::failure || m_loadedType == ELoadType::png );
				return r == ELoadResult::success;
			}

			if( ( r = loadViaSTB( filename, data ) ) != ELoadResult::keepSearching ) {
				AX_ASSERT( r == ELoadResult::failure || m_loadedType == ELoadType::stb );
				return r == ELoadResult::success;
			}

			return false;
		}
		Void fini() {
			switch( m_loadedType ) {
			case ELoadType::none:
				break;

			case ELoadType::png:
				m_ownedBuffer.clear();
				m_data = nullptr;
				break;

			case ELoadType::stb:
				AX_ASSERT_NOT_NULL( m_data );
				m_data = ( stbi_image_free( m_data ), nullptr );
				break;
			}

			m_loadedType = ELoadType::none;
			m_res_x = 0;
			m_res_y = 0;
		}

		U32 res_x() const { return m_res_x; }
		U32 res_y() const { return m_res_y; }
		ETextureFormat format() const { return kTexFmtRGBA8; }
		U32 channels() const { return 4; }

		U8 *data() const { AX_ASSERT_NOT_NULL( m_data ); return m_data; }
	};

	LoadingTexture::ELoadResult LoadingTexture::loadPNG( const Str &filename, const TArr<U8> &data ) {
		static const U32 kMaxPNGRes = 16384;
		static const UPtr kPNGSigSize = 8;
		if( data.num() < kPNGSigSize ) {
			if( filename.getExtension().caseCmp( ".png" ) ) {
				g_WarningLog( filename ) += "PNG file is too small";
			}

			return ELoadResult::keepSearching;
		}

		if( !png_check_sig( ( const png_byte * )data.get(), kPNGSigSize ) ) {
			if( filename.getExtension().caseCmp( ".png" ) ) {
				g_WarningLog( filename ) += "Has '.png' extension, but not a valid PNG file";
			}

			return ELoadResult::keepSearching;
		}

		LibpngMessageData msgdata;
		msgdata.loading = this;
		msgdata.filename = filename;

		png_structp pngptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, (png_voidp)&msgdata, &libpngError_f, &libpngWarning_f );
		if( !pngptr ) {
			g_ErrorLog( filename ) += "Failed to create PNG read struct";
			return ELoadResult::failure;
		}

		png_infop infoptr = png_create_info_struct( pngptr );
		if( !infoptr ) {
			g_ErrorLog( filename ) += "Failed to create PNG info struct";
			png_destroy_read_struct( &pngptr, nullptr, nullptr );
			return ELoadResult::failure;
		}

		png_set_user_limits( pngptr, kMaxPNGRes, kMaxPNGRes );

		LibpngReadData iodata;
		iodata.data = data;
		iodata.offset = 0;
		png_set_read_fn( pngptr, &iodata, &libpngRead_f );

		TMutArr<png_bytep> rows;

		if( setjmp( png_jmpbuf( pngptr ) ) ) {
			png_destroy_read_struct( &pngptr, &infoptr, nullptr );
			m_ownedBuffer.purge();
			return ELoadResult::failure;
		}

		png_set_sig_bytes( pngptr, 0 );
		png_read_info( pngptr, infoptr );

		const int pngbitdepth = png_get_bit_depth( pngptr, infoptr );
		const int pngcolortype = png_get_color_type( pngptr, infoptr );

		if( pngcolortype == PNG_COLOR_TYPE_PALETTE ) {
			png_set_palette_to_rgb( pngptr );
		} else if( pngcolortype == PNG_COLOR_TYPE_GRAY && pngbitdepth < 8 ) {
				png_set_expand_gray_1_2_4_to_8( pngptr );
		}
		png_set_gray_to_rgb( pngptr );

		if( png_get_valid( pngptr, infoptr, PNG_INFO_tRNS ) ) {
			png_set_tRNS_to_alpha( pngptr );
		}

		if( pngbitdepth == 16 ) {
			png_set_strip_16( pngptr );
		} else if( pngbitdepth < 8 ) {
			png_set_packing( pngptr );
		}

		png_read_update_info( pngptr, infoptr );

		char buf[ 128 ];

		const U32 pngresx = png_get_image_width( pngptr, infoptr );
		const U32 pngresy = png_get_image_height( pngptr, infoptr );
		switch( png_get_color_type( pngptr, infoptr ) ) {
		case PNG_COLOR_TYPE_GRAY:
			m_format = kTexFmtRGB8;
			m_channels = 3;
			break;
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			m_format = kTexFmtRGBA8;
			m_channels = 4;
			break;
		case PNG_COLOR_TYPE_RGB:
			m_format = kTexFmtRGB8;
			m_channels = 3;
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			m_format = kTexFmtRGBA8;
			m_channels = 4;
			break;

		default:
			g_ErrorLog( filename ) += axspf( buf, "Unknown color type: %i", int( png_get_color_type( pngptr, infoptr ) ) );
			longjmp( png_jmpbuf( pngptr ), 1 );
		}

		g_VerboseLog( filename ) += axspf( buf, "PNG resolution: %ux%u", pngresx, pngresy );
		g_DebugLog += buf;

		if( pngresx > kMaxPNGRes || pngresy > kMaxPNGRes ) {
			g_ErrorLog( filename ) += axspf( buf, "PNG is too large (%ux%u exceeds maximum of %u on any axis)", pngresx, pngresy, kMaxPNGRes );
			longjmp( png_jmpbuf( pngptr ), 1 );
		}

		if( !rows.reserve( axarr_size_t( pngresy ) ) ) {
			g_ErrorLog( filename ) += "Failed to allocate PNG row pointers";
			longjmp( png_jmpbuf( pngptr ), 1 );
		}

		if( !m_ownedBuffer.resize( pngresx*pngresy*m_channels ) ) {
			g_ErrorLog( filename ) += "Failed to allocate PNG image data";
			longjmp( png_jmpbuf( pngptr ), 1 );
		}

		do {
			const U32 stride = pngresx*m_channels;
			rows.resize( pngresy );
			for( U32 y = 0; y < pngresy; ++y ) {
				rows[ pngresy - ( y + 1 ) ] = m_ownedBuffer.pointer( y*stride );
			}
		} while( false );

		png_set_rows( pngptr, infoptr, rows.pointer() );

		png_read_image( pngptr, rows.pointer() );
		png_read_end( pngptr, nullptr );

		png_destroy_read_struct( &pngptr, &infoptr, nullptr );

		m_res_x = pngresx;
		m_res_y = pngresy;

		m_data = m_ownedBuffer.pointer();

		// Done
		m_loadedType = ELoadType::png;
		return ELoadResult::success;
	}
	LoadingTexture::ELoadResult LoadingTexture::loadViaSTB( const Str &filename, const TArr<U8> &data ) {
		// Check if it's a known extension
		do {
			static const Str knownexts[] = {
				".png", ".jpg", ".bmp", ".tga", ".psd", ".dds"
			};

			const Str fileext( filename.getExtension() );

			Bool found = false;
			for( const Str &testext : knownexts ) {
				if( fileext.caseCmp( testext ) ) {
					found = true;
					break;
				}
			}

			if( !found ) {
				return ELoadResult::keepSearching;
			}
		} while( false );

		stbi_set_flip_vertically_on_load( 1 );

		// Try loading the data into memory
		do {
			int w = 0, h = 0, c = 0;
			auto *image = stbi_load_from_memory( ( const stbi_uc * )data.get(), int(ptrdiff_t(data.num())), &w, &h, &c, STBI_rgb_alpha );
			if( !image ) {
				return ELoadResult::failure;
			}
			m_data = reinterpret_cast< U8 * >( image );
			m_res_x = static_cast< U32 >( w );
			m_res_y = static_cast< U32 >( h );
		} while( false );

		// Done
		m_loadedType = ELoadType::stb;
		return ELoadResult::success;
	}

	/*
	===============================================================================

		RECTANGLE ALLOCATOR
		For a given rectangular region, this manages the possible allocations
		within it.

	===============================================================================
	*/

	// constructor
	CRectangleAllocator::CRectangleAllocator()
	: head( nullptr )
	, tail( nullptr )
	{
		resolution.x = 0;
		resolution.y = 0;
	}
	// destructor
	CRectangleAllocator::~CRectangleAllocator()
	{
		fini();
	}

	// initialize
	Bool CRectangleAllocator::init( const SPixelVec2 &res )
	{
		// already initialized?
		if( head!=nullptr ) {
			return true;
		}

		// register the total resolution (must be done before allocating free space
		// node; otherwise the asserts will fail)
		resolution.x = res.x;
		resolution.y = res.y;

		// create the initial free space node
		SNode *const p = allocateNode( 0, 0, res.x, res.y );
		if( !p ) {
			return false;
		}

		// the system is in the valid initial state
		return true;
	}
	// finish
	Void CRectangleAllocator::fini()
	{
		// don't waste time doing proper unlinks
#if _MSC_VER
# pragma warning(push)
# pragma warning(disable:6001) //... false positive? "uninitialized memory"
#endif
		while( head!=nullptr ) {
			SNode *next = head->next;

			free( ( Void * )head );
			head = next;
		}
#if _MSC_VER
# pragma warning(pop)
#endif

		// nullify this setting so it's not accidentally used
		tail = nullptr;

		// there's no resolution anymore
		resolution.x = 0;
		resolution.y = 0;
	}

	// allocate an identifier, returning a token
	Void *CRectangleAllocator::allocateId( SPixelRect &rect, U16 textureId, const SPixelVec2 &res )
	{
		AX_ASSERT_MSG( textureId>0, "textureId is required" );
		AX_ASSERT_MSG( res.x>0, "resolution must be specified" );
		AX_ASSERT_MSG( res.y>0, "resolution must be specified" );

		SNode *const best = findBestFitNode( res.x, res.y );
		if( !best ) {
			return nullptr;
		}

		if( best->rect.res.x > res.x + 2 || best->rect.res.y > res.y + 2 ) {
			if( !splitNode( best, textureId, res.x, res.y ) ) {
				return nullptr;
			}
		}

		rect = best->rect;

		return ( Void * )best;
	}
	// deallocate a previously allocated identifier
	Void CRectangleAllocator::freeId( Void *p )
	{
		if( !p ) {
			return;
		}

#if AX_DEBUG_ENABLED
		// ensure the passed node was allocated in here
		SNode *test;
		for( test=head; test!=nullptr; test=test->next ) {
			if( test==( SNode * )p ) {
				break;
			}
		}

		// if this is nullptr it means we exhausted the whole list without finding the
		// node; therefore the pointer passed is invalid either because it was
		// already removed from the list -or- it was never in this list to begin
		// with
		AX_ASSERT_MSG( test!=nullptr, "node must exist in this allocator" );
#endif

		SNode *node = ( SNode * )p;

		node->textureId = 0;
		mergeAdjacentFreeNodes( node );
	}

	// allocate a free-space node
	CRectangleAllocator::SNode *CRectangleAllocator::allocateNode( U16 offX, U16 offY, U16 resX, U16 resY )
	{
		SNode *node;

		// verify parameters
		AX_ASSERT_MSG( resX>0, "resolution must be specified" );
		AX_ASSERT_MSG( resY>0, "resolution must be specified" );
		AX_ASSERT_MSG( ( offX + resX )<=resolution.x, "node must fit" );
		AX_ASSERT_MSG( ( offY + resY )<=resolution.y, "node must fit" );

		// NOTE: we don't check for intersections because when a split occurs it
		//       has to operate without altering the existing node

		// allocate
		node = ( SNode * )malloc( sizeof( *node ) );
		if( !node ) {
			return nullptr;
		}

		// default settings (free space)
		node->textureId = 0;
		node->rect.off.x = offX;
		node->rect.off.y = offY;
		node->rect.res.x = resX;
		node->rect.res.y = resY;

		// link
		node->next = nullptr;
		node->prev = tail;
		if( node->prev!=nullptr ) {
			node->prev->next = node;
		} else {
			head = node;
		}
		tail = node;

		g_DebugLog += axf( "Allocated %ux%u node at (%u,%u -> %u,%u)", +resX, +resY, +offX, +offY, offX+resX, offY+resY );

		// node is ready now
		return node;
	}
	// deallocate a node (it's okay to pass nullptr in)
	Void CRectangleAllocator::freeNode( SNode *node )
	{
		// accept nullptr input; we need to manually unlink so we can't just pass to
		// free
		if( !node ) {
			return;
		}

		// unlink
		if( node->prev!=nullptr ) {
			node->prev->next = node->next;
		} else {
			head = node->next;
		}
		if( node->next!=nullptr ) {
			node->next->prev = node->prev;
		} else {
			tail = node->prev;
		}

		// now deallocate the memory
		free( ( Void * )node );
	}

	// find the node which has the best fit for this
	CRectangleAllocator::SNode *CRectangleAllocator::findBestFitNode( U16 resX, U16 resY ) const
	{
		// ensure parameters match
		AX_ASSERT_MSG( resX>0, "resolution must be specified" );
		AX_ASSERT_MSG( resY>0, "resolution must be specified" );

		// early exit for nodes that would be too big
		if( resX>resolution.x || resY>resolution.y ) {
			return nullptr;
		}

		// scoring system
		struct SScoredNode
		{
			SNode *node;
			U32    score;
		};

		SScoredNode best = { nullptr, ~0U };

		// find the node with the least wasted space
		for( SNode *node=head; node!=nullptr; node=node->next ) {
			// skip if minimum requirements aren't met
			if( node->textureId!=0 ) {
				continue;
			}
			if( node->rect.res.x<resX || node->rect.res.y<resY ) {
				continue;
			}

			// construct the test object
			SScoredNode test = {
				node,
				( ( U32 )node->rect.res.x )*( ( U32 )node->rect.res.y )
			};

			// test against the current high score and keep the better of the two
			if( test.score<best.score ) {
				best = test;
			}
		}

		// return the currently selected node (nullptr if none met minimum reqs)
		return best.node;
	}
	// split a node into one-to-three nodes, marking one as used
	Bool CRectangleAllocator::splitNode( SNode *node, U16 textureId, U16 resX, U16 resY )
	{
		// ensure all the parameters match
		AX_ASSERT_MSG( node!=nullptr, "node is required" );
		AX_ASSERT_MSG( node->textureId==0, "node must be free" ); //must be a free node
		AX_ASSERT_MSG( textureId>0, "must specify texture" );
		AX_ASSERT_MSG( resX>0, "resolution is required" );
		AX_ASSERT_MSG( resY>0, "resolution is required" );
		AX_ASSERT_MSG( resX<=node->rect.res.x, "resolution must fit" );
		AX_ASSERT_MSG( resY<=node->rect.res.y, "resolution must fit" );

		// figure out the remaining space
		U16 splitRes[ 2 ] = {
			( U16 )( node->rect.res.x - resX ),
			( U16 )( node->rect.res.y - resY )
		};
		U32 bestSplit = splitRes[ 0 ]>splitRes[ 1 ] ? 0 : 1;

		// select the offsets of each remaining point
		SPixelVec2 splitPoints[ 2 ] = {
			{ ( U16 )( node->rect.off.x + resX ), node->rect.off.y },
			{ node->rect.off.x, ( U16 )( node->rect.off.y + resY ) }
		};
		SPixelVec2 splitExtents[ 2 ] = {
			{ splitRes[ 0 ], bestSplit==0 ? node->rect.res.y : resY },
			{ bestSplit==1 ? node->rect.res.x : resX, splitRes[ 1 ] }
		};

		// splits
		SNode *splits[ 2 ] = { nullptr, nullptr };

		// make each split
		for( U32 i=0; i<2; ++i ) {
			// don't make a split here if there's no space
			if( splitRes[ i ]==0 ) {
				continue;
			}

			// there's space for the split; allocate
			splits[ i ] = allocateNode( splitPoints[ i ].x, splitPoints[ i ].y, splitExtents[ i ].x, splitExtents[ i ].y );
			if( !splits[ i ] ) {
				// ... clean up (don't leave false free nodes!)
				if( i==1 ) {
					freeNode( splits[ 0 ] );
				}

				// we could not allocate
				return false;
			}
		}

		// list this node as an allocated texture (with the specified resolution)
		node->textureId = textureId;
		node->rect.res.x = resX;
		node->rect.res.y = resY;

		g_DebugLog += axf( "Marked %ux%u node at (%u,%u -> %u,%u)", resX, resY, node->rect.off.x, node->rect.off.y, node->rect.off.x+resX, node->rect.off.y+resY );

		// the node now marked and the partitions are in a valid state
		return true;
	}

	static Bool isAdjacent( const SPixelRect &a, const SPixelRect &b )
	{
		if( a.off.x + a.res.x == b.off.x || a.off.x == b.off.x + b.res.x ) {
			return a.off.y == b.off.y && a.res.y == b.res.y;
		}
		if( a.off.y + a.res.y == b.off.y || a.off.y == b.off.y + b.res.y ) {
			return a.off.x == b.off.x && a.res.x == b.res.x;
		}

		return false;
	}
	UPtr CRectangleAllocator::findAdjacentFreeNodes( SNode **out_nodes, UPtr maxOutNodes, SNode *node ) const
	{
		AX_ASSERT_NOT_NULL( out_nodes );
		AX_ASSERT( maxOutNodes >= 4 );
		AX_ASSERT_NOT_NULL( node );

		UPtr cAdjNodes = 0;

		const SPixelRect &mainRect = node->rect;
		for( SNode *p = head; p != nullptr; p = p->next ) {
			if( p == node || p->textureId != 0 || !isAdjacent( p->rect, mainRect ) ) {
				continue;
			}

			out_nodes[ cAdjNodes++ ] = p;
			if( cAdjNodes == maxOutNodes ) {
				break;
			}
		}

		return cAdjNodes;
	}
	Void CRectangleAllocator::mergeAdjacentFreeNodes( SNode *node )
	{
		static const UPtr kMaxAdjNodes = 4;
		SNode *pAdjNodes[ kMaxAdjNodes ];

		const UPtr cAdjNodes = findAdjacentFreeNodes( pAdjNodes, kMaxAdjNodes, node );
		for( UPtr i = 0; i < cAdjNodes; ++i ) {
			AX_ASSERT_NOT_NULL( pAdjNodes[ i ] );
			SNode &adj = *pAdjNodes[ i ];
			SPixelRect &rc = adj.rect;

			if( rc.off.x < node->rect.off.x ) {
				//AX_ASSERT( rc.off.y == node->rect.off.y );

				node->rect.off.x = rc.off.x;
				node->rect.res.x += rc.res.x;
			} else if( rc.off.x > node->rect.off.x ) {
				//AX_ASSERT( rc.off.y == node->rect.off.y );

				node->rect.res.x += rc.res.x;
			} else if( rc.off.y < node->rect.off.y ) {
				//AX_ASSERT( rc.off.x == node->rect.off.x );

				node->rect.off.y = rc.off.y;
				node->rect.res.y += rc.res.y;
			} else if( rc.off.y > node->rect.off.y ) {
				//AX_ASSERT( rc.off.x == node->rect.off.x );

				node->rect.res.y += rc.res.y;
			}

			freeNode( pAdjNodes[ i ] );
			pAdjNodes[ i ] = nullptr;
		}
	}

	/*
	===============================================================================

		TEXTURE MANAGER
		Controls all of the texture atlases, which in turn control all of the
		textures.

	===============================================================================
	*/

	// constructor
	MTextures::MTextures()
	: textures()
	, freeIds()
	, freeIds_sp(0)
	, defAtlasRes(0)
	, mgrAtlas_list()
	, mgrTex_list()
	{
		for( freeIds_sp=0; freeIds_sp<0xFFFF; ++freeIds_sp ) {
			freeIds[ freeIds_sp ] = 0xFFFF - freeIds_sp;
		}

		defAtlasRes = 1024;
	}
	// destructor
	MTextures::~MTextures()
	{
	}

	// pushes an "available for use" (free) texture identifier to the internal stack
	Void MTextures::pushFreeTextureId( U16 textureId )
	{
		AX_ASSERT_MSG( textureId>0, "Should not be pushing the nullptr id" );
		AX_ASSERT_MSG( freeIds_sp<MAX_TEXTURES - 1, "Should not have a full stack" );

		freeIds[ freeIds_sp++ ] = textureId;
	}
	// pops a free texture identifier from the internal stack
	U16 MTextures::popFreeTextureId()
	{
		if( freeIds_sp==0 ) {
			return 0;
		}

		return freeIds[ --freeIds_sp ];
	}

	// make a new texture
	RTexture *MTextures::makeTexture( U16 width, U16 height, const Void *data, ETextureFormat format, CTextureAtlas *specificAtlas )
	{
		CTextureAtlas *atlas = nullptr;
		RTexture *tex = nullptr;
		Bool atlasWasFound = false;

		// check the parameters
		if( !AX_VERIFY_NOT_NULL( data ) ) {
			return nullptr;
		}
		if( !AX_VERIFY_MSG( width > 0 && height > 0, "Invalid resolution" ) ) {
			return nullptr;
		}

		// try to allocate from an existing atlas first
		if( specificAtlas != nullptr ) {
			if( !AX_VERIFY_MSG( specificAtlas->getFormat() == format, "Invalid format for atlas" ) ) {
				return nullptr;
			}

			atlasWasFound = true;
			atlas = specificAtlas;

			tex = atlas->reserveTexture( width, height );
			if( !AX_VERIFY_MSG( tex != nullptr, "Atlas can't fit texture" ) ) {
				return nullptr;
			}
		} else {
			for( atlas = mgrAtlas_list.head(); atlas != nullptr; atlas = atlas->mgrAtlas_link.next() ) {
				if( atlas->getFormat() != format ) {
					continue;
				}

				tex = atlas->reserveTexture( width, height );
				if( tex!=nullptr ) {
					atlasWasFound = true;
					break;
				}
			}
		}

		// if we couldn't allocate the texture through an existing atlas, allocate
		// a new atlas altogether
		if( !tex ) {
			const U16 res = g_textureMgr.getDefaultAtlasResolution();

			atlas = allocateAtlas( format, res, res );
			if( !AX_VERIFY_MSG( atlas!=nullptr, "Atlas allocate failed" ) ) {
				return nullptr;
			}

			tex = atlas->reserveTexture( width, height );
			if( !AX_VERIFY_MSG( tex!=nullptr, "Reserve texture failed" ) ) {
				delete atlas;
				return nullptr;
			}
		}

#if DOLL_TEXTURE_MEMORY_ENABLED
		const UPtr totalSize = width*height*gfx_getTexelByteSize( format );
		if( !AX_VERIFY_MSG( tex->copyMemory( data, totalSize ), "Copy memory failed" ) ) {
			goto fail;
		}

		//
		//	TODO: Optimize the updates so it's done before the texture is needed
		//	-     automatically, with an optional `flushTextureUpdates` command or
		//	-     something like that to do it before then.
		//

		// update the atlas
		if( !AX_VERIFY_MSG( atlas->updateTextures( 1, &tex ), "Textures failed to update" ) ) {
			goto fail;
		}
#else
		if( !atlas->updateTexture( tex, format, data ) ) {
			goto fail;
		}
#endif

		// done
		return tex;

	fail:
		delete tex;
		tex = nullptr;

		if( atlasWasFound ) {
			delete atlas;
			atlas = nullptr;
		}

		return nullptr;
	}
	// load up a new texture
	RTexture *MTextures::loadTexture( Str filename, CTextureAtlas *specificAtlas )
	{
		//
		//	XXX: This needs to be fixed to support RGB8 too
		//

		LoadingTexture loading;

		// load the file
		do {
			U8 *filedata = nullptr;
			UPtr filesize = 0;

			if( !core_loadFile( filename, filedata, filesize, kTag_Texture ) ) {
				return nullptr;
			}

			// try to load up the image
			const Bool didLoad = loading.load( filename, TArr<U8>(filedata, filesize) );
			core_freeFile( filedata );

			if( !didLoad ) {
				g_DebugLog( filename ) += "Failed to load image.";
				return nullptr;
			}
		} while( false );

		const U32 width = loading.res_x();
		const U32 height = loading.res_y();
		const ETextureFormat format = loading.format();
		const U32 channels = loading.channels();
		U8 *const data = loading.data();

		// swap colors then copy the memory over
		for( U32 y=0; y<height; ++y ) {
			for( U32 x=0; x<width; ++x ) {
				U8 *const off = &data[ ( y*width + x )*channels ];
				const U8 tmp = off[ 2 ];
				off[ 2 ] = off[ 0 ];
				off[ 0 ] = tmp;
			}
		}

		// load the image as an actual texture
		RTexture *const tex = makeTexture( width, height, ( Void * )data, format, specificAtlas );

		// done
		return tex;
	}

	// allocate a texture atlas with the properties given
	CTextureAtlas *MTextures::allocateAtlas( ETextureFormat fmt, U16 resX, U16 resY )
	{
		CTextureAtlas *const atlas = new CTextureAtlas();
		if( !AX_VERIFY_MEMORY( atlas ) ) {
			return nullptr;
		}

		mgrAtlas_list.addTail( atlas->mgrAtlas_link );

		if( !AX_VERIFY_MSG( atlas->init( fmt, resX, resY ), "Init failed" ) ) {
			delete atlas;
			return nullptr;
		}

		return atlas;
	}
	// retrieve the texture handle from the identifier given
	RTexture *MTextures::getTextureById( U16 textureId ) const
	{
		return textures[ textureId ];
	}

	// retrieve the number of textures allocated
	U16 MTextures::getAllocatedTexturesCount() const
	{
		return ( MAX_TEXTURES - 1 ) - freeIds_sp;
	}

	// register a texture with a system returning its internal identifier
	U16 MTextures::procureTextureId()
	{
		return popFreeTextureId();
	}
	// store the value of a registered texture
	Void MTextures::setHandleForTextureId( U16 textureId, RTexture *handle )
	{
		AX_ASSERT_MSG( textureId!=0, "Invalid texture identifier" );
		AX_ASSERT_MSG( handle!=nullptr, "Invalid texture handle" );
		AX_ASSERT_MSG( textures[ textureId ]==nullptr, "RTexture already set" );

		textures[ textureId ] = handle;
	}
	// unregister a texture from the system, nullifying the internal pointer
	Void MTextures::nullifyTextureId( U16 textureId )
	{
		AX_ASSERT_MSG( textures[ textureId ]!=nullptr, "RTexture not set" );

		textures[ textureId ] = nullptr;
		pushFreeTextureId( textureId );
	}

	/*
	===============================================================================

		TEXTURE ATLAS

	===============================================================================
	*/

	// constructor
	CTextureAtlas::CTextureAtlas()
	: mgrAtlas_link( this )
	, texture( 0 )
	, resolution()
	, format( kTexFmtRGBA8 )
	, allocator()
	, atlas_list()
	{
		resolution.x = 0;
		resolution.y = 0;
	}
	// destructor
	CTextureAtlas::~CTextureAtlas()
	{
		fini();
		mgrAtlas_link.unlink();
	}

	// initialize the atlas with the given format and resolution
	Bool CTextureAtlas::init( ETextureFormat fmt, U16 resX, U16 resY )
	{
		AX_ASSERT_MSG( texture == 0, "RTexture already initialized" );
		AX_ASSERT_MSG( gfx_isTextureResolutionValid( resX, resY ), "Invalid resolution" );

		// set the resolution
		resolution.x = resX;
		resolution.y = resY;

		// must initialize the rectangle allocator
		if( !AX_VERIFY_MSG( allocator.init( resolution ), "allocator init failed" ) ) {
			return false;
		}

		// allocate the texture
		if( !AX_VERIFY_MSG( ( texture = gfx_r_createTexture( fmt, resX, resY, nullptr ) ) != 0, "Failed to create texture" ) ) {
			allocator.fini();
			return false;
		}

		// set the format
		format = fmt;

		// done
		return true;
	}
	// finish the atlas
	Void CTextureAtlas::fini()
	{
		if( !texture ) {
			return;
		}

		TIntrLink< RTexture > *link, *next;
		for( link=atlas_list.headLink(); link!=nullptr; link=next ) {
			next = link->nextLink();

			RTexture *const tex = link->node();
			AX_ASSERT_NOT_NULL( tex );

			tex->fini();
		}
		allocator.fini();

		gfx_r_destroyTexture( texture );
		texture = 0;
	}

#if DOLL_TEXTURE_MEMORY_ENABLED
	// supply a list of updates to the atlas
	Bool CTextureAtlas::updateTextures( UPtr numTextures, const RTexture *const *textures )
	{
		AX_ASSERT_MSG( textures!=nullptr, "Need the texture array" );

		if( !numTextures ) {
			return true;
		}

		for( UPtr i=0; i<numTextures; ++i ) {
			const RTexture *const src = textures[ i ];
			AX_ASSERT_NOT_NULL( src );

			const U8 *const mem = ( const U8 * )src->getMemoryPointer();
			if( !mem ) {
				continue;
			}

			const SPixelRect srcRect = src->getAtlasRectangle();
			const SPixelVec2 srcRes = src->getResolution();

			gfx_r_updateTexture( texture, srcRect.off.x, srcRect.off.y, srcRes.x, srcRes.y, mem );
		}

		return true;
	}
#else
	// update a texture in the atlas
	Bool CTextureAtlas::updateTexture( const RTexture *tex, ETextureFormat fmt, const Void *data )
	{
		AX_ASSERT_NOT_NULL( tex );
		AX_ASSERT( tex->atlas == this );

		const SPixelRect rc = tex->getAtlasRectangle();
		const SPixelVec2 sz = tex->getResolution();

		( void )fmt;
		gfx_r_updateTexture( texture, rc.off.x, rc.off.y, sz.x, sz.y, ( const U8 * )data );

		return true;
	}
#endif

	// allocate a texture within this system
	RTexture *CTextureAtlas::reserveTexture( U16 width, U16 height )
	{
		SPixelRect texRect;
		SPixelVec2 texRes;
		U16 texId;

		texId = g_textureMgr.procureTextureId();
		if( !AX_VERIFY_MSG( texId!=0, "No more textures available" ) ) {
			return nullptr;
		}

		texRes.x = width;
		texRes.y = height;

		Void *const texNode = allocator.allocateId( texRect, texId, texRes );
		if( !texNode ) {
			return nullptr;
		}

		RTexture *const texPtr = new RTexture( this, texRect, texNode, texId );
		if( !AX_VERIFY_MEMORY( texPtr ) ) {
			return nullptr;
		}

		g_textureMgr.setHandleForTextureId( texId, texPtr );

		atlas_list.addTail( texPtr->atlas_link );
		g_textureMgr.mgrTex_list.addTail( texPtr->mgrTex_link );

		return texPtr;
	}

	/*
	===============================================================================

		TEXTURE

	===============================================================================
	*/

	// constructor
	RTexture::RTexture( CTextureAtlas *mainAtlas, const SPixelRect &rect, Void *allocNode, U16 textureId )
	: atlas( mainAtlas )
	, texRect( rect )
	, allocNode( allocNode )
	, ident( textureId )
	, name( nullptr )
#if DOLL_TEXTURE_MEMORY_ENABLED
	, memory( nullptr )
#endif
	{
		AX_ASSERT_MSG( mainAtlas!=nullptr, "Cannot pass nullptr for the atlas" );
		AX_ASSERT_MSG( allocNode!=nullptr, "You need a valid allocation node" );
	}
	// destructor
	RTexture::~RTexture()
	{
		fini();
	}

	// translate coordinates of this texture into the texture atlas
	Void RTexture::translateCoordinates( STextureRect &dst, const SPixelRect &src ) const
	{
		static const int negateAmount = 1;
		SPixelRect adjSrc;
		SPixelRect atlasCoords;

		adjSrc.off.x = src.off.x >= texRect.res.x ? 0 : src.off.x;
		adjSrc.off.y = src.off.y >= texRect.res.y ? 0 : src.off.y;
		adjSrc.res.x = adjSrc.off.x + src.res.x > texRect.res.x ? texRect.res.x : src.res.x;
		adjSrc.res.y = adjSrc.off.y + src.res.y > texRect.res.y ? texRect.res.y : src.res.y;

		atlasCoords.res.x = ( adjSrc.res.x - negateAmount );
		atlasCoords.res.y = ( adjSrc.res.y - negateAmount );

		atlasCoords.off.x = texRect.off.x + adjSrc.off.x;
		atlasCoords.off.y = texRect.off.y + adjSrc.off.y;

		const SPixelVec2 atlasRes = atlas->getResolution();
		const Vec2f texOff = Vec2f( 1.0f/F32( atlasRes.x ), 1.0f/F32( atlasRes.y ) );
		//const Vec2f texOff( 0.0f, 0.0f );

		dst.pos[ 0 ] = texOff.x/2 + ( F32 )atlasCoords.off.x/( F32 )atlasRes.x;
		dst.pos[ 1 ] = texOff.y/2 + ( F32 )atlasCoords.off.y/( F32 )atlasRes.y;
		dst.res[ 0 ] = texOff.x/2 + ( F32 )atlasCoords.res.x/( F32 )atlasRes.x;
		dst.res[ 1 ] = texOff.y/2 + ( F32 )atlasCoords.res.y/( F32 )atlasRes.y;

#if 0
		static int iDebugCount = 0;
		if( iDebugCount == 2 ) {
			return;
		}
		++iDebugCount;

		g_DebugLog += axf( "%ux%u block at (%u,%u) becomes %.4fx%.4f units at (%.4f,%.4f)", src.res.x, src.res.y, atlasCoords.off.x, atlasCoords.off.y, dst.res[0], dst.res[1], dst.pos[0], dst.pos[1] );
#endif
	}

	// set the name of this texture
	Bool RTexture::setName( Str newName )
	{
		if( newName.isEmpty() ) {
			name.purge();
			return true;
		}

		name.clear();

		return AX_VERIFY_MEMORY( name.tryAssign( newName ) );
	}
	// retrieve the name of this texture
	Str RTexture::getName() const
	{
		return name;
	}

	// finish
	Void RTexture::fini()
	{
		if( !atlas ) {
			return;
		}

		AX_ASSERT_MSG( allocNode!=nullptr, "Invalid texture object" );

#if DOLL_TEXTURE_MEMORY_ENABLED
		delete [] memory;
		memory = nullptr;
#endif

		atlas->freeNode( allocNode );
		allocNode = nullptr;

		atlas_link.unlink();
		atlas = nullptr;

		if( ident!=0 ) {
			g_textureMgr.nullifyTextureId( ident );
		}

		texRect.off.x = 0;
		texRect.off.y = 0;
		texRect.res.x = 0;
		texRect.res.y = 0;

		name.purge();
	}


	DOLL_FUNC CTextureAtlas *DOLL_API gfx_newTextureAtlas( U16 resX, U16 resY, U16 format )
	{
		return g_textureMgr.allocateAtlas( ( ETextureFormat )format, resX, resY );
	}
	DOLL_FUNC CTextureAtlas *DOLL_API gfx_deleteTextureAtlas( CTextureAtlas *atlas )
	{
		delete atlas;
		return nullptr;
	}

	DOLL_FUNC RTexture *DOLL_API gfx_newTexture( U16 width, U16 height, const void *data, ETextureFormat format )
	{
		return g_textureMgr.makeTexture( width, height, data, format );
	}
	DOLL_FUNC RTexture *DOLL_API gfx_newTextureInAtlas( U16 width, U16 height, const void *data, ETextureFormat format, CTextureAtlas *atlas )
	{
		AX_ASSERT_NOT_NULL( atlas );

		return g_textureMgr.makeTexture( width, height, data, format, atlas );
	}

	DOLL_FUNC RTexture *DOLL_API gfx_loadTexture( Str filename )
	{
		AX_ASSERT( filename.isUsed() );

		return g_textureMgr.loadTexture( filename );
	}
	DOLL_FUNC RTexture *DOLL_API gfx_loadTextureInAtlas( Str filename, CTextureAtlas *atlas )
	{
		AX_ASSERT( filename.isUsed() );
		AX_ASSERT_NOT_NULL( atlas );

		return g_textureMgr.loadTexture( filename, atlas );
	}

	DOLL_FUNC RTexture *DOLL_API gfx_deleteTexture( RTexture *texture )
	{
		if( texture != nullptr ) {
			delete texture;
		}

		return nullptr;
	}
	DOLL_FUNC U32 DOLL_API gfx_getTextureResX( const RTexture *texture )
	{
		AX_ASSERT_NOT_NULL( texture );
		return ( U32 )texture->getResolution().x;
	}
	DOLL_FUNC U32 DOLL_API gfx_getTextureResY( const RTexture *texture )
	{
		AX_ASSERT_NOT_NULL( texture );
		return ( U32 )texture->getResolution().y;
	}

}
