#pragma once

#include "tiki_types.h"

typedef struct TikiAllocator TikiAllocator;

typedef struct TikiPoolChunk TikiPoolChunk;
typedef struct TikiPoolFreeElement TikiPoolFreeElement;

typedef struct TikiPool
{
	TikiAllocator*			allocator;

	uintsize				elementSize;
	uintsize				chunkElementCount;
	uintsize				chunkElementBits;

	TikiPoolChunk**			chunks;
	uintsize				chunkCount;
	uintsize				chunkCapacity;

	TikiPoolFreeElement*	firstFreeElement;
} TikiPool;

#define TIKI_POOL_INVALID_INDEX ((uint64)-1)

void						tikiPoolConstruct( TikiPool* pool, TikiAllocator* allocator, uintsize elementSize, uintsize chunkElementCount );
void						tikiPoolDestruct( TikiPool* pool );

uint64						tikiPoolAllocate( TikiPool* pool, void** element );
uint64						tikiPoolAllocateZero( TikiPool* pool, void** element );
void*						tikiPoolGetElement( TikiPool* pool, uint64 index );
void						tikiPoolFree( TikiPool* pool, uint64 index );
