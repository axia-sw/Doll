#pragma once

#include "../Core/Defs.hpp"

namespace doll {

/*
================================================================================

	INDEX MAP

	Inspired by id Software's "HashIndex" class. This serves as an auxilliary
	acceleration structure for finding objects from a given key.

	The key is expected to be a 32-bit hash value. This can be generated from a
	basic hashing function.

	The value is intended to be a 32-bit index (into a flat array where the
	objects of interest are either stored directly or stored as pointers).

	There are only two heap allocations used by this structure. One is for the
	hash table, which is used to lookup the first index in a chain for a given
	hash value. The other heap allocation is for the index chain table, which
	points to the next index in a chain from the given hash. (This deals with
	the fact that hash collisions are expected to be very frequent.)

================================================================================
*/

typedef UInt32 HashInt;
typedef UInt32 IndexInt;

static constexpr IndexInt kInvalidIndex = ~IndexInt(0);

static constexpr UInt32 kDefaultHashTableSize = 1024;
static constexpr UInt32 kDefaultLinkTableSize = 1024;
static constexpr UInt32 kDefaultLinkTableGranularity = 512;

class IndexMap {
	IndexInt *m_hashTable;  // Never null; when not allocated, points to g_invalidTable
	IndexInt *m_linkTable;  // Never null; when not allocated, points to g_invalidTable
	UInt32    m_cHashTable; // Must be a power of two
	UInt32    m_cLinkTable;
	IndexInt  m_indexMask;  // Set to either 0 or ~IndexInt(0) (all bits on)
	UInt32    m_cItems;

	static DOLL_DLLFUNC IndexInt g_invalidTable[1];

public:
	// Constructor -- Does not allocate any memory
	DOLL_DLLFUNC IndexMap( UInt32 hashTableSize = kDefaultHashTableSize, UInt32 linkTableSize = kDefaultLinkTableSize );
	// Destructor
	DOLL_DLLFUNC ~IndexMap();

	// Clear the tables without deallocating memory
	Void clear();
	// Clear the tables, deallocating associated memory
	DOLL_DLLFUNC Void purge();

	// Add an index to the table with the assumption that this is the first time
	// this index is being added to the table
	Void add( HashInt key, IndexInt index );
	// Remove an index from the table
	Void remove( HashInt key, IndexInt index );

	// Insert an index into the table, causing all indexes >= it to be
	// incremented by one
	DOLL_DLLFUNC Void insertIndex( HashInt key, IndexInt index );
	// Remove an index from the table, causing all indexes >= it to be
	// decremented by one
	DOLL_DLLFUNC Void removeIndex( HashInt key, IndexInt index );

	// Check if this is empty
	Bool isEmpty() const {
		return m_cItems == 0;
	}
	// Check if this is used (not empty)
	Bool isUsed() const {
		return m_cItems != 0;
	}

	// Receive the size of the hash table
	SizeType getHashTableSize() const {
		return SizeType( m_cHashTable );
	}
	// Receive the size of the link table
	SizeType getLinkTableSize() const {
		return SizeType( m_cLinkTable );
	}

	// Retrieve the number of items stored
	SizeType size() const {
		return SizeType( m_cItems );
	}

	// Retrieve the first index from a hash or kInvalidIndex if there is none
	IndexInt head( HashInt hash ) const {
		// Note: m_cHashTable is never 0 and m_hashTable is never null
		//
		// When we are not allocated, m_indexMask is set to 0 and m_linkTable
		// will point to g_invalidTable, resulting in an invalid index being
		// returned.
		const SizeType h = SizeType( hash & ( m_cHashTable - 1 ) & m_indexMask );
		return m_hashTable[ h ];
	}
	// Retrieve the next item from a hash or kInvalidIndex if there is none
	IndexInt next( IndexInt prevIndex ) const {
		AX_ASSERT( prevIndex < m_cLinkTable );

		// Note: m_cLinkTable is never 0 and m_linkTable is never null
		//
		// When we are not allocated, m_indexMask is set to 0 and m_linkTable
		// will point to g_invalidTable, resulting in an invalid index being
		// returned.
		const SizeType i = SizeType( prevIndex & m_indexMask );
		return m_linkTable[ i ];
	}

	DOLL_DLLFUNC Bool tryEnsureIndexAllocation( IndexInt maxIndex );
	DOLL_DLLFUNC Void ensureIndexAllocation( IndexInt maxIndex );

private:
	DOLL_DLLFUNC Bool tryAllocateHashTable( SizeType hashTableSize );
	DOLL_DLLFUNC Bool tryAllocateLinkTable( SizeType linkTableSize );
};

inline Void IndexMap::clear() {
	if( m_hashTable != g_invalidTable ) {
		memset( ( Void * )m_hashTable, 0xFF, m_cHashTable*sizeof(IndexInt) );
	}

	// NOTE: The link table does not need to be cleared since the heads of the
	//       chain are already reset by clearing the hash table above.

	m_cItems = 0;
}

inline Void IndexMap::add( HashInt key, IndexInt index ) {
	ensureIndexAllocation( index );

	AX_ASSERT( m_hashTable != g_invalidTable );

	// Determine the hash table index from the key
	const SizeType h = SizeType( key & ( m_cHashTable - 1 ) );

	// The next link in the hash chain is whatever was previously at the head
	m_linkTable[ index ] = m_hashTable[ h ];
	// The new head of the hash chain is the index we just added
	m_hashTable[ h ] = index;

	// We've added an item
	++m_cItems;
}
inline Void IndexMap::remove( HashInt key, IndexInt index ) {
	if( m_hashTable == g_invalidTable ) {
		return;
	}

	// Determine the hash table index from the key
	const SizeType h = SizeType( key & ( m_cHashTable - 1 ) );

	// Check each index
	for( IndexInt i = m_hashTable[ h ]; m_linkTable[ i ] != kInvalidIndex; i = m_linkTable[ i ] ) {
		// Ignore any index we're not looking for
		if( i != index ) {
			continue;
		}

		// Link the item that pointed to us to the index after us, removing us
		// from the chain
		if( m_hashTable[ h ] == index ) {
			m_hashTable[ h ] = m_linkTable[ index ];
		} else {
			m_linkTable[ i ] = m_linkTable[ index ];
		}

		// Set our own link to be invalid
		m_linkTable[ index ] = kInvalidIndex;

		// An item has been removed
		--m_cItems;

		// Done
		return;
	}
}

}
