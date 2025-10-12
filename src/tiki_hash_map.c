#include "tiki_hash_map.h"

#include "tiki_memory.h"

bool tikiHashMapConstructSize( TikiHashMap* hashMap, TikiAllocator* allocator, uintsize entrySize, TikiHashMapEntryHashFunc entryHashFunc, TikiHashMapIsKeyEqualsFunc entryKeyEqualsFunc, uintsize initialSize )
{
	// round up to next power of 2
	initialSize = TIKI_STATIC_MAX( initialSize, 2u );
	initialSize--;
	initialSize |= initialSize >> 1;
	initialSize |= initialSize >> 2;
	initialSize |= initialSize >> 4;
	initialSize |= initialSize >> 8;
	initialSize |= initialSize >> 16;
	initialSize++;

	const uintsize entriesInUseCount = (initialSize + 64u - 1) & (0 - 64);

	hashMap->allocator			= allocator;
	hashMap->entriesInUse		= TIKI_MEMORY_ARRAY_NEW_ZERO( allocator, uint64, entriesInUseCount >> 6u );
	hashMap->entries			= (uint8*)tikiMemoryAlloc( allocator, entrySize * initialSize );
	hashMap->entryCount			= 0u;
	hashMap->entryCapacity		= initialSize;
	hashMap->entrySize			= entrySize;
	hashMap->entryHashFunc		= entryHashFunc;
	hashMap->entryKeyEqualsFunc	= entryKeyEqualsFunc;

	if( !hashMap->entriesInUse || !hashMap->entries )
	{
		tikiHashMapDestruct( hashMap );
		return false;
	}

	return true;
}

bool tikiHashMapConstructStatic( TikiHashMap* hashMap, TikiAllocator* allocator, const void* data, uintsize entrySize, uintsize entryCount, TikiHashMapEntryHashFunc entryHashFunc, TikiHashMapIsKeyEqualsFunc entryKeyEqualsFunc )
{
	if( !tikiHashMapConstructSize( hashMap, allocator, entrySize, entryHashFunc, entryKeyEqualsFunc, entryCount * 2u ) )
	{
		return false;
	}

	const uint8* element = (const uint8*)data;
	for( uint32 i = 0; i < entryCount; ++i )
	{
		if( !tikiHashMapInsert( hashMap, element ) )
		{
			tikiHashMapDestruct( hashMap );
			return false;
		}

		element += entrySize;
	}

	return true;
}

bool tikiHashMapConstructStaticPointer( TikiHashMap* hashMap, TikiAllocator* allocator, const void* data, uintsize entrySize, uintsize entryCount, TikiHashMapEntryHashFunc entryHashFunc, TikiHashMapIsKeyEqualsFunc entryKeyEqualsFunc )
{
	if( !tikiHashMapConstructSize( hashMap, allocator, sizeof( void* ), entryHashFunc, entryKeyEqualsFunc, entryCount * 2) )
	{
		return false;
	}

	const uint8* element = (const uint8*)data;
	for( uint32 i = 0; i < entryCount; ++i )
	{
		if( !tikiHashMapInsert( hashMap, &element ) )
		{
			tikiHashMapDestruct( hashMap );
			return false;
		}

		element += entrySize;
	}

	return true;
}

void tikiHashMapDestruct( TikiHashMap* hashMap )
{
	tikiMemoryFree( hashMap->allocator, hashMap->entriesInUse );
	tikiMemoryFree( hashMap->allocator, hashMap->entries );

	hashMap->entriesInUse		= NULL;
	hashMap->entries			= NULL;
	hashMap->entryCount			= 0;
	hashMap->entryCapacity		= 0;
	hashMap->entrySize			= 0;
	hashMap->entryHashFunc		= NULL;
	hashMap->entryKeyEqualsFunc	= NULL;
	hashMap->allocator			= NULL;
}

static bool tikiHashMapGrow( TikiHashMap* hashMap )
{
	uintsize nextCapacity = hashMap->entryCapacity;
	uint64* newEntriesInUse;
	uint8* newEntries;
	bool retry = false;
	do
	{
		nextCapacity <<= 1u;

		const uint32 nextIndexMask = (uint32)nextCapacity - 1u;
		const uintsize nextInUseCount = TIKI_STATIC_MAX( 1u, nextCapacity >> 6u );

		newEntriesInUse = TIKI_MEMORY_ARRAY_NEW_ZERO( hashMap->allocator, uint64, nextInUseCount );
		newEntries = (uint8*)tikiMemoryAlloc( hashMap->allocator, nextCapacity * hashMap->entrySize );
		if( !newEntriesInUse || !newEntries )
		{
			return false;
		}

		for( uintsize mapIndex = 0; mapIndex < hashMap->entryCapacity; ++mapIndex )
		{
			const uint64* mapEntryInUse = &hashMap->entriesInUse[ mapIndex >> 6u ];
			const uint64 mapEntryInUseMask = 1ull << (mapIndex & 0x3fu);
			if( (*mapEntryInUse & mapEntryInUseMask) == 0u )
			{
				continue;
			}

			const uint8* mapEntry = &hashMap->entries[ mapIndex * hashMap->entrySize ];
			const TikiHash hash = hashMap->entryHashFunc( mapEntry );

			for( uint32 hashOffset = 0; ; ++hashOffset )
			{
				const uint32 newIndex = (hash + hashOffset) & nextIndexMask;

				uint64* newEntryInUse = &newEntriesInUse[ newIndex >> 6u ];
				const uint64 newEntryInUseMask = 1ull << (newIndex & 0x3fu);
				if( (*newEntryInUse & newEntryInUseMask) != 0u )
				{
					const uint8* newEntry = &newEntries[ newIndex * hashMap->entrySize ];
					const TikiHash newHash = hashMap->entryHashFunc( newEntry );
					if( (newHash & nextIndexMask) != (hash & nextIndexMask) )
					{
						retry = true;
						break;
					}
					continue;
				}

				*newEntryInUse |= newEntryInUseMask;

				uint8* newEntry = &newEntries[ newIndex * hashMap->entrySize ];
				memcpy( newEntry, mapEntry, hashMap->entrySize );
				break;
			}

			if( retry )
			{
				tikiMemoryFree( hashMap->allocator, newEntriesInUse );
				tikiMemoryFree( hashMap->allocator, newEntries );
				break;
			}
		}
	}
	while( retry );

	tikiMemoryFree( hashMap->allocator, hashMap->entriesInUse );
	tikiMemoryFree( hashMap->allocator, hashMap->entries );

	hashMap->entriesInUse	= newEntriesInUse;
	hashMap->entries		= newEntries;
	hashMap->entryCapacity	= nextCapacity;

	return true;
}

void* tikiHashMapFind( TikiHashMap* hashMap, const void* entry )
{
	const TikiHash hash		= hashMap->entryHashFunc( entry );
	const uint32 indexMask	= (uint32)hashMap->entryCapacity - 1u;

	for( uint32 hashOffset = 0u; ; ++hashOffset )
	{
		const uintsize index = (hash + hashOffset) & indexMask;

		uint64* mapEntryInUse = &hashMap->entriesInUse[ index >> 6u ];
		const uint64 mapEntryInUseMask = 1ull << (index & 0x3fu);
		if( (*mapEntryInUse & mapEntryInUseMask) == 0u )
		{
			break;
		}

		uint8* mapEntry = &hashMap->entries[ index * hashMap->entrySize ];
		if( hashMap->entryKeyEqualsFunc( mapEntry, entry ) )
		{
			return mapEntry;
		}

		const TikiHash mapHash = hashMap->entryHashFunc( mapEntry );
		if( (mapHash & indexMask) != (hash & indexMask) )
		{
			break;
		}
	}

	return NULL;
}

void* tikiHashMapInsert( TikiHashMap* hashMap, const void* entry )
{
	return tikiHashMapInsertNew( hashMap, entry, NULL );
}

void* tikiHashMapInsertNew( TikiHashMap* hashMap, const void* entry, bool* isNew )
{
	const TikiHash hash		= hashMap->entryHashFunc( entry );
	uint32 indexMask		= (uint32)hashMap->entryCapacity - 1u;

	uint32 hashOffset = 0;
	while( true )
	{
		const uint32 index = (hash + hashOffset) & indexMask;

		uint8* mapEntry = &hashMap->entries[ index * hashMap->entrySize ];

		uint64* mapEntryInUse = &hashMap->entriesInUse[ index >> 6u ];
		const uint64 mapEntryInUseMask = 1ull << (index & 0x3fu);
		if( (*mapEntryInUse & mapEntryInUseMask) == 0u )
		{
			*mapEntryInUse |= mapEntryInUseMask;

			memcpy( mapEntry, entry, hashMap->entrySize );

			hashMap->entryCount++;

			if( isNew )
			{
				*isNew = true;
			}

			return mapEntry;
		}

		if( hashMap->entryKeyEqualsFunc( mapEntry, entry ) )
		{
			if( isNew )
			{
				*isNew = false;
			}

			return mapEntry;
		}

		hashOffset++;

		const TikiHash mapHash = hashMap->entryHashFunc( mapEntry );
		if( (mapHash & indexMask) != (hash & indexMask) )
		{
			if( !tikiHashMapGrow( hashMap ) )
			{
				break;
			}

			hashOffset = 0u;
			indexMask = (uint32)hashMap->entryCapacity - 1u;
		}
	}

	return NULL;
}

bool tikiHashMapRemove( TikiHashMap* hashMap, const void* entry )
{
	const TikiHash hash		= hashMap->entryHashFunc( entry );
	const uint32 indexMask	= (uint32)hashMap->entryCapacity - 1u;

	for( uint32 hashOffset = 0u; ; ++hashOffset )
	{
		const uint32 index = (hash + hashOffset) & indexMask;

		uint64* mapEntryInUse = &hashMap->entriesInUse[ index >> 6u ];
		uint64 mapEntryInUseMask = 1ull << (index & 0x3fu);
		if( (*mapEntryInUse & mapEntryInUseMask) == 0u )
		{
			break;
		}

		uint8* mapEntry = &hashMap->entries[ index * hashMap->entrySize ];
		if( hashMap->entryKeyEqualsFunc( mapEntry, entry ) )
		{
			const uint32 baseIndex = hash & indexMask;

			uint32 nextIndex = index;
			while( true )
			{
				nextIndex = (nextIndex + 1u) & indexMask;

				uint64* nextMapEntryInUse = &hashMap->entriesInUse[ nextIndex >> 6u ];
				const uint64 nextMapEntryInUseMask = 1ull << (nextIndex & 0x3fu);
				if( (*nextMapEntryInUse & nextMapEntryInUseMask) == 0u )
				{
					break;
				}

				uint8* nextMapEntry = &hashMap->entries[ nextIndex * hashMap->entrySize ];
				const TikiHash nextHash = hashMap->entryHashFunc( nextMapEntry );
				if( (nextHash & indexMask) != baseIndex )
				{
					break;
				}

				memcpy( mapEntry, nextMapEntry, hashMap->entrySize );

				mapEntry = nextMapEntry;
				mapEntryInUse = nextMapEntryInUse;
				mapEntryInUseMask = nextMapEntryInUseMask;
			}

			*mapEntryInUse &= ~mapEntryInUseMask;
			hashMap->entryCount--;

			memset( mapEntry, 0, hashMap->entrySize );

			return true;
		}
	}

	return false;
}

uintsize tikiHashMapFindFirstIndex( TikiHashMap* hashMap )
{
	if( hashMap->entryCount == 0u )
	{
		return TIKI_HASH_MAP_INVALID_INDEX;
	}

	const uint64* mapEntryInUse = hashMap->entriesInUse;
	uint64 mapEntryInUseMask = 1u;
	for( uintsize mapIndex = 0u; mapIndex < hashMap->entryCapacity; ++mapIndex )
	{
		if( *mapEntryInUse & mapEntryInUseMask )
		{
			return mapIndex;
		}

		mapEntryInUseMask <<= 1u;
		if( mapEntryInUseMask == 0u )
		{
			mapEntryInUse++;
			mapEntryInUseMask = 1u;
		}
	}

	return TIKI_HASH_MAP_INVALID_INDEX;
}

uintsize tikiHashMapFindNextIndex( TikiHashMap* hashMap, uintsize mapIndex )
{
	++mapIndex;

	const uint64* mapEntryInUse = &hashMap->entriesInUse[ mapIndex >> 6u ];
	uint64 mapEntryInUseMask = 1ull << (mapIndex & 0x3fu);

	for( ; mapIndex < hashMap->entryCapacity; ++mapIndex )
	{
		if( *mapEntryInUse & mapEntryInUseMask )
		{
			return mapIndex;
		}

		mapEntryInUseMask <<= 1u;
		if( mapEntryInUseMask == 0u )
		{
			mapEntryInUse++;
			mapEntryInUseMask = 1u;
		}
	}

	return TIKI_HASH_MAP_INVALID_INDEX;
}

void* tikiHashMapGetEntry( TikiHashMap* hashMap, uintsize index )
{
	return &hashMap->entries[ index * hashMap->entrySize ];
}

TikiHash tikiHashMurmur3( const void* data, size_t dataSize )
{
	return tikiHashMurmur3Seed( data, dataSize, TIKI_HASH_DEFAULT_SEED );
}

TikiHash tikiHashMurmur3Seed( const void* data, size_t dataSize, TikiHash seed )
{
	uint32 hash = seed;
	uint32 dataPart;
	const uint8* bytes = (const uint8*)data;
	for( uintsize i = dataSize >> 2; i; --i )
	{
		dataPart = *(uint32*)bytes;
		bytes += sizeof( uint32 );

		uint32 scramble = dataPart * 0xcc9e2d51;
		scramble = (scramble << 15) | (scramble >> 17);
		scramble *= 0x1b873593;

		hash ^= scramble;
		hash = (hash << 13) | (hash >> 19);
		hash = hash * 5 + 0xe6546b64;
	}

	dataPart = 0;
	for( uintsize i = dataSize & 3; i; --i )
	{
		dataPart <<= 8;
		dataPart |= bytes[ i - 1 ];
	}

	uint32 scramble = dataPart * 0xcc9e2d51;
	scramble = (scramble << 15) | (scramble >> 17);
	scramble *= 0x1b873593;

	hash ^= scramble;
	hash ^= dataSize;
	hash ^= hash >> 16u;
	hash *= 0x85ebca6b;
	hash ^= hash >> 13u;
	hash *= 0xc2b2ae35;
	hash ^= hash >> 16u;

	return hash;
}
