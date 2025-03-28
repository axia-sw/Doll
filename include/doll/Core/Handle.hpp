#pragma once

#include "../AxlibConfig.h"

#include "Private/Variants.hpp"
#include "Version.hpp"

// Systems implementing handles should define these to the appropriate runtime
// error routines for their subsystem.

#ifndef DOLL__RUNTIME_ERROR_INVALID_HANDLE
# define DOLL__RUNTIME_ERROR_INVALID_HANDLE() ((void)0)
#endif
#ifndef DOLL__RUNTIME_ERROR_DEAD_HANDLE
# define DOLL__RUNTIME_ERROR_DEAD_HANDLE() ((void)0)
#endif
#ifndef DOLL__RUNTIME_ERROR_REUSED_HANDLE
# define DOLL__RUNTIME_ERROR_REUSED_HANDLE() ((void)0)
#endif
#ifndef DOLL__RUNTIME_ERROR_NULL_HANDLE
# define DOLL__RUNTIME_ERROR_NULL_HANDLE() ((void)0)
#endif

namespace doll {

namespace internal {

template<unsigned tSize>
struct THandleImpl {
};

template<>
struct THandleImpl<4> {
	unsigned uIndex      : 28;
	unsigned uGeneration :  4;

	static const unsigned kGenMask = unsigned(0x0F);
};
template<>
struct THandleImpl<8> {
	unsigned uIndex      : 32;
	unsigned uGeneration : 32;

	static const unsigned kGenMask = unsigned(0xFFFFFFFF);
};

static_assert( sizeof(THandleImpl<4>) == 4, "Handle implementation for 32-bit is invalid." );
static_assert( sizeof(THandleImpl<8>) == 8, "Handle implementation for 64-bit is invalid." );

using HandleImpl = THandleImpl<sizeof(void*)>;

struct HandleObject {
	void *pObject = nullptr;
	unsigned uAge = 0;
};

struct HandleValidation_None {
	static const HandleObject &validate(const HandleObject *arr, unsigned len, HandleImpl h) {
		((void)len);
		return arr[ h.uIndex - 1 ];
	}
};

struct HandleValidation_Full {
	static const HandleObject &validate(const HandleObject *arr, unsigned len, HandleImpl h) {
		static const HandleObject dummy;

		if( h.uIndex == 0 ) {
			DOLL__RUNTIME_ERROR_NULL_HANDLE();
			return dummy;
		}

		if( h.uIndex > len ) {
			DOLL__RUNTIME_ERROR_INVALID_HANDLE();
			return dummy;
		}

		const HandleObject &obj = arr[ h.uIndex - 1 ];
		if( obj.pObject == nullptr ) {
			DOLL__RUNTIME_ERROR_DEAD_HANDLE();
			return dummy;
		}

		// ( ( obj.uAge ^ h.uGeneration ) & HandleImpl::kGenMask ) != 0
		if( ( obj.uAge & HandleImpl::kGenMask ) != h.uGeneration ) {
			DOLL__RUNTIME_ERROR_REUSED_HANDLE();
			return dummy;
		}

		return obj;
	}
};

template<EVariant tVariant>
struct THandleValidation: public HandleValidation_Full {};

template<>
struct THandleValidation<EVariant::release>: public HandleValidation_None {};

using HandleValidation = THandleValidation<kVariant>;

static inline void *handleToObject( const TArr<HandleObject> &arr, HandleImpl h ) {
	return HandleValidation::validate( arr.pointer(), arr.len(), h ).pObject;
}

} // namespace doll::internal

template<typename T>
class THandlePool {
public:
	struct Handle: public internal::HandleImpl {
		using ObjectType = T;

		Handle() {
			uIndex      = 0;
			uGeneration = 0;
		}
		Handle( decltype(nullptr) ): Handle() {}
		Handle( const Handle &h ) {
			uIndex      = h.uIndex;
			uGeneration = h.uGeneration;
		}
		~Handle() = default;

		Handle &operator=( const Handle &h ) = default;

		bool operator!() const { return uIndex==0; }
		operator bool() const { return uIndex!=0; }

		bool operator==( decltype(nullptr) ) const { return uIndex==0; }
		bool operator!=( decltype(nullptr) ) const { return uIndex!=0; }

		bool operator==( Handle h ) const { return uIndex==h.uIndex && uGeneration==h.uGeneration; }
		bool operator!=( Handle h ) const { return uIndex!=h.uIndex || uGeneration!=h.uGeneration; }
	};

	Handle add( T *pObject ) {
		AX_ASSERT_NOT_NULL( pObject );

		unsigned index;
		if( nextFree(index) ) {
			mObjects[ index ].pObject = (void *)pObject;

			Handle h;

			h.uIndex      = index + 1;
			h.uGeneration = mObjects[ index ].uAge;

			return h;
		}

		if( !AX_VERIFY_MSG( mObjects.append(), "Failed to create handle." ) ) {
			return nullptr;
		}

		internal::HandleObject &obj = mObjects.last();

		obj.pObject = (void *)pObject;
		obj.uAge    = ( rand() % Handle::kGenMask ) + rand() % 2;

		Handle h;

		h.uIndex      = mObjects.len();
		h.uGeneration = obj.uAge;

		return h;
	}
	T *remove( Handle h ) {
		if( !h ) {
			return nullptr;
		}

		T *const pObj = (T *)internal::handleToObject( mObjects, h );

		const unsigned index = h.uIndex - 1;
		mObjects[ index ].pObject = nullptr;
		mObjects[ index ].uAge += 1;

		pushFree( index );

		return pObj;
	}

	T *get( Handle h ) const {
		return (T *)internal::handleToObject( mObjects, h );
	}

private:
	struct FreeRange {
		unsigned start;
		unsigned count;
	};

	TMutArr<internal::HandleObject> mObjects;
	TMutArr<FreeRange>              mFree;

	inline bool nextFree( unsigned &dstIndex ) {
		if( mFree.isEmpty() ) {
			return false;
		}

		bool isZero = false;
		{
			FreeRange &range = mFree.last();
			AX_ASSERT( range.count > 0 );

			range.count -= 1;
			dstIndex = range.start + range.count;

			isZero = range.count == 0;
		}

		if( isZero ) {
			mFree.removeLast();
		}

		return true;
	}
	inline void pushFree( unsigned index ) {
		AX_ASSERT( index != ~unsigned(0) );

		if( mFree.isUsed() ) {
			const unsigned max_c = 3;
			const unsigned n     = mFree.len();

			for( unsigned c = 1; c <= n && c <= max_c; ++c ) {
				const unsigned i = n - c;
				FreeRange &range = mFree[ i ];

				bool addedIndex = false;
				if( index == range.start - 1 ) {
					range.start -= 1;
					addedIndex = true;
				} else if( index == range.start + range.count ) {
					range.count += 1;
					addedIndex = true;
				}

				if( addedIndex ) {
					(void)attemptMerge( i, i - 1 );
					(void)attemptMerge( i, i + 1 );
					return;
				}
			}
		}

		mFree.append( FreeRange {
			.start = index,
			.count = 1
		} );
	}
	inline bool attemptMerge( unsigned i, unsigned j ) {
		if( i >= mFree.len() || j >= mFree.len() ) {
			return false;
		}

		AX_ASSERT( i != j );
		AX_ASSERT( i < j ? j - i == 1 : i - j == 1 );

		FreeRange &a = mFree[ i ];
		FreeRange &b = mFree[ j ];

		if( a.start + a.count == b.start ) {
			a.count += b.count;
			mFree.remove( j );
			return true;
		}

		if( b.start + b.count == a.start ) {
			b.count += a.count;
			mFree.remove( i );
			return true;
		}

		return false;
	}
};

/*

	// Example of usage:

	using RTexturePool = THandlePool<RTexture>;
	using HTexture     = RTexturePool::Handle;

	HTexture gfx_makeWhiteTexture() {
		return g_texturePool.add( new RTexture(ESolidColor::White) );
	}
	void gfx_killTexture( HTexture texture ) {
		RTexture *const pTexture = g_texturePool.remove( texture );
		// pTexture can be null here, but delete accepts nullptr
		delete pTexture;
	}

	int gfx_getTextureWidth( HTexture texture ) {
		const RTexture *const pTexture = g_texturePool.get(texture);
		return pTexture != nullptr ? pTexture->iRes.x : 0;
	}
	int gfx_getTextureHeight( HTexture texture ) {
		const RTexture *const pTexture = g_texturePool.get(texture);
		return pTexture != nullptr ? pTexture->iRes.y : 0;
	}

*/

} // namespace doll
