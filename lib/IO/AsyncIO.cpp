#include "doll/IO/AsyncIO.hpp"

#include "doll/Core/Engine.hpp"
#include "doll/Core/Logger.hpp"

// FIXME: Finish implementing this file.
//        See comments in the header.

namespace doll
{

	class CAsyncOp: public TPoolObject< CAsyncOp, kTag_FileSys >
	{
	public:
		typedef TIntrLink<CAsyncOp> Link;

		CAsyncOp( IFile &file, const Str &name, Void *pDst, UPtr cBytes, IAsyncRead *pAsyncRead )
		: m_file( file )
		, m_name( name )
		, m_pDst( pDst )
		, m_cReqBytes( cBytes )
		, m_cGotBytes( 0 )
		, m_pAsyncRead( pAsyncRead )
		, m_status( EAsyncStatus::Pending )
		, m_cRefs( 1 )
		, m_iPriority( 0 )
		, m_config()
		, m_bNeedConfig( true )
		, m_bOwnsDst( false )
		, m_uNextTryMilliseconds( 0 )
		, m_cTries( 0 )
		, m_siblings( this )
		{
			const U64 cFileBytes = m_file.size();

			if( !cBytes ) {
				AX_ASSERT( pDst == nullptr );

				if( cFileBytes > 0x7FFFFFFF ) {
					m_cReqBytes = 0x7FFFFFFF;
				} else {
					m_cReqBytes = UPtr( cFileBytes );
				}

				if( !m_cReqBytes ) {
					m_cReqBytes = 1;
				}

				m_bOwnsDst = true;
				m_pDst = DOLL_ALLOC( *DOLL__HEAP_ALLOCATOR, m_cReqBytes, kTag_FileSys );
				if( !AX_VERIFY_MEMORY( m_pDst ) ) {
					return;
				}

				( ( U8 * )m_pDst )[ m_cReqBytes - 1 ] = 0;
			} else {
				const U64 uFilePos = m_file.tell();
				if( uFilePos + m_cReqBytes > cFileBytes && uFilePos <= cFileBytes ) {
					m_cReqBytes = UPtr( cFileBytes - uFilePos );
				}
			}

			m_file.grab();
		}

		inline Void grab()
		{
			++m_cRefs;
		}
		inline NullPtr drop()
		{
			if( --m_cRefs != 0 ) {
				return nullptr;
			}

			g_core.io.trashedOpsLock.acquire();
			g_core.io.trashedOps.addTail( m_siblings );
			g_core.io.trashedOpsLock.release();

			return nullptr;
		}

		inline Str getName() const
		{
			return m_name;
		}
		inline UPtr getSize() const
		{
			return m_cReqBytes;
		}
		inline UPtr getPos() const
		{
			return m_cGotBytes;
		}
		inline const Void *getPtr() const
		{
			return m_pDst;
		}
		inline EAsyncStatus getStatus() const
		{
			return m_status;
		}

		inline Void cancel()
		{
			if( m_status != EAsyncStatus::Success ) {
				m_status = EAsyncStatus::Aborted;
			}
		}

		// Get rid of (some of) the collected trash thus far
		static inline Void clearTrash()
		{
			AX_PUSH_DISABLE_WARNING_MSVC(6385)

			CAsyncOp *pOps[ kMaxAsyncOps ];
			UPtr      cOps = 0;

			// Don't care if this might be wrong since deleting isn't urgent
			if( g_core.io.trashedOps.isEmpty() ) {
				return;
			}
			
			// Pull the ops out from the global list and into a local array
			g_core.io.trashedOpsLock.acquire();
			while( cOps < arraySize( pOps ) ) {
				CAsyncOp *const p = g_core.io.trashedOps.head();
				if( !p ) {
					break;
				}

				p->m_siblings.unlink();
				pOps[ cOps++ ] = p;
			}
			g_core.io.trashedOpsLock.release();

			// Delete from the local array so we're not holding the global lock
			// for too long
			while( cOps > 0 ) {
				delete pOps[ --cOps ];
			}

			AX_POP_DISABLE_WARNING_MSVC()
		}
		// Get rid of all the collected trash (used upon termination)
		static inline Void purgeTrash()
		{
			g_core.io.trashedOpsLock.acquire();
			while( g_core.io.trashedOps.isUsed() ) {
				delete g_core.io.trashedOps.head();
			}
			g_core.io.trashedOpsLock.release();
		}

		// Check if we need a new configuration, then update if so
		inline Void updateConfig()
		{
			if( !m_bNeedConfig ) {
				return;
			}

			SAsyncReadConf conf;
			if( !m_pAsyncRead || !m_pAsyncRead->io_config( conf ) ) {
				conf.cReqBytes    = 0;
				conf.cMaxBytes    = 0;
				conf.uWantFrameId = 0;
				conf.uNeedFrameId = 0;
			}

			setConfig( conf );
			m_bNeedConfig = false;
		}
		// Recalculate our priority
		inline Void calcPriority( U32 uFrameId )
		{
			const S32 iDistNeed = m_config.uNeedFrameId != 0 ? m_config.uNeedFrameId - uFrameId : 60;
			const S32 iDistWant = m_config.uWantFrameId != 0 ? m_config.uWantFrameId - uFrameId : iDistNeed;

			if( iDistNeed <= 100 ) {
				m_iPriority = ( 100 - iDistNeed )*100;
			} else {
				m_iPriority = 0;
			}
			if( iDistWant <= 100 ) {
				m_iPriority += 100 - iDistWant;
			}
		}
		// Compare to another op for priority sorting
		//
		// Larger priorities should come first in the list
		inline S32 sortCmp( const CAsyncOp &other ) const
		{
			return other.m_iPriority - m_iPriority;
		}

		// Read from the file
		//
		// Returns false if this should be removed from the pending queue
		inline Bool read()
		{
			// FIXME: Shouldn't this be an AX_ASSERT() instead?
			if( m_status != EAsyncStatus::Pending ) {
				return false;
			}

			// We might be in a retry time-out period
			if( m_uNextTryMilliseconds != 0 ) {
				// If the next time we can retry is in the future then skip
				if( m_uNextTryMilliseconds > U32( milliseconds_lowLatency() ) ) {
					return true;
				}
			}

			const UPtr uAlignReq = m_file.getAlignReqs();
			const UPtr cReqBytesY = uAlignReq > 1 ? align( m_config.cReqBytes, uAlignReq ) : m_config.cReqBytes;
			const UPtr cReqBytesX = cReqBytesY < m_config.cMaxBytes ? cReqBytesY : m_config.cMaxBytes;
			const UPtr cReqBytes = m_cGotBytes + cReqBytesX < m_cReqBytes ? cReqBytesX : m_cReqBytes - m_cGotBytes;

			const UPtr cGotBytes = m_file.read( ( Void * )( UPtr( m_pDst ) + m_cGotBytes ), cReqBytes );
			m_cGotBytes += cGotBytes;
			if( !cGotBytes ) {
				if( m_file.isEnd() || m_cGotBytes == m_cReqBytes ) {
					// Fill the rest of the buffer with zeros
					if( m_cGotBytes < m_cReqBytes ) {
						memset( ( Void * )( UPtr( m_pDst ) + m_cGotBytes ), 0, m_cReqBytes - m_cGotBytes );
					}

					m_status = EAsyncStatus::Success;
				} else {
					char szBuf[128];
					if( m_cTries++ < g_core.io.cRetries ) {
						g_DebugLog(m_name) += (axspf(szBuf,"%p: retry %u",this,m_cTries),szBuf);
						m_uNextTryMilliseconds = U32( milliseconds_lowLatency() ) + 25*m_cTries*m_cTries;
						return true;
					}

					g_WarningLog(m_name) += (axspf(szBuf,"Async op %p failed.",this),szBuf);
					m_status = EAsyncStatus::Failure;
				}

				if( m_pAsyncRead != nullptr ) {
					m_pAsyncRead->io_notify( cGotBytes, m_status );
				}
				return false;
			}

			if( m_pAsyncRead != nullptr ) {
				m_pAsyncRead->io_notify( cGotBytes, m_status );
			}
			m_bNeedConfig = true;
			return true;
		}

	private:
		IFile &        m_file;
		MutStr         m_name;
		Void *         m_pDst;
		UPtr           m_cReqBytes;
		UPtr           m_cGotBytes;
		IAsyncRead *   m_pAsyncRead;
		EAsyncStatus   m_status;
		UPtr           m_cRefs;
		S32            m_iPriority;
		SAsyncReadConf m_config;
		Bool           m_bNeedConfig;
		Bool           m_bOwnsDst;
		U32            m_uNextTryMilliseconds;
		U32            m_cTries;

		inline ~CAsyncOp()
		{
			if( m_bOwnsDst ) {
				DOLL_DEALLOC( *DOLL__HEAP_ALLOCATOR, m_pDst );
			}

			m_file.drop();
		}

		// Accept a configuration and calculate a priority
		inline Void setConfig( const SAsyncReadConf &conf )
		{
			char szBuf[ 128 ];

			const UPtr uAlignReq = m_file.getAlignReqs();
			if( uAlignReq > 1 ) {
				AX_ASSERT( conf.cMaxBytes >= uAlignReq );
			}

			m_config.cReqBytes = conf.cReqBytes;
			m_config.cMaxBytes = conf.cMaxBytes;
			if( conf.cReqBytes > conf.cMaxBytes ) {
				m_config.cReqBytes = conf.cMaxBytes;

				g_WarningLog( m_name ) += ( axspf( szBuf, "[async] `cReqBytes > cMaxBytes` (%zu > %zu)", conf.cReqBytes, conf.cMaxBytes ), szBuf );
			}

			if( !m_config.cMaxBytes ) {
				m_config.cMaxBytes = m_cReqBytes - m_cGotBytes;
			}
			if( !m_config.cReqBytes ) {
				static const UPtr kMaxBytes = 0x80000; // 512KB (around 2.7~ seconds of 48kHz stereo audio)
				m_config.cReqBytes = m_config.cMaxBytes > kMaxBytes ? kMaxBytes : m_config.cMaxBytes;
			}

			m_config.uWantFrameId = conf.uWantFrameId ? conf.uWantFrameId : 100;
			m_config.uNeedFrameId = conf.uNeedFrameId ? conf.uNeedFrameId : 100;
			if( conf.uNeedFrameId != 0 && conf.uWantFrameId > conf.uNeedFrameId ) {
				m_config.uWantFrameId = conf.uNeedFrameId;

				g_WarningLog( m_name ) += ( axspf( szBuf, "[async] `uWantFrameId > uNeedFrameId` (%u > %u)", conf.uWantFrameId, conf.uNeedFrameId ), szBuf );
			}
		}

	public:
		Link         m_siblings;
	};

	// ---------------------------------------------------------------------- //

	static Bool async__enqueue( CAsyncOp *pOp )
	{
		AX_ASSERT_NOT_NULL( pOp );

		g_core.io.transferOpsLock.acquire();
		g_core.io.transferOps.addTail( pOp->m_siblings );
		g_core.io.transferOpsLock.release();

		axth_sem_signal( &g_core.io.worksem );
		return true;
	}
	static Bool async__dequeue( TIntrList<CAsyncOp> &dstList )
	{
		g_core.io.transferOpsLock.acquire();
		do {
			CAsyncOp *const pOp = g_core.io.transferOps.head();
			if( !pOp ) {
				return false;
			}

			dstList.addTail( pOp->m_siblings );
		} while( axth_sem_timed_wait( &g_core.io.worksem, 0 ) );
		g_core.io.transferOpsLock.release();

		return true;
	}

	static Bool sem_predicated_wait( Bool bIsTimed, axth_sem_t &sem )
	{
		return bIsTimed ? !!axth_sem_timed_wait( &sem, 0 ) : ( axth_sem_wait( &sem ), true );
	}
	static int asyncSortCmp( const CAsyncOp &a, const CAsyncOp &b )
	{
		return a.sortCmp( b );
	}

	static int AXTHREAD_CALL async__thread_f( axthread_t *, Void * )
	{
		// List of pending operations
		TIntrList<CAsyncOp> pendingOps;
		// Current working frame id
		U32 frameId = ~0U;

		// Main loop
		for(;;) {
			// Check for new work
			if( sem_predicated_wait( pendingOps.isUsed(), g_core.io.worksem ) ) {
				// Retrieve the pending operations
				if( !async__dequeue( pendingOps ) ) {
					// Break, so that pending ops will be cancelled
					break;
				}

				// This might be a signal to quit instead of work
				if( axthread_is_quitting( &g_core.io.thread ) ) {
					// Break, don't return, so that pending ops will be handled
					break;
				}
			}

			// Grab the current frame id
			const U32 newFrameId = AX_ATOMIC_COMPARE_EXCHANGE_REL32( &g_core.io.frameId, 0, 0 );

			// Apply frame change updates
			if( frameId != newFrameId ) {
				frameId = newFrameId;

				// Enumerate each operation
				for( CAsyncOp *pOp = pendingOps.head(); pOp != nullptr; pOp = pOp->m_siblings.next() ) {
					// Grab new configurations if necessary
					pOp->updateConfig();

					// Update its priority
					pOp->calcPriority( frameId );
				}

				// Sort the list of operations again
				pendingOps.sort( &asyncSortCmp );
			}

			// First available pending operation
			CAsyncOp *const pPendingOp = pendingOps.head();
			
			// Read from the pending operation if it's available, then push it
			// to the end of the list (round-robin scheduling within a frame)
			if( pPendingOp != nullptr ) {
				if( pPendingOp->read() ) {
					// Successful read -- add to end of queue
					pendingOps.addTail( pPendingOp->m_siblings );
				} else {
					// Operation finished or was aborted -- remove from queue
					pPendingOp->m_siblings.unlink();
					pPendingOp->drop();
				}
			}
		}

		// Cancel all the active pending operations
		{
			CAsyncOp *pNext;
			for( CAsyncOp *p = pendingOps.head(); p != nullptr; p = pNext ) {
				pNext = p->m_siblings.next();

				p->cancel();
				p->drop();
			}
		}

		return EXIT_SUCCESS;
	}

	// ---------------------------------------------------------------------- //

	DOLL_FUNC Bool DOLL_API async_init()
	{
		AX_ASSERT( axthread_is_running( &g_core.io.thread ) == false );

		if( !axth_sem_init( &g_core.io.worksem, 0 ) ) {
			DOLL_ERROR_LOG += "Could not create IO semaphore.";
			return false;
		}

		if( !axthread_init( &g_core.io.thread, &async__thread_f, nullptr ) ) {
			DOLL_ERROR_LOG += "Could not create IO thread.";
			return false;
		}

		axthread_set_name( &g_core.io.thread, "[Doll] Async IO" );
		axthread_set_priority( &g_core.io.thread, kAxthread_Priority_VeryHigh );

		return true;
	}
	DOLL_FUNC Void DOLL_API async_fini()
	{
		axthread_signal_quit( &g_core.io.thread );
		axth_sem_signal( &g_core.io.worksem );

		axthread_fini( &g_core.io.thread );
		axth_sem_fini( &g_core.io.worksem );

		CAsyncOp::purgeTrash();
	}

	DOLL_FUNC Void DOLL_API async_step()
	{
		for( UPtr i = g_core.io.engineOps.num(); i > 0; --i ) {
			CAsyncOp *const pOp = g_core.io.engineOps[ i - 1 ];
			AX_ASSERT_NOT_NULL( pOp );

			if( async_status( pOp ) == EAsyncStatus::Pending ) {
				continue;
			}

			g_core.io.engineOps.remove( i );
			async_close( pOp );
		}

		CAsyncOp::purgeTrash();
		atomicInc( &g_core.io.frameId, ax::Mem::Relaxed() );
	}

	DOLL_FUNC CAsyncOp *DOLL_API async_readFile( IFile *pFile, const Str &name, Void *pDst, UPtr cBytes, IAsyncRead *pAsyncRead )
	{
		AX_ASSERT_NOT_NULL( pFile );

		CAsyncOp *const pOp = new CAsyncOp( *pFile, name, pDst, cBytes, pAsyncRead );
		if( !AX_VERIFY_MEMORY( pOp ) || !pOp->getPtr() ) {
			return nullptr;
		}

		async__enqueue( pOp );
		return pOp;
	}
	DOLL_FUNC NullPtr DOLL_API async_close( CAsyncOp *pAsyncOp )
	{
		if( pAsyncOp != nullptr ) {
			pAsyncOp->cancel();
			pAsyncOp->drop();
		}

		return nullptr;
	}

	DOLL_FUNC Bool DOLL_API async_getName( const CAsyncOp *pAsyncOp, Str &dstName )
	{
		AX_ASSERT_NOT_NULL( pAsyncOp );
		
		dstName = pAsyncOp->getName();
		return true;
	}

	DOLL_FUNC UPtr DOLL_API async_size( const CAsyncOp *pAsyncOp )
	{
		AX_ASSERT_NOT_NULL( pAsyncOp );
		return pAsyncOp->getSize();
	}
	DOLL_FUNC UPtr DOLL_API async_tell( const CAsyncOp *pAsyncOp )
	{
		AX_ASSERT_NOT_NULL( pAsyncOp );
		return pAsyncOp->getPos();
	}

	DOLL_FUNC const Void *DOLL_API async_getPtr( const CAsyncOp *pAsyncOp )
	{
		AX_ASSERT_NOT_NULL( pAsyncOp );
		return pAsyncOp->getPtr();
	}
	DOLL_FUNC EAsyncStatus DOLL_API async_status( const CAsyncOp *pAsyncOp )
	{
		AX_ASSERT_NOT_NULL( pAsyncOp );
		return pAsyncOp->getStatus();
	}

	DOLL_FUNC UPtr DOLL_API async_enumPending( const CAsyncOp **ppDstOps, UPtr cMaxDstOps )
	{
		( Void )ppDstOps;
		( Void )cMaxDstOps;
		AX_ASSERT_MSG( false, "Unimplemented" );
		return 0;
	}

}
