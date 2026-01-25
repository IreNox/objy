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
	const ObjyType*			type;

	TikiStringView			name;

	//TikiList				children;
	//TikiListNode			childrenNode;
	ObjyObject*				firstChild;
	ObjyObject*				lastChild;
	ObjyObject*				prevSibling;
	ObjyObject*				nextSibling;
	uintsize				childCount;

	//ObjyValue*				rootValue;
};

typedef struct ObjyObjectState
{
	ObjyId					id;
	uint64					index;

	bool					isNewObject;
	ObjyObject*				object;
	ObjyValue*				value;
} ObjyObjectState;

typedef struct ObjyObjectStateContext
{
	ObjyContext*					context;

	struct ObjyObjectStateContext*	parentState;

	TikiHashMap						idMap;
} ObjyObjectStateContext;


typedef struct ObjyObjectStorage
{
	TikiAllocator*		allocator;

	TikiPool			pool;
	TikiHashMap			idMap;
} ObjyObjectStorage;

bool		objyObjectStorageConstruct( ObjyObjectStorage* objects, TikiAllocator* allocator );
void		objyObjectStorageDestruct( ObjyObjectStorage* objects );

ObjyObject*	objyObjectStorageCreateObject( ObjyContext* context, ObjyId id, TikiStringView name, const ObjyType* structType, ObjyId parentId );
void		objyObjectStorageDestroyObject( ObjyContext* context, ObjyObject* object );

ObjyObject*	objyObjectStorageFind( ObjyObjectStorage* objects, ObjyId id );

void		objyObjectAddToParent( ObjyObject* object, ObjyObject* parent );
void		objyObjectRemoveFromParent( ObjyObject* object );

bool		objyObjectStateContextConstruct( ObjyObjectStateContext* stateContext, ObjyContext* context, ObjyObjectStateContext* parentState );
void		objyObjectStateContextDestruct( ObjyObjectStateContext* stateContext );

ObjyValue*	objyObjectStateContextFind( ObjyObjectStateContext* stateContext, ObjyId id );
ObjyValue*	objyObjectStateContextFindOrCreate( ObjyObjectStateContext* stateContext, ObjyObject* object );
bool		objyObjectStateContextSet( ObjyObjectStateContext* stateContext, ObjyObject* object, ObjyValue* value, bool isNewObject );
