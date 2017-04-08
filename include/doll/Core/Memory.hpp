#pragma once

#include "Defs.hpp"

#ifndef DOLL_TAGS_ENABLED
# if AX_DEBUG_ENABLED
#  define DOLL_TAGS_ENABLED 1
# else
#  define DOLL_TAGS_ENABLED 0
# endif
#endif

#undef DOLL_TAGS_ENABLED
#define DOLL_TAGS_ENABLED 1

namespace doll
{

#define DOLL_ALIGN(X_,A_)\
	( (X_) + ( ( (A_) - ( (X_)%(A_) ) )%(A_) ) )

	inline U32 align( U32 x, U32 alignment )
	{
		return DOLL_ALIGN( x, alignment );
		//return x + ( ( alignment - ( x%alignment ) )%alignment );
	}
	inline U64 align( U64 x, U64 alignment )
	{
		return DOLL_ALIGN( x, alignment );
	}
	inline U64 align( U64 x, U32 alignment )
	{
		return DOLL_ALIGN( x, alignment );
	}

	namespace Mem
	{

		void copy( void *dst, const void *src, UPtr n );
		void move( void *dst, UPtr dstMax, const void *src, UPtr n );
		void fill( void *dst, UPtr dstMax, U8 v );
		inline void zero( void *dst, UPtr dstMax )
		{
			fill( dst, dstMax, 0 );
		}

		template< UPtr tSize >
		inline void copy( char( &dst )[ tSize ], const void *src )
		{
			copy( ( void * )dst, src, tSize );
		}
		template< UPtr tSize >
		inline void move( char( &dst )[ tSize ], const void *src, UPtr n )
		{
			move( ( void * )dst, tSize, src, n );
		}
		template< UPtr tSize >
		inline void fill( char( &dst )[ tSize ], U8 v )
		{
			fill( ( void * )dst, tSize, v );
		}
		template< UPtr tSize >
		inline void zero( char( &dst )[ tSize ] )
		{
			zero( ( void * )dst, tSize );
		}

#if DOLL_TAGS_ENABLED
		struct STagStatsShared
		{
			UPtr numAllocs;
			UPtr numDeallocs;

			UPtr minAllocSize;
			UPtr maxAllocSize;
			UPtr avgAllocSize;

			UPtr totalAllocSize;
			UPtr totalDeallocSize;

			inline STagStatsShared(): numAllocs( 0 ), numDeallocs( 0 ),
				minAllocSize( 0 ), maxAllocSize( 0 ), avgAllocSize( 0 ),
				totalAllocSize( 0 ), totalDeallocSize( 0 )
			{
			}
			inline ~STagStatsShared()
			{
			}

			inline void reset()
			{
				numAllocs = 0;
				numDeallocs = 0;

				minAllocSize = 0;
				maxAllocSize = 0;
				avgAllocSize = 0;

				totalAllocSize = 0;
				totalDeallocSize = 0;
			}
			inline STagStatsShared operator+( const STagStatsShared &x ) const
			{
				STagStatsShared r = *this;

				r.numAllocs += x.numAllocs;
				r.numDeallocs += x.numDeallocs;

				if( x.minAllocSize < r.minAllocSize ) {
					r.minAllocSize = x.minAllocSize;
				}
				if( x.maxAllocSize > r.maxAllocSize ) {
					r.maxAllocSize = x.maxAllocSize;
				}

				r.avgAllocSize = ( UPtr )( double( x.avgAllocSize +
					r.avgAllocSize )/2.0 );

				r.totalAllocSize += x.totalAllocSize;
				r.totalDeallocSize += x.totalDeallocSize;

				return r;
			}

			inline STagStatsShared &operator+=( const STagStatsShared &x )
			{
				*this = *this + x;
				return *this;
			}
		};

		struct STagThunk
		{
			int tag;
			UPtr size;
		};

		void initTags();
		void updateTags();

		const STagStatsShared &getTagFrameStats( int tag );
		const STagStatsShared &getTagAppStats( int tag );
		const char *getTagName( int tag );

		void enableTagReporting();
		void disableTagReporting();
		bool isTagReportingEnabled();

		void tagAlloc( void *p, UPtr size, int tag, const char *file, int line, const char *func );
		void tagDealloc( void *p, UPtr size, int tag, const char *file, int line, const char *func );
#endif

	}




	/*
	===========================================================================

		STATIC MEMORY BUFFER

	===========================================================================
	*/

	template< UPtr tSize >
	class TStaticMemoryBuffer
	{
	public:
		inline TStaticMemoryBuffer()
		{
			Mem::zero( mBuffer );
		}
		inline ~TStaticMemoryBuffer()
		{
		}

		inline void *ptr()
		{
			return mBuffer;
		}
		inline UPtr size() const
		{
			return tSize;
		}
		inline U8( &buffer() )[ tSize ] {
			return mBuffer;
		}

	private:
		U8 mBuffer[ tSize ];
	};




	/*
	===========================================================================

		ALLOCATOR - BASE INTERFACE

	===========================================================================
	*/

	class IAllocator
	{
	public:
		inline IAllocator()
		{
		}
		inline virtual ~IAllocator()
		{
		}

		inline void *alloc( UPtr size, int tag, const char *file, int line, const char *func )
		{
#if DOLL_TAGS_ENABLED
			void *p = allocate( size + sizeof( Mem::STagThunk ) );

			( ( Mem::STagThunk * )p )->tag = tag;
			( ( Mem::STagThunk * )p )->size = size;

			Mem::tagAlloc( p, size, tag, file, line, func );

			return ( void * )( ( ( char * )p ) + sizeof( Mem::STagThunk ) );
#else
			return allocate( size );
#endif
		}
		inline void *dealloc( void *p, const char *file, int line, const char *func )
		{
			if( !p ) {
				return nullptr;
			}

#if DOLL_TAGS_ENABLED
			p = ( void * )( ( ( char * )p ) - sizeof( Mem::STagThunk ) );

			const auto tag = ( ( const Mem::STagThunk * )p )->tag;
			const auto size = ( ( const Mem::STagThunk * )p )->size;
			Mem::tagDealloc( p, size, tag, file, line, func );
#endif

			return deallocate( p );
		}

	protected:
		virtual void *allocate( UPtr size ) = 0;
		virtual void *deallocate( void *p ) = 0;
	};




	/*
	===========================================================================

		ALLOCATOR - HEAP-BASED DYNAMIC MEMORY ALLOCATOR

	===========================================================================
	*/

	class CAllocator_Heap: public virtual IAllocator
	{
	public:
#ifdef DOLL__BUILD
		static CAllocator_Heap gInstance;
#endif

		CAllocator_Heap();
		virtual ~CAllocator_Heap();

	protected:
		virtual void *allocate( UPtr size ) override;
		virtual void *deallocate( void *p ) override;
	};

#ifdef DOLL__BUILD
	static CAllocator_Heap *const gDefaultAllocator = &CAllocator_Heap::gInstance;
#endif

	DOLL_FUNC IAllocator *DOLL_API mem_getHeapAllocator();
#ifdef DOLL__BUILD
# define DOLL__HEAP_ALLOCATOR (&CAllocator_Heap::gInstance)
#else
# define DOLL__HEAP_ALLOCATOR mem_getHeapAllocator()
#endif

	DOLL_FUNC IAllocator *DOLL_API mem_getDefaultAllocator();
#ifdef DOLL__BUILD
# define DOLL__DEFAULT_ALLOCATOR gDefaultAllocator
#else
# define DOLL__DEFAULT_ALLOCATOR mem_getDefaultAllocator()
#endif

#define DOLL__TEMP_ALLOCATOR DOLL__DEFAULT_ALLOCATOR




	/*
	===========================================================================

		ALLOCATOR - NULL

	===========================================================================
	*/

	class CAllocator_Null: public virtual IAllocator
	{
	public:
#ifdef DOLL__BUILD
		static CAllocator_Null gInstance;
#endif

		inline CAllocator_Null()
		{
		}
		inline virtual ~CAllocator_Null()
		{
		}

	protected:
		inline virtual void *allocate( UPtr ) override
		{
			return nullptr;
		}
		inline virtual void *deallocate( void * ) override
		{
			return nullptr;
		}
	};




	//
	//	Overflow allocator
	//
#define DOLL__OVERFLOW_ALLOCATOR_DECL( C )\
	static C *const gOverflowAllocator = &C::gInstance

#ifdef DOLL__BUILD
# if AX_DEBUG_ENABLED
	DOLL__OVERFLOW_ALLOCATOR_DECL( CAllocator_Heap );
# else
	DOLL__OVERFLOW_ALLOCATOR_DECL( CAllocator_Null );
# endif
#endif

	DOLL_FUNC IAllocator *DOLL_API mem_getOverflowAllocator();
#ifdef DOLL__BUILD
# define DOLL__OVERFLOW_ALLOCATOR gOverflowAllocator
#else
# define DOLL__OVERFLOW_ALLOCATOR mem_getOverflowAllocator()
#endif




	/*
	===========================================================================

		ALLOCATOR - NODE-BASED STATIC MEMORY ALLOCATOR

	===========================================================================
	*/

	class CAllocator_Node: public virtual IAllocator
	{
	public:
#ifndef _MSC_VER
		CAllocator_Node( void *buffer, UPtr bufferSize, UPtr nodeSize, IAllocator *overflow = DOLL__OVERFLOW_ALLOCATOR );

		template< UPtr tSize >
		inline CAllocator_Node( char( &buffer )[ tSize ], UPtr nodeSize, IAllocator *overflow = DOLL__OVERFLOW_ALLOCATOR )
		: CAllocator_Node( &buffer[ 0 ], tSize, nodeSize, overflow )
		{
		}
#else
		inline CAllocator_Node( void *buffer, UPtr bufferSize, UPtr nodeSize, IAllocator *overflow )
		: mBuffer( ( char * )buffer )
		, mBufferSize( bufferSize )
		, mNodeSize( nodeSize )
		, mOverflowAllocator( overflow )
		, mNumNodes( 0 )
		, mPeakNodeCount( 0 )
		{
			AX_ASSERT_NOT_NULL( buffer );
			AX_ASSERT_MSG( bufferSize > 0, "Must have buffer" );
			AX_ASSERT_MSG( nodeSize >= sizeof( void * ), "Node size too small" );
			AX_ASSERT_NOT_NULL( overflow );

			initBuffer();
		}

		template< UPtr tSize >
		inline CAllocator_Node( char( &buffer )[ tSize ], UPtr nodeSize, IAllocator *overflow = DOLL__OVERFLOW_ALLOCATOR )
		: mBuffer( ( char * )buffer )
		, mBufferSize( tSize )
		, mNodeSize( nodeSize )
		, mOverflowAllocator( overflow )
		, mNumNodes( 0 )
		, mPeakNodeCount( 0 )
		{
			AX_ASSERT_NOT_NULL( buffer );
			AX_ASSERT_MSG( tSize > 0, "Must have buffer" );
			AX_ASSERT_MSG( nodeSize >= sizeof( void * ), "Node size too small" );
			AX_ASSERT_NOT_NULL( overflow );

			initBuffer();
		}
#endif

		virtual ~CAllocator_Node();

		inline UPtr getMaxNodeCount() const
		{
			return mBufferSize/mNodeSize;
		}

		inline UPtr getNodeCount() const
		{
			return mNumNodes;
		}
		inline UPtr getOverflowNodeCount() const
		{
			if( mNumNodes > getMaxNodeCount() ) {
				return mNumNodes - getMaxNodeCount();
			}

			return 0;
		}
		inline UPtr getPeakNodeCount() const
		{
			return mPeakNodeCount;
		}
		inline UPtr getPeakOverflowNodeCount() const
		{
			if( mPeakNodeCount > getMaxNodeCount() ) {
				return mPeakNodeCount - getMaxNodeCount();
			}

			return 0;
		}

	protected:
		virtual void *allocate( UPtr size ) override;
		virtual void *deallocate( void *p ) override;

	private:
		char *const       mBuffer;
		const UPtr        mBufferSize;
		const UPtr        mNodeSize;
		IAllocator *const mOverflowAllocator;
		void **           mFreeNode;
		UPtr              mNumNodes;
		UPtr              mPeakNodeCount;

		void initBuffer();

		inline bool isPointerOwned( void *ptr ) const
		{
			return ( ( UPtr )( ( char * )ptr - mBuffer ) ) < mBufferSize;
		}
		inline void incNodes()
		{
			++mNumNodes;
			if( mNumNodes > mPeakNodeCount ) {
				mPeakNodeCount = mNumNodes;
			}
		}
		inline void decNodes()
		{
			AX_ASSERT_MSG( mNumNodes > 0, "Invalid state" );
			--mNumNodes;
		}

		inline void *overAlloc( UPtr size )
		{
			AX_ASSERT_NOT_NULL( mOverflowAllocator );

			return mOverflowAllocator->alloc( size, 0, nullptr, 0, nullptr );
		}
		inline void *overDealloc( void *ptr )
		{
			AX_ASSERT_NOT_NULL( mOverflowAllocator );

			return mOverflowAllocator->dealloc( ptr, nullptr, 0, nullptr );
		}
	};




	/*
	===========================================================================

		ALLOCATOR - LINEAR STATIC MEMORY ALLOCATOR

	===========================================================================
	*/

	class CAllocator_Linear: public virtual IAllocator
	{
	public:
#ifndef _MSC_VER
		CAllocator_Linear( void *buffer, UPtr bufferSize );

		template< UPtr tSize >
		inline CAllocator_Linear( char( &buffer )[ tSize ] )
		: CAllocator_Linear( ( void * )( &buffer[ 0 ] ), tSize )
		{
		}
#else
		inline CAllocator_Linear( void *buffer, UPtr bufferSize )
		: mBuffer( ( char * )buffer )
		, mBufferSize( bufferSize )
		, mOffset( 0 )
		, mPeakOffset( 0 )
		{
			AX_ASSERT_NOT_NULL( buffer );
			AX_ASSERT_MSG( bufferSize > 0, "Must have buffer" );
		}

		template< UPtr tSize >
		inline CAllocator_Linear( char( &buffer )[ tSize ] )
		: mBuffer( ( char * )buffer )
		, mBufferSize( tSize )
		, mOffset( 0 )
		, mPeakOffset( 0 )
		{
			AX_ASSERT_NOT_NULL( buffer );
			AX_ASSERT_MSG( tSize > 0, "Must have buffer" );
		}
#endif

		virtual ~CAllocator_Linear();

		inline UPtr getAllocatedBytes() const
		{
			return mOffset;
		}
		inline UPtr getPeakAllocatedBytes() const
		{
			return mPeakOffset;
		}

		inline void reset( UPtr newOffset = 0 )
		{
			AX_ASSERT_MSG( newOffset <= mOffset, "Invalid offset" );

			mOffset = newOffset;
		}

	protected:
		virtual void *allocate( UPtr size ) override;
		virtual void *deallocate( void *p ) override;

	private:
		char *const mBuffer;
		const UPtr  mBufferSize;
		UPtr        mOffset;
		UPtr        mPeakOffset;
	};

	//------------------------------------------------------------------------------

	/*
	===========================================================================

		ALLOCATOR POOL - HEAP ALLOCATOR

	===========================================================================
	*/

	struct SHeapAllocatorPool
	{
		inline void *alloc( UPtr n, int tag, const char *file, int line, const char *func )
		{
			return DOLL__HEAP_ALLOCATOR->alloc( n, tag, file, line, func );
		}
		inline void *dealloc( void *p, const char *file, int line, const char *func )
		{
			return DOLL__HEAP_ALLOCATOR->dealloc( p, file, line, func );
		}
	};




	/*
	===========================================================================

		ALLOCATOR POOL - NODE ALLOCATOR

	===========================================================================
	*/

	template< UPtr tSize, UPtr tNodeSize >
	struct TNodeAllocatorPool
	{
		TStaticMemoryBuffer< tSize > mBuffer;
		CAllocator_Node              mAllocator;

		inline TNodeAllocatorPool()
		: mBuffer()
		, mAllocator( mBuffer.buffer(), tNodeSize )
		{
		}
		inline ~TNodeAllocatorPool()
		{
		}

		inline void *alloc( UPtr n, int tag, const char *file, int line, const char *func )
		{
			return mAllocator.alloc( n, tag, file, line, func );
		}
		inline void *dealloc( void *p, const char *file, int line, const char *func )
		{
			return mAllocator.dealloc( p, file, line, func );
		}
	};




	/*
	===========================================================================

		ALLOCATOR POOL - LINEAR ALLOCATOR

	===========================================================================
	*/

	template< UPtr tSize >
	struct TLinearAllocatorPool
	{
		TStaticMemoryBuffer< tSize > mBuffer;
		CAllocator_Linear mAllocator;

		inline TLinearAllocatorPool()
		: mBuffer()
		, mAllocator( mBuffer.buffer() )
		{
		}
		inline ~TLinearAllocatorPool()
		{
		}

		inline void *alloc( UPtr n, int tag, const char *file, int line, const char *func )
		{
			return mAllocator.alloc( n, tag, file, line, func );
		}
		inline void *dealloc( void *p, const char *file, int line, const char *func )
		{
			return mAllocator.dealloc( p, file, line, func );
		}
	};




	/*
	===========================================================================

		POOL OBJECT
		Forces derived classes to use the given allocator pool on new/delete.

	===========================================================================
	*/

	template< typename T, int tTag = 0, typename PoolT = SHeapAllocatorPool >
	class TPoolObject
	{
	public:
		inline TPoolObject()
		{
		}
		inline virtual ~TPoolObject()
		{
		}

		inline void *operator new( SizeType n, int tag = tTag, const char *file = nullptr, int line = 0, const char *func = nullptr )
		{
			return gPool.alloc( n, tag, file, line, func );
		}
#if defined( _MSC_VER )
		inline void operator delete( void *p, int tag, const char *file, int line, const char *func )
		{
			( void )p;
			( void )tag;
			( void )file;
			( void )line;
			( void )func;

			AX_ASSERT_MSG( false, "Don't call this function" );
		}
#endif
		inline void operator delete( void *p, const char *file, int line, const char *func )
		{
			gPool.dealloc( p, file, line, func );
		}
		inline void operator delete( void *p )
		{
			gPool.dealloc( p, nullptr, 0, nullptr );
		}

	protected:
		static PoolT gPool;
	};

	template< typename T, int tTag, typename PoolT >
	PoolT TPoolObject< T, tTag, PoolT >::gPool;

	template< typename T >
	T *alloc( const char *pszFile = nullptr, U32 line = 0, const char *pszFunc = nullptr )
	{
		AX_ASSERT_NOT_NULL( DOLL__DEFAULT_ALLOCATOR );

		T *const p = ( T * )DOLL__DEFAULT_ALLOCATOR->alloc( sizeof( T ), T::kTag, pszFile, line, pszFunc );
		if( !p ) {
			return nullptr;
		}

		construct( *p );
		return p;
	}
	template< typename T >
	T *dealloc( T *p, const char *pszFile, U32 line = 0, const char *pszFunc = nullptr )
	{
		AX_ASSERT_NOT_NULL( DOLL__DEFAULT_ALLOCATOR );

		if( !p ) {
			return nullptr;
		}

		p->~T();
		DOLL__DEFAULT_ALLOCATOR->dealloc( ( void * )p, pszFile, line, pszFunc );

		return nullptr;
	}

}

#define DOLL_ALLOC( allocator, size, tag )\
	( ( allocator ).alloc( ( size ), ( tag ), __FILE__, __LINE__, AX_FUNCTION ) )
#define DOLL_DEALLOC( allocator, p )\
	( ( allocator ).dealloc( reinterpret_cast< void * >( p ), __FILE__, __LINE__, AX_FUNCTION ) )

#define DOLL_NEW(T_)    doll::alloc< T_ >( __FILE__, __LINE__, AX_FUNCTION )
#define DOLL_DELETE(P_) doll::dealloc( (P_), __FILE__, __LINE__, AX_FUNCTION )
