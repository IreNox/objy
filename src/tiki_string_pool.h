#pragma once

#include "tiki_types.h"

#ifndef TIKI_DEFAULT_STRING_POOL_CHUNK_SIZE
#	define TIKI_DEFAULT_STRING_POOL_CHUNK_SIZE		4096u
#endif

typedef struct TikiStringPoolChunk TikiStringPoolChunk;

typedef struct TikiStringPool
{
	TikiAllocator*			allocator;
	TikiStringPoolChunk*	firstChunk;

	TikiHashMap				keyMap;
} TikiStringPool;

bool						tikiStringPoolConstruct( TikiStringPool* stringPool, TikiAllocator* allocator );
void						tikiStringPoolDestruct( TikiStringPool* stringPool );

void						tikiStringPoolClear( TikiStringPool* stringPool );

TikiStringView				tikiStringPoolAdd( TikiStringPool* stringPool, TikiStringView string );
TikiStringView				tikiStringPoolAddPointer( TikiStringPool* stringPool, const char* string );
const TikiStringView*		tikiStringPoolFind( TikiStringPool* stringPool, TikiStringView string );

