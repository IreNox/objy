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

struct ObjySystem
{
	TikiAllocator		allocator;

	ObjyTypeCollection	types;

	TikiPool			contextPool;
	ObjyContext*		rootContext;
};

struct ObjyContext
{
	TikiAllocator*		allocator;
	ObjySystem*			system;

	uint64				index;

	ObjyObjectStorage	objects;
	ObjyValueStorage	values;
	ObjyChangeStorage	changes;

	ObjyObject*			rootObject;
};

// TODO
#define TIKI_DEBUG_WARNING( fmt, ... )
#define TIKI_DEBUG_ERROR( fmt, ... )