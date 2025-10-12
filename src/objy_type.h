#pragma once

#include "objy/objy.h"

#include "tiki_hash_map.h"
#include "tiki_pool.h"
#include "tiki_string_pool.h"

#ifndef OBJY_TYPE_CHUNK_SIZE
#	define OBJY_TYPE_CHUNK_SIZE 256
#endif

typedef struct ObjyTypeCollection
{
	TikiAllocator*	allocator;

	ObjyType*		firstType;
	ObjyType*		lastType;

	TikiPool		pool;
	TikiHashMap		nameMap;
	TikiStringPool	stringPool;
} ObjyTypeCollection;

struct ObjyType
{
	TikiStringView	name;
	uint64			index;
	ObjyTypeKind	kind;
	ObjyType*		baseType;
	uint8			bitCount;
	bool			signedInt;
	void*			userData;

	ObjyType*		prevType;
	ObjyType*		nextType;

	ObjyTypeField*	localFields;
	uintsize		localFieldCount;
	ObjyTypeField*	globalFields;
	uintsize		globalFieldCount;
};

bool				objyTypeCollectionConstruct( ObjyTypeCollection* types, TikiAllocator* allocator );
void				objyTypeCollectionDestruct( ObjyTypeCollection* types );

const ObjyType*		objyTypeCollectionFind( ObjyTypeCollection* types, const char* name );
