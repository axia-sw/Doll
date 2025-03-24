#include "../BuildSettings.hpp"
#include "doll/Util/IndexMap.hpp"

#include "doll/Core/Memory.hpp"
#include "doll/Math/Bits.hpp"

#include <string.h> // memcpy/memset
#include <stdlib.h> // malloc/free

#define axidxmap_alloc(Size_)  malloc((Size_))
#define axidxmap_dealloc(Mem_) free((Mem_))

namespace doll {

IndexInt IndexMap::g_invalidTable[1] = { kInvalidIndex };

IndexMap::IndexMap( UInt32 hashTableSize, UInt32 linkTableSize )
: m_hashTable( g_invalidTable )
, m_linkTable( g_invalidTable )
, m_cHashTable( hashTableSize )
, m_cLinkTable( linkTableSize )
, m_indexMask( 0 )
, m_cItems( 0 )
{
	AX_ASSERT( bitIsPowerOfTwo(hashTableSize) );
}
IndexMap::~IndexMap() {
	purge();
}

Void IndexMap::purge() {
	if( m_hashTable != g_invalidTable ) {
		axidxmap_dealloc( (Void*)m_hashTable );
		m_hashTable = g_invalidTable;
	}

	if( m_linkTable != g_invalidTable ) {
		axidxmap_dealloc( (Void*)m_linkTable );
		m_linkTable = g_invalidTable;
	}

	m_indexMask = 0;
	m_cItems = 0;
}

Void IndexMap::insertIndex( HashInt key, IndexInt index ) {
	UInt32 i;

	// If this index is larger than the largest index we can hold then just
	// shortcut to a basic add() call
	if( index > m_cLinkTable || m_hashTable == g_invalidTable ) {
		add( key, index );
		return;
	}

	// Make sure we've got space for this index to be stored
	ensureIndexAllocation( index );

	// For every index >= this stored in the hash table, increment by one
	for( i = 0; i < m_cHashTable; ++i ) {
		if( m_hashTable[ i ] < index ) {
			continue;
		}

		m_hashTable[ i ] += 1;
	}

	// For every index >= this stored in the link table, increment by one
	for( i = 0; i < m_cLinkTable; ++i ) {
		if( m_linkTable[ i ] < index ) {
			continue;
		}

		m_linkTable[ i ] += 1;
	}

	// Fix-up the link chain
	for( i = m_cLinkTable; i > index; --i ) {
		m_linkTable[ i ] = m_linkTable[ i - 1 ];
	}
	m_linkTable[ index ] = kInvalidIndex;

	// Add the index
	add( key, index );
}
Void IndexMap::removeIndex( HashInt key, IndexInt index ) {
	IndexInt maxIndex = index;
	UInt32 i;

	// Return immediately if the hash table is empty
	if( m_hashTable == g_invalidTable ) {
		return;
	}

	// Remove the index
	remove( key, index );

	// For every index >= this stored in the hash table, decrement by one
	for( i = 0; i < m_cHashTable; ++i ) {
		if( m_hashTable[ i ] < index ) {
			continue;
		}

		if( maxIndex < m_hashTable[ i ] ) {
			maxIndex = m_hashTable[ i ];
		}

		m_hashTable[ i ] -= 1;
	}

	// For every index >= this stored in the link table, decrement by one
	for( i = 0; i < m_cLinkTable; ++i ) {
		if( m_linkTable[ i ] < index ) {
			continue;
		}

		if( maxIndex < m_linkTable[ i ] ) {
			maxIndex = m_linkTable[ i ];
		}

		m_linkTable[ i ] -= 1;
	}

	// Fix-up the link chain
	for( i = index; i < maxIndex; ++i ) {
		m_linkTable[ i ] = m_linkTable[ i + 1 ];
	}
}

Bool IndexMap::tryEnsureIndexAllocation( IndexInt maxIndex ) {
	const SizeType cIndices = align( maxIndex + 1, kDefaultLinkTableGranularity );

	return
		tryAllocateHashTable( m_cHashTable ) &&
		tryAllocateLinkTable( cIndices );
}
Void IndexMap::ensureIndexAllocation( IndexInt maxIndex ) {
	AX_EXPECT_MEMORY( tryEnsureIndexAllocation( maxIndex ) );
}

Bool IndexMap::tryAllocateHashTable( SizeType hashTableSize ) {
	AX_ASSERT( bitIsPowerOfTwo( hashTableSize ) );
	AX_ASSERT( hashTableSize < kUint32Max/4 );

	if( hashTableSize < m_cHashTable && m_hashTable != g_invalidTable ) {
		return true;
	}

	IndexInt *const hashTable = ( IndexInt * )axidxmap_alloc( hashTableSize*sizeof(IndexInt) );
	if( !hashTable ) {
		return false;
	}

	if( m_hashTable != g_invalidTable ) {
		memcpy( ( Void * )hashTable, ( const Void * )m_hashTable, m_cHashTable*sizeof(IndexInt) );
		memset( ( Void * )( hashTable + m_cHashTable ), 0xFF, ( hashTableSize - m_cHashTable )*sizeof(IndexInt) );

		axidxmap_dealloc( ( Void * )m_hashTable );
	} else {
		memset( ( Void *)hashTable, 0xFF, hashTableSize*sizeof(IndexInt) );
	}

	m_hashTable = hashTable;
	m_cHashTable = static_cast<UInt32>(hashTableSize);

	return true;
}
Bool IndexMap::tryAllocateLinkTable( SizeType linkTableSize ) {
	AX_ASSERT( linkTableSize > 0 );

	if( m_cLinkTable >= linkTableSize && m_linkTable != g_invalidTable ) {
		return true;
	}

	IndexInt *const linkTable = ( IndexInt * )axidxmap_alloc( linkTableSize*sizeof(IndexInt) );
	if( !linkTable ) {
		return false;
	}

	if( m_linkTable != g_invalidTable ) {
		memcpy( ( Void * )linkTable, ( const Void * )m_linkTable, m_cLinkTable*sizeof(IndexInt) );
		memset( ( Void * )( linkTable + m_cLinkTable ), 0xFF, ( linkTableSize - m_cLinkTable )*sizeof(IndexInt) );

		axidxmap_dealloc( (Void *)m_linkTable );
	} else {
		memset( ( Void * )linkTable, 0xFF, linkTableSize*sizeof(IndexInt) );
	}

	m_linkTable = linkTable;
	m_cLinkTable = static_cast<UInt32>(linkTableSize);

	m_indexMask = ~IndexInt(0);

	return true;
}

}
