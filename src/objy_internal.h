#pragma once

#include "objy/objy.h"

#include "objy_change.h"
#include "objy_object.h"
#include "objy_type.h"
#include "objy_value.h"

#include "tiki_hash_map.h"
#include "tiki_pool.h"

#ifndef OBJY_CONTEXT_CHUNK_SIZE
#	define OBJY_CONTEXT_CHUNK_SIZE 1024
#endif

#ifndef OBJY_OBJECT_CHUNK_SIZE
#	define OBJY_OBJECT_CHUNK_SIZE 1024
#endif

#ifndef OBJY_OBJECT_STATE_CHUNK_SIZE
#	define OBJY_OBJECT_STATE_CHUNK_SIZE 1024
#endif

struct ObjySystem
{
	TikiAllocator			allocator;

	ObjyTypeCollection		types;

	TikiPool				contextPool;
	ObjyContext*			rootContext;

	ObjyType*				typeFolder;
	ObjyType*				typeKind;
	ObjyType*				typeType;
	ObjyType*				typeField;
	ObjyType*				typeRef;

	ObjyType*				typeBool;
	ObjyType*				typeUInt8;
	ObjyType*				typeUInt16;
	ObjyType*				typeUInt32;
	ObjyType*				typeUInt64;
	ObjyType*				typeSInt8;
	ObjyType*				typeSInt16;
	ObjyType*				typeSInt32;
	ObjyType*				typeSInt64;
	ObjyType*				typeFloat;
	ObjyType*				typeDouble;

	ObjyObject*				typesFolderObject;

	ObjyObject*				typeKindObjects[ TIKI_ARRAY_COUNT( ObjyIdObjyTypeKindValues ) ];

	ObjyObject*				typeTypeKindObject;
	ObjyObject*				typeTypeBaseTypeObject;
	ObjyObject*				typeTypeValueBitCountObject;
	ObjyObject*				typeTypeSignedIntegerObject;

	ObjyObject*				typeFieldTypebject;
};

struct ObjyContext
{
	TikiAllocator*			allocator;
	ObjySystem*				system;

	uint64					index;

	ObjyObject*				rootObject;

	ObjyObjectStorage		objects;
	ObjyValueStorage		values;
	ObjyChangeStorage		changes;

	TikiPool				statePool;
	ObjyObjectStateContext	baseState;
	ObjyObjectStateContext*	currentState;
};
