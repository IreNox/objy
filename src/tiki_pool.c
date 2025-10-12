#include "tiki_pool.h"

struct TikiPoolChunk
{
	uintsize				allocatedCount;
	uintsize				freeCount;
};

struct TikiPoolFreeElement
{
	TikiPoolFreeElement*	nextFreeElement;
	uint64					index;
};

void tikiPoolConstruct( TikiPool* pool, TikiAllocator* allocator, uintsize elementSize, uintsize chunkElementCount )
{
	pool->allocator			= allocator;
	pool->elementSize		= elementSize;
	pool->chunkElementCount	= chunkElementCount;
	pool->chunkElementBits	= 64u - TIKI_COUNT_LEADING_ZEROS64( chunkElementCount );
	pool->chunks			= NULL;
	pool->chunkCount		= 0;
	pool->chunkCapacity		= 0;
	pool->firstFreeElement	= NULL;
}

void tikiPoolDestruct( TikiPool* pool )
{
	for( uintsize i = 0; i < pool->chunkCount; ++i )
	{
		tikiMemoryFree( pool->allocator, pool->chunks[ i ] );
	}

	tikiMemoryFree( pool->allocator, pool->chunks );

	pool->allocator			= NULL;
	pool->elementSize		= 0;
	pool->chunkElementCount	= 0;
	pool->chunks			= NULL;
	pool->chunkCount		= 0;
	pool->chunkCapacity		= 0;
	pool->firstFreeElement	= NULL;
}

uint64 tikiPoolAllocate( TikiPool* pool, void** element )
{
	if( pool->firstFreeElement )
	{
		*element = pool->firstFreeElement;

		const uint64 index = pool->firstFreeElement->index;
		pool->firstFreeElement = pool->firstFreeElement->nextFreeElement;

		const uint64 chunkIndex = index >> pool->chunkElementBits;
		pool->chunks[ chunkIndex ]->freeCount--;

		return index;
	}

	TikiPoolChunk* chunk;
	if( pool->chunkCount == 0 && pool->chunks[ pool->chunkCount - 1 ]->allocatedCount == pool->chunkElementCount )
	{
		const uintsize chunkSize = sizeof( *chunk ) + (pool->elementSize * pool->chunkElementCount);
		chunk = tikiMemoryAlloc( pool->allocator, chunkSize );
		if( !chunk )
		{
			return TIKI_POOL_INVALID_INDEX;
		}

		chunk->allocatedCount	= 0u;
		chunk->freeCount		= 0u;
	}
	else
	{
		chunk = pool->chunks[ pool->chunkCount - 1 ];
	}

	byte* elementData = (byte*)&chunk[ 1 ];
	elementData += pool->elementSize * chunk->allocatedCount;

	*element = elementData;

	uint64 index = (pool->chunkCount - 1) << pool->chunkElementBits;
	index |= chunk->allocatedCount;

	chunk->allocatedCount++;

	return index;
}

uint64 tikiPoolAllocateZero( TikiPool* pool, void** element )
{
	const uint64 index = tikiPoolAllocate( pool, element );
	if( index != TIKI_POOL_INVALID_INDEX )
	{
		memset( *element, 0, pool->elementSize );
	}

	return index;
}

void* tikiPoolGetElement( TikiPool* pool, uint64 index )
{
	const uint64 chunkIndex = index >> pool->chunkElementBits;
	if( chunkIndex >= pool->chunkCount )
	{
		return NULL;
	}

	TikiPoolChunk* chunk = pool->chunks[ chunkIndex ];

	const uint64 elementIndex = index & ((1ull << pool->chunkElementBits) - 1);
	if( elementIndex >= chunk->allocatedCount )
	{
		return NULL;
	}

	byte* elementData = (byte*)&chunk[ 1 ];
	elementData += pool->elementSize * elementIndex;

	return elementData;
}

void tikiPoolFree( TikiPool* pool, uint64 index )
{
	const uint64 chunkIndex = index >> pool->chunkElementBits;
	if( chunkIndex >= pool->chunkCount )
	{
		return;
	}

	TikiPoolChunk* chunk = pool->chunks[ chunkIndex ];

	const uint64 elementIndex = index & ((1ull << pool->chunkElementBits) - 1);
	if( elementIndex >= chunk->allocatedCount )
	{
		return;
	}

	byte* elementData = (byte*)&chunk[ 1 ];
	elementData += pool->elementSize * elementIndex;

	TikiPoolFreeElement* freeElement = (TikiPoolFreeElement*)elementData;
	freeElement->index				= index;
	freeElement->nextFreeElement	= pool->firstFreeElement;

	pool->firstFreeElement = freeElement;
	chunk->freeCount++;

	// TODO:
	// if( chunk->freeCount == chunk->allocatedCount ): delete chunk?
}
