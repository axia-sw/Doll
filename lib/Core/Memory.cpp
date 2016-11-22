#include "doll/Core/Memory.hpp"
#include "doll/Core/MemoryTags.hpp"
#include "doll/Core/Logger.hpp"

namespace doll
{

	namespace Mem
	{

		/*
		=======================================================================

			BASIC MEMORY MANAGEMENT

		=======================================================================
		*/

		void copy( void *dst, const void *src, UPtr n )
		{
			AX_ASSERT_NOT_NULL( dst );
			AX_ASSERT_NOT_NULL( src );

			if( !n ) {
				return;
			}

			memcpy( dst, src, n );
		}
		void move( void *dst, UPtr dstMax, const void *src, UPtr n )
		{
			AX_ASSERT_NOT_NULL( dst );
			AX_ASSERT_NOT_NULL( src );
			AX_ASSERT_MSG( dstMax > 0, "Must have buffer" );

#if defined( _MSC_VER ) && defined( __STDC_WANT_SECURE_LIB__ )
			memmove_s( dst, dstMax, src, n );
#else
			memmove( dst, src, dstMax < n ? dstMax : n );
#endif
		}
		void fill( void *dst, UPtr dstMax, U8 v )
		{
			AX_ASSERT_NOT_NULL( dst );
			AX_ASSERT_MSG( dstMax > 0, "Must have buffer" );

			memset( dst, v, dstMax );
		}




		/*
		=======================================================================

			ALLOCATION TAGGING

		=======================================================================
		*/
#if DOLL_TAGS_ENABLED
		struct STagStats
		{
			const char *name;

			STagStatsShared global;
			STagStatsShared local;

			inline STagStats()
			: name( nullptr )
			, global()
			, local()
			{
			}
			inline ~STagStats()
			{
			}
		};

		static STagStats tags[ kMaxTags ];

		void initTags()
		{
			memset( &tags[ 0 ], 0, sizeof( tags ) );

			for( UPtr i = 0; i < kMaxTags; ++i ) {
				tags[ i ].name = kTagNames[ i ];
			}
		}
		void updateTags()
		{
			for( UPtr i = 0; i < kMaxTags; ++i ) {
				tags[ i ].global += tags[ i ].local;
				tags[ i ].local.reset();
			}
		}

		static const STagStatsShared nullStats;
		static bool reportingEnabled = false;

		const STagStatsShared &getTagFrameStats( int tag )
		{
			if( ( UPtr )tag >= kMaxTags ) {
				return nullStats;
			}

			return tags[ tag ].local;
		}
		const STagStatsShared &getTagAppStats( int tag )
		{
			if( ( UPtr )tag >= kMaxTags ) {
				return nullStats;
			}

			return tags[ tag ].global;
		}
		const char *getTagName( int tag )
		{
			if( ( UPtr )tag >= kMaxTags ) {
				return "";
			}

			return tags[ tag ].name;
		}

		void enableTagReporting()
		{
			reportingEnabled = true;
		}
		void disableTagReporting()
		{
			reportingEnabled = false;
		}
		bool isTagReportingEnabled()
		{
			return reportingEnabled;
		}

		void tagAlloc( void *p, UPtr size, int tag, const char *file,
					   int line, const char *func )
		{
			( Void )p;
			
			if( ( UPtr )tag >= kMaxTags ) {
				char buf[ 64 ];

				axspf( buf, "alloc: invalid tag (%i)", tag );
				g_ErrorLog( file, line, func ) += buf;
				return;
			}

			STagStatsShared &t = tags[ tag ].local;

			++t.numAllocs;
			t.totalAllocSize += size;

			if( t.numAllocs == 1 || size < t.minAllocSize ) {
				t.minAllocSize = size;
			}
			if( size > t.maxAllocSize ) {
				t.maxAllocSize = size;
			}

			t.avgAllocSize += ( UPtr )( double( size - t.avgAllocSize )/
										  double( t.avgAllocSize ) );

			if( reportingEnabled ) {
				char buf[ 512 ];

				axspf( buf, "alloc<%i>(%zu)", tag, size );
				g_DebugLog( file, line, func ) += buf;
			}
		}
		void tagDealloc( void *p, UPtr size, int tag, const char *file,
						 int line, const char *func )
		{
			( Void )p;
			
			if( ( UPtr )tag >= kMaxTags ) {
				char buf[ 64 ];

				axspf( buf, "dealloc: invalid tag (%i)", tag );
				g_ErrorLog( file, line, func ) += buf;
				return;
			}

			STagStatsShared &t = tags[ tag ].local;

			++t.numDeallocs;
			t.totalDeallocSize += size;

			if( reportingEnabled ) {
				char buf[ 512 ];

				axspf( buf, "dealloc<%i>(%zu)", tag, size );
				g_DebugLog( file, line, func ) += buf;
			}
		}
#endif

	}

	CAllocator_Heap CAllocator_Heap::gInstance;
	CAllocator_Null CAllocator_Null::gInstance;

	DOLL_FUNC IAllocator *DOLL_API mem_getHeapAllocator()
	{
		return &CAllocator_Heap::gInstance;
	}
	DOLL_FUNC IAllocator *DOLL_API mem_getDefaultAllocator()
	{
		return gDefaultAllocator;
	}
	DOLL_FUNC IAllocator *DOLL_API mem_getOverflowAllocator()
	{
		return gOverflowAllocator;
	}

	/*
	===========================================================================

	HEAP ALLOCATOR

	===========================================================================
	*/
	CAllocator_Heap::CAllocator_Heap()
	{
	}
	CAllocator_Heap::~CAllocator_Heap()
	{
	}

	void *CAllocator_Heap::allocate( UPtr size )
	{
		return size > 0 ? malloc( size ) : nullptr;
	}
	void *CAllocator_Heap::deallocate( void *p )
	{
		free( p );
		return nullptr;
	}

	/*
	===========================================================================

	NODE ALLOCATOR

	===========================================================================
	*/
#ifndef _MSC_VER
	CAllocator_Node::CAllocator_Node( void *buffer, UPtr bufferSize, UPtr nodeSize, IAllocator *overflow )
	: mBuffer( ( char * )buffer )
	, mBufferSize( bufferSize )
	, mNodeSize( nodeSize )
	, mOverflowAllocator( overflow )
	, mNumNodes( 0 )
	, mPeakNodeCount( 0 )
	{
		AX_ASSERT_NOT_NULL( buffer );
		AX_ASSERT( bufferSize > 0 );
		AX_ASSERT( nodeSize >= sizeof( void * ) );
		AX_ASSERT_NOT_NULL( overflow );

		initBuffer();
	}
#endif
	CAllocator_Node::~CAllocator_Node()
	{
	}

	void *CAllocator_Node::allocate( UPtr size )
	{
		AX_ASSERT_MSG( size == mNodeSize, "Size does not match expected" );
		if( size != mNodeSize ) {
			return nullptr;
		}

		// if there aren't any free nodes then need to use overflow allocator
		if( !mFreeNode ) {
			void *const p = overAlloc( size );
			if( !p ) {
				return nullptr;
			}

			// keep track of the number of nodes we're using
			incNodes();
			return p;
		}

		// take the free node out of the free list; set free node to the next
		// one in the list
		void **const ptr = mFreeNode;
		mFreeNode = ( void ** )*mFreeNode;
		*ptr = nullptr;

		// successfully found a node
		incNodes();
		return ( void * )ptr;
	}
	void *CAllocator_Node::deallocate( void *p )
	{
		if( !p ) {
			return nullptr;
		}

		// we're getting rid of a node
		decNodes();

		// handle nodes allocated through the overflow allocator
		if( !isPointerOwned( p ) ) {
			return overDealloc( p );
		}

		// add this node to the free nodes list
		void **const ptr = ( void ** )p;
		*ptr = mFreeNode;
		mFreeNode = ptr;

		// nullptr on return (allows for: 'ptr = deallocate( ptr );')
		return nullptr;
	}

	void CAllocator_Node::initBuffer()
	{
		mFreeNode = nullptr;
		for( UPtr index = 0; index < mBufferSize; index += mNodeSize ) {
			void **const ptr = ( void ** )&mBuffer[ index ];

			*ptr = mFreeNode;
			mFreeNode = ptr;
		}
	}




	/*
	===========================================================================

		LINEAR ALLOCATOR

	===========================================================================
	*/
#ifndef _MSC_VER
	CAllocator_Linear::CAllocator_Linear( void *buffer, UPtr bufferSize )
	: mBuffer( ( char * )buffer )
	, mBufferSize( bufferSize )
	, mOffset( 0 )
	, mPeakOffset( 0 )
	{
		AX_ASSERT_NOT_NULL( buffer );
		AX_ASSERT( bufferSize > 0 );
	}
#endif
	CAllocator_Linear::~CAllocator_Linear()
	{
	}

	void *CAllocator_Linear::allocate( UPtr size )
	{
		if( mOffset + size > mBufferSize ) {
			return nullptr;
		}

		void *const p = &mBuffer[ mOffset ];

		mOffset += size;
		if( mOffset > mPeakOffset ) {
			mPeakOffset = mOffset;
		}

		return p;
	}
	void *CAllocator_Linear::deallocate( void *p )
	{
		( void )p;
		return nullptr;
	}

}
