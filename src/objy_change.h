#pragma once

#include "objy/objy.h"

#include "objy_object.h"

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
	TikiStringView			name;
	ObjyId					parentId;
	TikiStringView			typeName;
	ObjyValue*				initValue;
} ObjyChangeCreateData;

typedef struct ObjyChangeDeleteData
{
	ObjyValue*				oldValue;
} ObjyChangeDeleteData;

typedef struct ObjyChangeMoveData
{
	ObjyId					oldParentId;
	ObjyId					newParentId;
} ObjyChangeMoveData;

typedef struct ObjyChangeModifyData
{
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
	ObjyId					objectId;
	ObjyChangeData			data;
} ObjyChange;

typedef struct ObjyChangeSetNode
{
	ObjyChangeSet*			prevItem;
	ObjyChangeSet*			nextItem;
} ObjyChangeSetNode;

struct ObjyChangeSet
{
	uint64					index;
	ObjyId					id;

	ObjyContext*			context;
	ObjyObjectStateContext	stateContext;

	bool					isDetached;
	ObjyChangeSetResult		result;

	ObjyChangeSetNode		listNode;
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
	//TikiList				changeSets;

	//TikiList				openChangeSets;
	//TikiList				detachedChangeSets;
	ObjyChangeSet*			firstOpenChangeSet;
	ObjyChangeSet*			lastOpenChangeSet;
	ObjyChangeSet*			firstDetachedChangeSet;
	ObjyChangeSet*			lastDetachedChangeSet;
} ObjyChangeStorage;

void	objyChangeStorageConstruct( ObjyChangeStorage* changes, TikiAllocator* allocator );
void	objyChangeStorageDestruct( ObjyChangeStorage* changes );

void	objyChangeStorageDestroyChangeSets( ObjyChangeStorage* changes );
