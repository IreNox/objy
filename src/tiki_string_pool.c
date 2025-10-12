#include "tiki_string_pool.h"

#include <string.h>

struct TikiStringPoolChunk
{
	TikiStringPoolChunk*	nextChunk;

	uintsize				usedSize;
	uintsize				remainingSize;
	char					data[ 1u ];
};

static TikiStringView	tikiStringViewCreateEmpty();
static TikiHash			tikiStringPoolHash( const void* entry );
static bool				tikiStringPoolIsKeyEquals( const void* lhs, const void* rhs );

bool tikiStringPoolConstruct( TikiStringPool* stringPool, TikiAllocator* allocator )
{
	stringPool->allocator	= allocator;
	stringPool->firstChunk	= NULL;

	if( !tikiHashMapConstructSize( &stringPool->keyMap, allocator, sizeof( TikiStringView ), tikiStringPoolHash, tikiStringPoolIsKeyEquals, 64u ) )
	{
		tikiStringPoolDestruct( stringPool );
		return false;
	}

	return true;
}

void tikiStringPoolDestruct( TikiStringPool* stringPool )
{
	tikiHashMapDestruct( &stringPool->keyMap );

	TikiStringPoolChunk* chunk = stringPool->firstChunk;
	TikiStringPoolChunk* nextChunk = NULL;
	while( chunk )
	{
		nextChunk = chunk->nextChunk;
		tikiMemoryFree( stringPool->allocator, chunk );
		chunk = nextChunk;
	}

	stringPool->firstChunk	= NULL;
	stringPool->allocator	= NULL;
}

void tikiStringPoolClear( TikiStringPool* stringPool )
{
	for( TikiStringPoolChunk* chunk = stringPool->firstChunk; chunk != NULL; chunk = chunk->nextChunk )
	{
		chunk->remainingSize += chunk->usedSize;
		chunk->usedSize = 0u;
	}
}

TikiStringView tikiStringPoolAdd( TikiStringPool* stringPool, TikiStringView string )
{
	bool isNew;
	TikiStringView* mapEntry = (TikiStringView*)tikiHashMapInsertNew( &stringPool->keyMap, &string, &isNew );
	if( !mapEntry )
	{
		return tikiStringViewCreateEmpty();
	}
	else if( !isNew )
	{
		return *mapEntry;
	}

	TikiStringPoolChunk* chunk;
	if( stringPool->firstChunk == NULL ||
		string.length >= stringPool->firstChunk->remainingSize )
	{
		uintsize size = TIKI_DEFAULT_STRING_POOL_CHUNK_SIZE;
		if( string.length >= size )
		{
			size = string.length;
		}

		chunk = (TikiStringPoolChunk*)tikiMemoryAlloc( stringPool->allocator, sizeof( TikiStringPoolChunk ) + size );
		if( !chunk )
		{
			return tikiStringViewCreateEmpty();
		}

		chunk->nextChunk		= stringPool->firstChunk;
		chunk->usedSize			= 0u;
		chunk->remainingSize	= size + 1u;

		stringPool->firstChunk = chunk;
	}
	else
	{
		chunk = stringPool->firstChunk;
	}

	char* target = chunk->data + chunk->usedSize;
	memcpy( target, string.data, string.length );
	target[ string.length ] = '\0';

	chunk->usedSize += string.length + 1u;
	chunk->remainingSize -= string.length + 1u;

	mapEntry->data		= target;
	mapEntry->length	= string.length;

	return *mapEntry;
}

TikiStringView tikiStringPoolAddPointer( TikiStringPool* stringPool, const char* string )
{
	const TikiStringView view = { string, strlen( string ) };
	return tikiStringPoolAdd( stringPool, view );
}

const TikiStringView* tikiStringPoolFind( TikiStringPool* stringPool, TikiStringView string )
{
	return (const TikiStringView*)tikiHashMapFind( &stringPool->keyMap, &string );
}

static TikiHash tikiStringPoolHash( const void* entry )
{
	const TikiStringView* string = (const TikiStringView*)entry;
	return tikiHashMurmur3( string->data, string->length );
}

static bool tikiStringPoolIsKeyEquals( const void* lhs, const void* rhs )
{
	const TikiStringView* lhsString = (const TikiStringView*)lhs;
	const TikiStringView* rhsString = (const TikiStringView*)rhs;

	if( lhsString->length != rhsString->length )
	{
		return false;
	}
	else if( lhsString->length == 0u )
	{
		return true;
	}
	else if( lhsString->data[ 0u ] != rhsString->data[ 0u ] )
	{
		return false;
	}

	return memcmp( lhsString->data, rhsString->data, lhsString->length ) == 0;
}

static TikiStringView tikiStringViewCreateEmpty()
{
	TikiStringView string;
	string.data		= NULL;
	string.length	= 0u;
	return string;
}
