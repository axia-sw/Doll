#pragma once

#include "../Core/Defs.hpp"
#include "VFS.hpp"

namespace doll
{

	class CAsyncOp;

	enum class EAsyncStatus: S32
	{
		Aborted = -1,
		Failure =  0,

		Pending,
		Success
	};

	// Scheduling configuration for asynchronous reads.
	//
	// This information is used to determine the urgency of the next read
	// operation. For example, streaming music can make extensive use of this
	// structure by checking how much time is left until the end of the music it
	// had buffered and determining how much more should be buffered. The IO
	// scheduler can then determine if there are any higher priority operations
	// coming up (such as streaming in level data that suddenly came into view)
	// and select which operation to perform first.
	//
	// This structure is requested by the IO thread in a call to
	// `IAsyncRead::io_config()`. An `IAsyncRead` interface is implemented by
	// the user and can optionally be passed to certain asynchronous routines,
	// such as `async_readFile()`.
	struct SAsyncReadConf
	{
		// Number of bytes requested for the next read operation. (0 means any.)
		UPtr cReqBytes;
		// Maximum number of bytes that are allowed to be read. (0 means
		// unlimited.)
		//
		// If unbuffered IO is used then this must be at least equal to the
		// alignment requirements of the file. (See `IFile::getAlignReqs()`.)
		//
		// If `cReqBytes > cMaxBytes` then a warning may be issued.
		UPtr cMaxBytes;
		// Which frame number we would like the operation to be completed by. (0
		// means "no preference.")
		U32  uWantFrameId;
		// Which frame number we *need* the operation completed by. (0 means "no
		// preference.")
		//
		// If `uNeedFrameId != 0 && uWantFrameId > uNeedFrameId` then a warning
		// may be issued.
		U32  uNeedFrameId;
	};

	// User-defined interface that handles notifications of data reads (for
	// example, to update an UI element, or activate a decompression job in
	// another thread).
	class IAsyncRead
	{
	public:
		// IO thread calls this to determine the priority of its next read.
		//
		// dst: holds configuration data if return value is true.
		// return: `true` if `dst` has been completely configured; `false`
		//         otherwise, which indicates that a default configuration
		//         should be used.
		virtual Bool io_config( SAsyncReadConf &dst ) = 0;

		// IO thread calls this after it has written data into the buffer given
		// to the operation.
		//
		// cGotBytes: number of bytes written into the buffer this update.
		// EAsyncStatus: if `EAsyncStatus::Pending` then more data is expected,
		//               otherwise no further data will be requested.
		//
		// NOTE: Keep this function from taking more than a few cycles.
		virtual Void io_notify( UPtr cGotBytes, EAsyncStatus ) = 0;
	};

	// Initialize the IO thread. (Called automatically by `doll_init()`.)
	DOLL_FUNC Bool DOLL_API async_init();
	// Finish using the IO thread, cancelling all pending operations. (Called
	// automatically by `doll_fini()`.)
	DOLL_FUNC Void DOLL_API async_fini();
	// Step the asynchronous IO. (This is called by doll_sync())
	//
	// - Deletes or recycles unused asynchronous operations.
	// - Updates development graphs if open, including information about which
	//   IO operations are happening and how far along they are.
	DOLL_FUNC Void DOLL_API async_step();

	// Creates an asynchronous read operation
	//
	// The read occurs from the current position of the file (fs_tell()) will
	// transfer at most `cBytes` worth of data.
	//
	// name: Name to give the operation. Useful for debug purposes.
	// pDst: Destination buffer. If `nullptr` then a buffer is allocated,
	//       aligned to the granularity specified by the file.
	// cBytes: Number of bytes available in `pDst`. Must be `0` if `pDst` is
	//         `nullptr`.
	//
	// An `IAsyncRead` callback interface can be provided. This interface
	// provides the buffer that will be written to for the read operation, and
	// allows for notification of reads.
	//
	// The `CAsyncOp` object returned can be queried for information about the
	// operation.
	DOLL_FUNC CAsyncOp *DOLL_API async_readFile( IFile *, const Str &name = Str(), Void *pDst = nullptr, UPtr cBytes = 0, IAsyncRead * = nullptr );
	// Destroys an asynchronous operation
	//
	// If the operation is currently active, it will be cancelled.
	DOLL_FUNC NullPtr DOLL_API async_close( CAsyncOp * );
	
	// Retrieve the name of a given async operation
	DOLL_FUNC Bool DOLL_API async_getName( const CAsyncOp *, Str &dstName );
	inline Str DOLL_API async_getName( const CAsyncOp *p )
	{
		Str r;
		return async_getName( p, r ), r;
	}

	// Retrieve the total size of the async operation
	DOLL_FUNC UPtr DOLL_API async_size( const CAsyncOp * );
	// Retrieve the number of bytes transfered for the async operation
	DOLL_FUNC UPtr DOLL_API async_tell( const CAsyncOp * );

	// Retrieve the buffer that is being written into. (Use `async_tell()` to
	// determine how many bytes are available.)
	DOLL_FUNC const Void *DOLL_API async_getPtr( const CAsyncOp * );
	// Get the status of the async operation
	DOLL_FUNC EAsyncStatus DOLL_API async_status( const CAsyncOp * );

	// Enumerate all pending asynchronous operations
	//
	// If `ppDstOps` is `nullptr` then this returns the total number of pending
	// operations that would be returned. (Note, however, that this number may
	// increase or decrease, so it should only be used as a hint.)
	//
	// cMaxDstOps: Capacity of `ppDstOps`. No more than this many will be
	//             written into the array.
	//
	// return: Number of ops written into the array, or if `ppDstOps` is
	//         `nullptr` then the number that would be written into an infinite
	//         capacity array.
	DOLL_FUNC UPtr DOLL_API async_enumPending( const CAsyncOp **ppDstOps, UPtr cMaxDstOps );
	template< UPtr tDstOps >
	inline TArr<const CAsyncOp *> DOLL_API async_enumPending( const CAsyncOp *( &pDstOps )[ tDstOps ] )
	{
		const UPtr cOps = async_enumPending( pDstOps, tDstOps );
		return TArr< const CAsyncOp * >( pDstOps, cOps );
	}

}
