#pragma once

#include "tiki_types.h"

#include <stdbool.h>

#ifndef TIKI_DEFAULT_ARRAY_CAPACITY
#	define TIKI_DEFAULT_ARRAY_CAPACITY				16u
#endif

#define TIKI_MEMORY_NEW( ALLOCATOR, TYPE )		(TYPE*)tikiMemoryAlloc( ALLOCATOR, sizeof( TYPE ) )
#define TIKI_MEMORY_NEW_ZERO( ALLOCATOR, TYPE )	(TYPE*)tikiMemoryAllocZero( ALLOCATOR, sizeof( TYPE ) )

#define TIKI_MEMORY_ARRAY_NEW( ALLOCATOR, TYPE, COUNT )			(TYPE*)tikiMemoryAlloc( ALLOCATOR, sizeof( TYPE ) * COUNT )
#define TIKI_MEMORY_ARRAY_NEW_ZERO( ALLOCATOR, TYPE, COUNT )	(TYPE*)tikiMemoryAllocZero( ALLOCATOR, sizeof( TYPE ) * COUNT )

#define TIKI_MEMORY_ARRAY_CHECK_CAPACITY( ALLOCATOR, ARRAY, CAPACITY, COUNT )		tikiMemoryArrayCheckCapacity( ALLOCATOR, (void**)&ARRAY, &CAPACITY, COUNT, sizeof( *ARRAY ), false )
#define TIKI_MEMORY_ARRAY_CHECK_CAPACITY_ZERO( ALLOCATOR, ARRAY, CAPACITY, COUNT )	tikiMemoryArrayCheckCapacity( ALLOCATOR, (void**)&ARRAY, &CAPACITY, COUNT, sizeof( *ARRAY ), true )

#define TIKI_MEMORY_ARRAY_REMOVE_UNSORTED( ARRAY, COUNT, INDEX )		tikiMemoryArrayRemoveElementUnsorted( ARRAY, &COUNT, INDEX, sizeof( *ARRAY ), false )
#define TIKI_MEMORY_ARRAY_REMOVE_UNSORTED_ZERO( ARRAY, COUNT, INDEX )	tikiMemoryArrayRemoveElementUnsorted( ARRAY, &COUNT, INDEX, sizeof( *ARRAY ), true )

#define TIKI_MEMORY_ARRAY_SHRINK( ALLOCATOR, ARRAY, CAPACITY, COUNT )	tikiMemoryArrayShrink( ALLOCATOR, (void**)&ARRAY, &CAPACITY, COUNT, sizeof( *ARRAY ) )

#define TIKI_MEMORY_ARRAY_FREE( ALLOCATOR, ARRAY, CAPACITY )			tikiMemoryArrayFree( ALLOCATOR, (void**)&ARRAY, &CAPACITY )

void	tikiMemoryAllocatorPrepare( TikiAllocator* targetAllocator, TikiAllocatorMallocFunc mallocFunc, TikiAllocatorReallocFunc reallocFunc, TikiAllocatorFreeFunc freeFunc, void* userData );
void	tikiMemoryAllocatorFinalize( TikiAllocator* targetAllocator, const TikiAllocator* sourceAllocator );

void*	tikiMemoryAlloc( TikiAllocator* allocator, uintsize size );
void*	tikiMemoryAllocZero( TikiAllocator* allocator, uintsize size );
void*	tikiMemoryRealloc( TikiAllocator* allocator, void* oldMemory, uintsize oldSize, uintsize newSize );
void	tikiMemoryFree( TikiAllocator* allocator, const void* memory );

bool	tikiMemoryArrayCheckCapacity( TikiAllocator* allocator, void** memory, uintsize* capacity, uintsize requiredCapacity, uintsize elementSize, bool zero );
void	tikiMemoryArrayRemoveElementUnsorted( void* memory, uintsize* arrayCount, uintsize elementIndex, uintsize elementSize, bool zero );
void	tikiMemoryArrayShrink( TikiAllocator* allocator, void** memory, uintsize* capacity, uintsize count, uintsize elementSize );
void	tikiMemoryArrayFree( TikiAllocator* allocator, void** memory, uintsize* capacity );
