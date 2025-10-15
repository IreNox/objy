#pragma once

#include "objy/objy.h"

#include "tiki_pool.h"

typedef enum ObjyChangeType
{
	ObjyChangeType_Create,
	ObjyChangeType_Delete,
	ObjyChangeType_Move,
	ObjyChangeType_Modify
} ObjyChangeType;

typedef struct ObjyChangeCreateData
{
	ObjyId					id;
	TikiStringView			typeName;
	ObjyId					parentId;
} ObjyChangeCreateData;

typedef struct ObjyChangeDeleteData
{
	ObjyId					id;
} ObjyChangeDeleteData;

typedef struct ObjyChangeMoveData
{
	ObjyId					objectId;
	ObjyId					oldParentId;
	ObjyId					newParentId;
} ObjyChangeMoveData;

typedef struct ObjyChangeModifyData
{
	ObjyId					object;
	TikiStringView			path;
	ObjyValue*				oldValue;
	ObjyValue*				newValue;
} ObjyChangeModifyData;

typedef union ObjyChangeData
{
	ObjyChangeCreateData	create;
	ObjyChangeDeleteData	del;	// delete
	ObjyChangeMoveData		move;
	ObjyChangeModifyData	modify;
} ObjyChangeData;

typedef struct ObjyChange
{
	ObjyChangeType			type;
	ObjyChangeData			data;
} ObjyChange;

struct ObjyChangeSet
{
	ObjyContext*			context;

	ObjyChangeSet*			prevChangeSet;
	ObjyChangeSet*			nextChangeSet;

	ObjyChange*				changes;
	uintsize				changeCount;
	uintsize				changeCapacity;
};

typedef struct ObjyChangeStorage
{
	TikiAllocator*			allocator;

	TikiPool				changeSetPool;
	ObjyChangeSet*			firstChangeSet;
	ObjyChangeSet*			lastChangeSet;
} ObjyChangeStorage;

void	objyChangeStorageConstruct( ObjyChangeStorage* changes, TikiAllocator* allocator );
void	objyChangeStorageDestruct( ObjyChangeStorage* changes );
