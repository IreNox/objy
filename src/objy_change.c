#include "objy_change.h"

#include "tiki_debug.h"

#include <stdlib.h>

static void			objyChangeSetFree( ObjyChangeSet* changeSet );

static ObjyChange*	objyChangeCreateInternal( ObjyChangeSet* changeSet, ObjyChangeType type, ObjyId objectId );
static ObjyChange*	objyChangeWriteValueInternal( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, size_t pathLength, ObjyValue* newValue );
static void			objyChangeFree( ObjyChangeSet* changeSet, ObjyChange* change );

static bool			objyChangeApply( ObjyChangeSet* changeSet, ObjyChange* change, ObjyObject* object );
static bool			objyChangeApplyCreate( ObjyChangeSet* changeSet, ObjyChange* change );
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

//bool objyChangeSetDeserializeMerge( ObjyChangeSet* changeSet, ObjyBlob data, ObjyChangeSetFormatter* formatter )
//{
//	ObjyChangeSet* secondChangeSet = objyChangeSetDeserialize( changeSet->context, data, formatter->userData );
//	if( !secondChangeSet )
//	{
//		return false;
//	}
//
//	const bool result = objyChangeSetMerge( changeSet, secondChangeSet );
//	objyChangeSetDiscard( secondChangeSet );
//	return result;
//}

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
	createData->id			= id;
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

void objyChangeSetWriteBool( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, bool value )
{
	ObjyValue* valueData = objyValueCreateBool( changeSet->context, value );
	if( !valueData )
	{
		changeSet->hasError = true;
		return;
	}

	objyChangeWriteValueInternal( changeSet, object, path, strlen( path ), valueData );
}

void objyChangeSetWriteUInt( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, uint64_t value )
{
	ObjyValue* valueData = objyValueCreateUInt( changeSet->context, value );
	if( !valueData )
	{
		changeSet->hasError = true;
		return;
	}

	objyChangeWriteValueInternal( changeSet, object, path, strlen( path ), valueData );
}

void objyChangeSetWriteSInt( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, int64_t value )
{
	ObjyValue* valueData = objyValueCreateSInt( changeSet->context, value );
	if( !valueData )
	{
		changeSet->hasError = true;
		return;
	}

	objyChangeWriteValueInternal( changeSet, object, path, strlen( path ), valueData );
}

void objyChangeSetWriteFloat( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, double value )
{
	ObjyValue* valueData = objyValueCreateFloat( changeSet->context, value );
	if( !valueData )
	{
		changeSet->hasError = true;
		return;
	}

	objyChangeWriteValueInternal( changeSet, object, path, strlen( path ), valueData );
}

void objyChangeSetWriteString( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const char* value )
{
	objyChangeSetWriteStringLength( changeSet, object, path, value, strlen( value ) );
}

void objyChangeSetWriteStringLength( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const char* value, size_t stringLength )
{
	ObjyValue* valueData = objyValueCreateStringLength( changeSet->context, value, stringLength );
	if( !valueData )
	{
		changeSet->hasError = true;
		return;
	}

	objyChangeWriteValueInternal( changeSet, object, path, strlen( path ), valueData );
}

//void objyChangeSetWriteArray( ObjyChangeSet* changeSet, ObjyObject* object, const char* path )
//{
//	ObjyValue* valueData = objyValueCreateArray( changeSet->context, ... );
//	if( !valueData )
//	{
//		changeSet->hasError = true;
//		return;
//	}
//
//	objyChangeWriteValueInternal( changeSet, object, path, strlen( path ), valueData );
//}
//
//void objyChangeSetWriteReference( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const ObjyType* type )
//{
//	ObjyValue* valueData = objyValueCreateStruct( changeSet->context, ... );
//	if( !valueData )
//	{
//		changeSet->hasError = true;
//		return;
//	}
//
//	objyChangeWriteValueInternal( changeSet, object, path, strlen( path ), valueData );
//}

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
		result = objyChangeApplyCreate( changeSet, change );
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

	return result;
}

static bool objyChangeApplyCreate( ObjyChangeSet* changeSet, ObjyChange* change )
{
	ObjyChangeCreateData* createData = &change->data.create;

	const ObjyType* type = objyTypeCollectionFind( &changeSet->context->system->types, createData->typeName.data );
	if( !type )
	{
		TIKI_DEBUG_ERROR( "Could not find type with name '%s'.", createData->typeName.data );
		return false;
	}

	ObjyObject* object = objyObjectStorageCreateObject( changeSet->context, createData->id, createData->name, type, createData->parentId, NULL );
	if( !object )
	{
		changeSet->hasError = true;
		return false;
	}

	if( createData->initValue )
	{
		if( !objyObjectStateContextSet( &changeSet->stateContext, object, createData->initValue, true ) )
		{
			changeSet->hasError = true;
			objyObjectStorageDestroyObject( changeSet->context, object );
			return false;
		}
	}

	return true;
}

static bool objyChangeApplyDelete( ObjyChangeSet* changeSet, ObjyObject* object )
{
	objyObjectStorageDestroyObject( changeSet->context, object );
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
	ObjyChangeModifyData* modifyData = &change->data.modify;

	const ObjyType* pathType = objyTypeFindPathType( object->type, modifyData->path );
	if( !pathType )
	{
		TIKI_DEBUG_ERROR( "Could not find type for path '%.*s' on type '%.*s'", modifyData->path.length, modifyData->path.data, object->type->name.length, object->type->name.data );
		return false;
	}

	if( modifyData->newValue->kind &&
		modifyData->newValue->kind != pathType->kind )
	{
		TIKI_DEBUG_ERROR( "Can set a '%s' value on a '%s' field for path '%.*s' on object '" OBJY_ID_FORMAT_STRING "'", objyTypeKindGetString( modifyData->newValue->kind ), objyTypeKindGetString( pathType->kind ), modifyData->path.length, modifyData->path.data, OBJY_ID_FORMAT_DATA( object->id ) );
		return false;
	}

	ObjyValue* rootValue = objyObjectStateContextFindOrCreate( &changeSet->stateContext, object );
	if( !rootValue )
	{
		TIKI_DEBUG_ERROR( "Cloud not create root value for object '" OBJY_ID_FORMAT_STRING "'", OBJY_ID_FORMAT_DATA( object->id ) );
		return false;
	}

	const ObjyType* type = object->type;
	ObjyValue* value = rootValue;
	TikiStringView path = modifyData->path;
	while( path.length )
	{
		if( path.data[ 0 ] == '[' )
		{
			if( value->kind != ObjyTypeKind_Array )
			{
				TIKI_DEBUG_ERROR( "Expected 'Array' value but got '%s' value. Path: %.*s", objyTypeKindGetString( value->kind ), path.length, path.data );
				return false;
			}

			const char* arrayClose = memchr( path.data, ']', path.length );
			if( !arrayClose )
			{
				TIKI_DEBUG_ERROR( "No array close square brackets. Path: %.*s", path.length, path.data );
				return false;
			}

			char* numberEnd = NULL;
			const long arrayIndex = strtol( path.data + 1, &numberEnd, 10 );
			if( numberEnd != arrayClose )
			{
				TIKI_DEBUG_ERROR( "Invalid array index. Path: %.*s", path.length, path.data );
				return false;
			}

			if( arrayIndex >= value->data.arr.valueCount )
			{
				if( !TIKI_MEMORY_ARRAY_CHECK_CAPACITY_ZERO( changeSet->context->allocator, value->data.arr.values, value->data.arr.valueCapacity, arrayIndex + 1 ) )
				{
					return false;
				}

				value->data.arr.values[ arrayIndex ] = objyValueCreate( changeSet->context, type->baseType );
				value->data.arr.valueCount = arrayIndex + 1;
			}

			type = type->baseType;
			value = value->data.arr.values[ arrayIndex ];
			path = tikiStringViewCreateBeginEnd( arrayClose + 1, path.data + path.length );
		}
		else if( value->kind == ObjyTypeKind_Reference )
		{
			type = type->baseType;
			value = value->data.ref.value;
		}
		else if( value->kind == ObjyTypeKind_Struct )
		{
			uintsize endAdd = 1;
			const char* fieldEnd = memchr( path.data, '.', path.length );
			if( !fieldEnd )
			{
				endAdd = 0;
				fieldEnd = path.data + path.length;
			}

			const TikiStringView fieldView = tikiStringViewCreateBeginEnd( path.data, fieldEnd );

			const ObjyType* fieldType = NULL;
			for( uintsize typeFieldIndex = 0; typeFieldIndex < type->globalFieldCount; ++typeFieldIndex )
			{
				const ObjyTypeField* typeField = &type->globalFields[ typeFieldIndex ];
				if( !tikiStringViewIsEqualsStr( fieldView, typeField->name ) )
				{
					continue;
				}

				fieldType = typeField->type;
				break;
			}

			if( !fieldType )
			{
				TIKI_DEBUG_ERROR( "Field '%.*s' not found in Type '%.*s'. Path: %.*s", fieldView.length, fieldView.data, type->name.length, type->name.data, path.length, path.data );
				return false;
			}

			bool found = false;
			for( size_t valueFieldIndex = 0; valueFieldIndex < value->data.struc.valueCount; ++valueFieldIndex )
			{
				ObjyValueField* valueField = &value->data.struc.values[ valueFieldIndex ];
				if( !tikiStringViewIsEquals( fieldView, valueField->name ) )
				{
					continue;
				}

				found = true;
				type = fieldType;
				value = valueField->value;
				break;
			}

			if( !found )
			{
				type = fieldType;
				value = objyValueCreate( changeSet->context, fieldType );
			}

			path = tikiStringViewCreateBeginEnd( fieldEnd + endAdd, path.data + path.length );
		}
		else
		{
			TIKI_DEBUG_ERROR( "Unexpected '%s' value. Path: %.*s", objyTypeKindGetString( value->kind ), path.length, path.data );
			return false;
		}
	}

	if( !objyValueCopyData( changeSet->context, value, modifyData->newValue ) )
	{
		return false;
	}

#pragma message( "objyChangeApplyModify: not implemented" )

	return true;
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