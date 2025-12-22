#include "objy_change.h"

static void			objyChangeSetFree( ObjyChangeSet* changeSet );

static ObjyChange*	objyChangeCreateInternal( ObjyChangeSet* changeSet, ObjyChangeType type, ObjyId objectId );
static ObjyChange*	objyChangeWriteValueInternal( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, size_t pathLength, ObjyValue* newValue );
static void			objyChangeFree( ObjyChangeSet* changeSet, ObjyChange* change );

static bool			objyChangeApply( ObjyChangeSet* changeSet, ObjyChange* change, ObjyObject* object );
static bool			objyChangeApplyCreate( ObjyChangeSet* changeSet, ObjyChange* change, ObjyObject* object );
static bool			objyChangeApplyDelete( ObjyChangeSet* changeSet, ObjyObject* object );
static bool			objyChangeApplyMove( ObjyChangeSet* changeSet, ObjyChange* change, ObjyObject* object );
static bool			objyChangeApplyModify( ObjyChangeSet* changeSet, ObjyChange* change, ObjyObject* object );

void objyChangeStorageConstruct( ObjyChangeStorage* changes, TikiAllocator* allocator )
{
	changes->allocator		= allocator;
	changes->firstChangeSet	= NULL;
	changes->lastChangeSet	= NULL;

	tikiPoolConstruct( &changes->changeSetPool, allocator, sizeof( ObjyChangeSet ), 256u );
}

void objyChangeStorageDestruct( ObjyChangeStorage* changes )
{
	tikiPoolDestruct( &changes->changeSetPool );

	changes->allocator		= NULL;
	changes->firstChangeSet	= NULL;
	changes->lastChangeSet	= NULL;
}

void objyChangeStorageDestroyChangeSets( ObjyChangeStorage* changes )
{
	ObjyChangeSet* nextChangeSet = NULL;
	for( ObjyChangeSet* changeSet = changes->lastOpenChangeSet; changeSet != NULL; changeSet = nextChangeSet )
	{
		nextChangeSet = changeSet->prevChangeSet;
		objyChangeSetFree( changeSet );
	}

	for( ObjyChangeSet* changeSet = changes->lastDetachedChangeSet; changeSet != NULL; changeSet = nextChangeSet )
	{
		nextChangeSet = changeSet->prevChangeSet;
		objyChangeSetFree( changeSet );
	}

	for( ObjyChangeSet* changeSet = changes->lastChangeSet; changeSet != NULL; changeSet = nextChangeSet )
	{
		nextChangeSet = changeSet->prevChangeSet;
		objyChangeSetFree( changeSet );
	}
}

ObjyChangeSet* objyChangeSetBegin( ObjyContext* context )
{
	ObjyChangeSet* changeSet = NULL;
	const uint64 index = tikiPoolAllocateZero( &context->changes.changeSetPool, (void**)&changeSet );
	if( index == TIKI_POOL_INVALID_INDEX )
	{
		return NULL;
	}

	if( !objyObjectStateContextConstruct( &changeSet->stateContext, context, context->currentState ) )
	{
		tikiPoolFree( &context->changes.changeSetPool, index );
		return NULL;
	}

	changeSet->index	= index;
	changeSet->context	= context;

	return changeSet;
}

ObjyChangeSet* objyChangeSetBeginDetached( ObjyContext* context )
{
	ObjyChangeSet* changeSet = objyChangeSetBegin( context );
	if( !changeSet )
	{
		return NULL;
	}

	changeSet->isDetached = true;

	return changeSet;
}

//bool objyChangeSetMerge( ObjyChangeSet* changeSet, const ObjyChangeSet* sourceChangeSet );

void objyChangeSetDiscard( ObjyChangeSet* changeSet )
{
	if( !changeSet->isDetached )
	{
		// undo changes
	}

	objyChangeSetFree( changeSet );
}

bool objyChangeSetSubmit( ObjyChangeSet* changeSet )
{
	const bool hasError = changeSet->hasError;
	if( hasError )
	{
		objyChangeSetDiscard( changeSet );
		return false;
	}

	if( changeSet->isDetached )
	{
		// apply changes
	}

	changeSet->prevChangeSet = changeSet->context->changes.lastChangeSet;
	changeSet->context->changes.lastChangeSet = changeSet;

	return true;
}

static void objyChangeSetFree( ObjyChangeSet* changeSet )
{
	objyObjectStateContextDestruct( &changeSet->stateContext );

	for( uintsize i = 0; i < changeSet->changeCount; ++i )
	{
		objyChangeFree( changeSet, &changeSet->changes[ i ] );
	}

	TIKI_MEMORY_ARRAY_FREE( changeSet->context->allocator, changeSet->changes, changeSet->changeCapacity );

	tikiPoolFree( &changeSet->context->changes.changeSetPool, changeSet->index );
}

ObjyBlob objyChangeSetSerialize( ObjyContext* context, const ObjyChangeSet* changeSet, ObjyChangeSetFormatter* formatter )
{
	return formatter->serializeFunc( changeSet, formatter->userData );
}

ObjyChangeSet* objyChangeSetDeserialize( ObjyContext* context, ObjyBlob data, ObjyChangeSetFormatter* formatter )
{
	ObjyChangeSet* changeSet = objyChangeSetBegin( context );
	if( !changeSet )
	{
		return NULL;
	}

	if( !formatter->deserializeFunc( changeSet, data, formatter->userData ) )
	{
		objyChangeSetDiscard( changeSet );
		return NULL;
	}

	return changeSet;
}

bool objyChangeSetDeserializeMerge( ObjyChangeSet* changeSet, ObjyBlob data, ObjyChangeSetFormatter* formatter )
{
	ObjyChangeSet* secondChangeSet = objyChangeSetDeserialize( changeSet->context, data, formatter->userData );
	if( !secondChangeSet )
	{
		return false;
	}

	const bool result = objyChangeSetMerge( changeSet, secondChangeSet );
	objyChangeSetDiscard( secondChangeSet );
	return result;
}

bool objyChangeSetIsValid( const ObjyChangeSet* changeSet )
{
	return !changeSet->hasError;
}

static ObjyChange* objyChangeCreateInternal( ObjyChangeSet* changeSet, ObjyChangeType type, ObjyId objectId )
{
	if( !TIKI_MEMORY_ARRAY_CHECK_CAPACITY_ZERO( changeSet->context->allocator, changeSet->changes, changeSet->changeCapacity, changeSet->changeCount + 1u ) )
	{
		TIKI_DEBUG_ERROR( "Can't allocate changes for change set." );
		changeSet->hasError = true;
		return NULL;
	}

	ObjyChange* change = &changeSet->changes[ changeSet->changeCount++ ];
	change->type		= type;
	change->objectId	= objectId;

	return change;
}

ObjyObject* objyChangeSetObjectCreate( ObjyChangeSet* changeSet, ObjyId id, const char* name, const ObjyType* structType, ObjyId parentId )
{
	return objyChangeSetObjectCreateValue( changeSet, id, name, strlen( name ), structType, parentId, NULL );
}

ObjyObject* objyChangeSetObjectCreateValue( ObjyChangeSet* changeSet, ObjyId id, const char* name, size_t nameLength, const ObjyType* structType, ObjyId parentId, ObjyValue* initValue )
{
	ObjyChange* change = objyChangeCreateInternal( changeSet, ObjyChangeType_Create, id );
	if( !change )
	{
		return NULL;
	}

	ObjyChangeCreateData* createData = &change->data.create;
	createData->name		= tikiStringViewAllocateCopyPointerLength( changeSet->context->allocator, name, nameLength );
	createData->parentId	= parentId;
	createData->typeName	= structType->name;
	createData->initValue	= initValue;

	if( !objyChangeApply( changeSet, change, NULL ) )
	{
		changeSet->changeCount--;
		return NULL;
	}

	ObjyObject* object = objyObjectStorageFind( &changeSet->context->objects, id );
	if( !object )
	{
		TIKI_DEBUG_ERROR( "Can't find newly created object." );
		changeSet->hasError = true;
		changeSet->changeCount--;
		return NULL;
	}

	return object;
}

void objyChangeSetObjectDelete( ObjyChangeSet* changeSet, ObjyObject* object )
{
	if( !object )
	{
		changeSet->hasError = true;
		TIKI_DEBUG_ERROR( "Can't create change no object given." );
		return;
	}

	ObjyChange* change = objyChangeCreateInternal( changeSet, ObjyChangeType_Delete, object->id );
	if( !change )
	{
		return;
	}

	if( !objyChangeApply( changeSet, change, object ) )
	{
		changeSet->changeCount--;
	}
}

void objyChangeSetObjectDeleteId( ObjyChangeSet* changeSet, ObjyId objectId )
{
	ObjyObject* object = objyObjectStorageFind( &changeSet->context->objects, objectId );
	if( !object )
	{
		TIKI_DEBUG_ERROR( "Can't delete object no object found." );
		return;
	}

	objyChangeSetObjectDelete( changeSet, object );
}

void objyChangeSetObjectMove( ObjyChangeSet* changeSet, ObjyObject* object, ObjyObject* newParent )
{
	if( !object )
	{
		changeSet->hasError = true;
		TIKI_DEBUG_ERROR( "Can't create change no object given." );
		return;
	}

	ObjyChange* change = objyChangeCreateInternal( changeSet, ObjyChangeType_Delete, object->id );
	if( !change )
	{
		changeSet->hasError = true;
		return;
	}

	ObjyChangeMoveData* moveData = &change->data.move;
	moveData->oldParentId	= object->parent ? object->parent->id : ObjyIdInvalid;
	moveData->newParentId	= newParent ? newParent->id : ObjyIdInvalid;

	if( !objyChangeApply( changeSet, change, object ) )
	{
		changeSet->changeCount--;
	}
}

static ObjyChange* objyChangeWriteValueInternal( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, size_t pathLength, ObjyValue* newValue )
{
	if( !object )
	{
		changeSet->hasError = true;
		TIKI_DEBUG_ERROR( "Can't create change no object given." );
		return NULL;
	}

	ObjyChange* change = objyChangeCreateInternal( changeSet, ObjyChangeType_Modify, object->id );
	if( !change )
	{
		return NULL;
	}

	const ObjyValue* oldValue = objyObjectFindValueLength( object, path, pathLength );

	ObjyChangeModifyData* modifyData = &change->data.modify;
	modifyData->path		= tikiStringViewAllocateCopyPointerLength( changeSet->context->allocator, path, pathLength );
	modifyData->oldValue	= objyValueCreateCopy( changeSet->context, oldValue );
	modifyData->newValue	= newValue;

	if( !objyChangeApply( changeSet, change, object ) )
	{
		objyChangeFree( changeSet, change );
		changeSet->changeCount--;
		return NULL;
	}

	return change;
}

void objyChangeSetWriteId( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, ObjyId value )
{
	ObjyValue* valueData = objyValueCreateId( changeSet->context, value );
	if( !valueData )
	{
		changeSet->hasError = true;
		return;
	}

	objyChangeWriteValueInternal( changeSet, object, path, strlen( path ), valueData );
}

void					objyChangeSetWriteBool( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, bool value );
void					objyChangeSetWriteUInt( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, uint64_t value );
void					objyChangeSetWriteSInt( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, int64_t value );
void					objyChangeSetWriteFloat( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, double value );
void					objyChangeSetWriteString( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const char* value );
void					objyChangeSetWriteArray( ObjyChangeSet* changeSet, ObjyObject* object, const char* path );
void					objyChangeSetWriteReference( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const ObjyType* type );

static bool objyChangeApply( ObjyChangeSet* changeSet, ObjyChange* change, ObjyObject* object )
{
	if( changeSet->isDetached )
	{
		return true;
	}

	bool result = false;
	switch( change->type )
	{
	case ObjyChangeType_Create:
		result = objyChangeApplyCreate( changeSet, change, object );
		break;

	case ObjyChangeType_Delete:
		result = objyChangeApplyDelete( changeSet, object );
		break;

	case ObjyChangeType_Move:
		result = objyChangeApplyMove( changeSet, change, object );
		break;

	case ObjyChangeType_Modify:
		result = objyChangeApplyModify( changeSet, change, object );
		break;

	default:
		TIKI_DEBUG_ERROR( "Invalid change type: %d", change->type );
		break;
	}

	if( !result )
	{
		changeSet->hasError = true;
	}

	return false;
}

static bool objyChangeApplyCreate( ObjyChangeSet* changeSet, ObjyChange* change, ObjyObject* object )
{
	ObjyChangeCreateData* createData = &change->data.create;

	ObjyObject* object = objyObjectStorageCreateObject( changeSet->context, createData->id, createData->name, createData->typeName, createData->parentId );
	if( !object )
	{
		changeSet->hasError = true;
		return false;
	}

	return true;
}

static bool objyChangeApplyDelete( ObjyChangeSet* changeSet, ObjyObject* object )
{
	objyObjectStorageDestroyObject( &changeSet->context->objects, object );
	return true;
}

static bool objyChangeApplyMove( ObjyChangeSet* changeSet, ObjyChange* change, ObjyObject* object )
{
	ObjyObject* newParent = NULL;
	if( objyIdIsValid( change->data.move.newParentId ) )
	{
		newParent = objyObjectStorageFind( &changeSet->context->objects, change->data.move.newParentId );
		if( !newParent )
		{
			TIKI_DEBUG_ERROR( "Could not find new parent with id '" OBJY_ID_FORMAT_STRING "'.", OBJY_ID_FORMAT_DATA( change->data.move.newParentId ) );
			return false;
		}
	}

	objyObjectRemoveFromParent( object );
	objyObjectAddToParent( object, newParent );

	return true;
}

static bool objyChangeApplyModify( ObjyChangeSet* changeSet, ObjyChange* change, ObjyObject* object )
{
#pragma message( "objyChangeApplyModify: not implemented" )
}

static void objyChangeFree( ObjyChangeSet* changeSet, ObjyChange* change )
{
	if( change->type == ObjyChangeType_Modify )
	{
		ObjyChangeModifyData* modifyData = &change->data.modify;

		tikiMemoryFree( changeSet->context->allocator, modifyData->path.data );
		objyValueDestroy( changeSet->context, modifyData->oldValue );
		objyValueDestroy( changeSet->context, modifyData->newValue );
	}
}