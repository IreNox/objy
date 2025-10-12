#include "tiki_memory.h"

#include <stdlib.h>
#include <string.h>

static void*	tikiMemoryDefaultAlloc( uintsize size, void* userData );
static void*	tikiMemoryDefaultRealloc( void* oldMemory, uintsize oldSize, uintsize newSize, void* userData );
static void*	tikiMemoryPseudoRealloc( void* oldMemory, uintsize oldSize, uintsize newSize, void* userData );
static void		tikiMemoryDefaultFree( void* memory, void* userData );

void tikiMemoryAllocatorPrepare( TikiAllocator* targetAllocator, TikiAllocatorMallocFunc mallocFunc, TikiAllocatorReallocFunc reallocFunc, TikiAllocatorFreeFunc freeFunc, void* userData )
{
	if( mallocFunc == NULL ||
		freeFunc == NULL )
	{
		targetAllocator->mallocFunc		= tikiMemoryDefaultAlloc;
		targetAllocator->reallocFunc	= tikiMemoryDefaultRealloc;
		targetAllocator->freeFunc		= tikiMemoryDefaultFree;
		targetAllocator->userData		= NULL;
		targetAllocator->internalData	= NULL;
	}
	else
	{
		targetAllocator->mallocFunc		= mallocFunc;
		targetAllocator->reallocFunc	= reallocFunc;
		targetAllocator->freeFunc		= freeFunc;
		targetAllocator->userData		= userData;
		targetAllocator->internalData	= userData;
	}
}

void tikiMemoryAllocatorFinalize( TikiAllocator* targetAllocator, const TikiAllocator* sourceAllocator )
{
	*targetAllocator = *sourceAllocator;

	if( targetAllocator->reallocFunc == NULL )
	{
		targetAllocator->reallocFunc	= tikiMemoryPseudoRealloc;
		targetAllocator->internalData	= targetAllocator;
	}
}

void* tikiMemoryAlloc( TikiAllocator* allocator, uintsize size )
{
	return allocator->mallocFunc( size, allocator->userData );
}

void* tikiMemoryAllocZero( TikiAllocator* allocator, uintsize size )
{
	void* memory = tikiMemoryAlloc( allocator, size );
	if( memory == NULL )
	{
		return NULL;
	}

	memset( memory, 0, size );
	return memory;
}

void* tikiMemoryRealloc( TikiAllocator* allocator, void* oldMemory, uintsize oldSize, uintsize newSize )
{
	return allocator->reallocFunc( oldMemory, oldSize, newSize, allocator->internalData );
}

void tikiMemoryFree( TikiAllocator* allocator, const void* memory )
{
	allocator->freeFunc( (void*)memory, allocator->userData );
}

bool tikiMemoryArrayCheckCapacity( TikiAllocator* allocator, void** memory, uintsize* capacity, uintsize requiredCapacity, uintsize elementSize, bool zero )
{
	if( *capacity >= requiredCapacity )
	{
		return true;
	}

	uintsize nextCapacity = TIKI_NEXT_POWER_OF_TWO( requiredCapacity );
	if( nextCapacity < TIKI_DEFAULT_ARRAY_CAPACITY )
	{
		nextCapacity = TIKI_DEFAULT_ARRAY_CAPACITY;
	}

	const uintsize oldSize = *capacity * elementSize;
	const uintsize newSize = nextCapacity * elementSize;
	void* newMemory = tikiMemoryRealloc( allocator, *memory, oldSize, newSize );
	if( !newMemory )
	{
		return false;
	}

	if( zero )
	{
		uint8* data = (uint8*)newMemory;
		memset( data + oldSize, 0, newSize - oldSize );
	}

	*memory = newMemory;
	*capacity = nextCapacity;
	return true;
}

void tikiMemoryArrayRemoveElementUnsorted( void* memory, uintsize* arrayCount, uintsize elementIndex, uintsize elementSize, bool zero )
{
	TIKI_ASSERT( elementIndex < *arrayCount );

	uint8* bytes = (uint8*)memory;
	uint8* element = bytes + (elementIndex * elementSize);

	if( *arrayCount > 1u &&
		elementIndex != *arrayCount - 1u )
	{
		uint8* lastElement = bytes + ((*arrayCount - 1u) * elementSize);
		memcpy( element, lastElement, elementSize );

		if( zero )
		{
			memset( lastElement, 0, elementSize );
		}
	}
	else if( zero &&
		*arrayCount == 1u )
	{
		memset( element, 0, elementSize );
	}

	(*arrayCount)--;
}

void tikiMemoryArrayShrink( TikiAllocator* allocator, void** memory, uintsize* capacity, uintsize count, uintsize elementSize )
{
	uintsize shrinkCapacity = *capacity >> 1;
	if( shrinkCapacity > 0 &&
		shrinkCapacity < TIKI_DEFAULT_ARRAY_CAPACITY )
	{
		shrinkCapacity = TIKI_DEFAULT_ARRAY_CAPACITY;
	}

	if( count >= shrinkCapacity ||
		shrinkCapacity >= *capacity )
	{
		return;
	}

	*memory = tikiMemoryRealloc( allocator, *memory, *capacity * elementSize, shrinkCapacity * elementSize );

	if( !*memory )
	{
		return;
	}

	*capacity = shrinkCapacity;
}

void tikiMemoryArrayFree( TikiAllocator* allocator, void** memory, uintsize* capacity )
{
	tikiMemoryFree( allocator, *memory );
	*memory		= NULL;
	*capacity	= 0u;
}

static void* tikiMemoryDefaultAlloc( uintsize size, void* userData )
{
	(void)userData;

	return malloc( size );
}

static void* tikiMemoryDefaultRealloc( void* oldMemory, uintsize oldSize, uintsize newSize, void* userData )
{
	(void)oldSize;
	(void)userData;

	return realloc( oldMemory, newSize );
}

static void* tikiMemoryPseudoRealloc( void* oldMemory, uintsize oldSize, uintsize newSize, void* userData )
{
	(void)userData;

	TikiAllocator* allocator = (TikiAllocator*)userData;
	void* newMemory = tikiMemoryAlloc( allocator, newSize );
	if( newMemory == NULL )
	{
		tikiMemoryFree( allocator, oldMemory );
		return NULL;
	}

	memcpy( newMemory, oldMemory, oldSize < newSize ? oldSize : newSize );
	tikiMemoryFree( allocator, oldMemory );

	return newMemory;
}

static void tikiMemoryDefaultFree( void* memory, void* userData )
{
	(void)userData;

	free( memory );
}
