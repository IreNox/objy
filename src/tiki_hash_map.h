#pragma once

#include "tiki_types.h"

typedef struct TikiAllocator TikiAllocator;

typedef uint32 TikiHash;
#define TIKI_HASH_DEFAULT_SEED 0xc6b568d8
TikiHash tikiHashMurmur3( const void* data, size_t dataSize );
TikiHash tikiHashMurmur3Seed( const void* data, size_t dataSize, TikiHash seed );

#define TIKI_HASH_MAP_INVALID_INDEX ((uintsize)-1)

typedef TikiHash (*TikiHashMapEntryHashFunc)( const void* entry );
typedef bool (*TikiHashMapIsKeyEqualsFunc)( const void* lhs, const void* rhs );

typedef struct TikiHashMap
{
	TikiAllocator*				allocator;

	uint64*						entriesInUse;
	uint8*						entries;
	uintsize					entryCount;
	uintsize					entryCapacity;

	uintsize					entrySize;
	TikiHashMapEntryHashFunc	entryHashFunc;
	TikiHashMapIsKeyEqualsFunc	entryKeyEqualsFunc;
} TikiHashMap;

bool			tikiHashMapConstructSize( TikiHashMap* hashMap, TikiAllocator* allocator, uintsize entrySize, TikiHashMapEntryHashFunc entryHashFunc, TikiHashMapIsKeyEqualsFunc entryKeyEqualsFunc, uintsize initialSize );
bool			tikiHashMapConstructStatic( TikiHashMap* hashMap, TikiAllocator* allocator, const void* data, uintsize entrySize, uintsize entryCount, TikiHashMapEntryHashFunc entryHashFunc, TikiHashMapIsKeyEqualsFunc entryKeyEqualsFunc );
bool			tikiHashMapConstructStaticPointer( TikiHashMap* hashMap, TikiAllocator* allocator, const void* data, uintsize entrySize, uintsize entryCount, TikiHashMapEntryHashFunc entryHashFunc, TikiHashMapIsKeyEqualsFunc entryKeyEqualsFunc );
void			tikiHashMapDestruct( TikiHashMap* hashMap );

void*			tikiHashMapFind( TikiHashMap* hashMap, const void* entry );

void*			tikiHashMapInsert( TikiHashMap* hashMap, const void* entry );
void*			tikiHashMapInsertNew( TikiHashMap* hashMap, const void* entry, bool* isNew );
bool			tikiHashMapRemove( TikiHashMap* hashMap, const void* entry );

uintsize		tikiHashMapFindFirstIndex( TikiHashMap* hashMap );
uintsize		tikiHashMapFindNextIndex( TikiHashMap* hashMap, uintsize entry );

void*			tikiHashMapGetEntry( TikiHashMap* hashMap, uintsize index );
