#pragma once

#include "objy/objy.h"

#include "objy_type.h"

//#include "tiki_list.h"

struct ObjyObject
{
	ObjyId					id;
	uint64					index;

	ObjyContext*			context;
	ObjyObject*				parent;
	ObjyType*				type;

	TikiStringView			name;

	//TikiList				children;
	//TikiListNode			childrenNode;
	ObjyObject*				firstChild;
	ObjyObject*				lastChild;
	ObjyObject*				prevSibling;
	ObjyObject*				nextSibling;
	uintsize				childCount;

	ObjyValue*				rootValue;
};

typedef struct ObjyObjectStorage
{
	TikiAllocator*		allocator;

	TikiPool			pool;
	TikiHashMap			idMap;
} ObjyObjectStorage;

bool	objyObjectStorageConstruct( ObjyObjectStorage* objects, TikiAllocator* allocator );
void	objyObjectStorageDestruct( ObjyObjectStorage* objects );
