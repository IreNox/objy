#include "objy_object.h"

#include "objy_internal.h"

#include <stdlib.h>

static TikiHash		objyObjectIdMapHash( const void* entry );
static bool			objyObjectIdMapIsKeyEquals( const void* lhs, const void* rhs );

static ObjyValue*	objyObjectFindValueInternal( ObjyValue* value, TikiStringView path );

static TikiHash		objyObjectStateIdMapHash( const void* entry );
static bool			objyObjectStateIdMapIsKeyEquals( const void* lhs, const void* rhs );

bool objyObjectStorageConstruct( ObjyObjectStorage* objects, TikiAllocator* allocator )
{
	objects->allocator = allocator;

	tikiPoolConstruct( &objects->pool, allocator, sizeof( ObjyObject ), 128u );

	return tikiHashMapConstructSize( &objects->idMap, allocator, sizeof( ObjyObject* ), objyObjectIdMapHash, objyObjectIdMapIsKeyEquals, 16u );
}

void objyObjectStorageDestruct( ObjyObjectStorage* objects )
{
	tikiHashMapDestruct( &objects->idMap );
	tikiPoolDestruct( &objects->pool );
}

ObjyObject* objyObjectStorageCreateObject( ObjyContext* context, ObjyId id, TikiStringView name, const ObjyType* structType, ObjyId parentId, ObjyValue* baseValue )
{
	if( !objyIdIsValid( id ) )
	{
		TIKI_DEBUG_ERROR( "Invalid ID to create object." );
		return NULL;
	}

	const ObjyId* mapId = &id;
	if( tikiHashMapFind( &context->objects.idMap, &mapId ) )
	{
		TIKI_DEBUG_ERROR( "A object with the given ID already exists." );
		return NULL;
	}

	if( !name.length )
	{
		TIKI_DEBUG_ERROR( "Object needs a name." );
		return NULL;
	}

	if( !structType )
	{
		TIKI_DEBUG_ERROR( "Can't create object no type given." );
		return NULL;
	}
	else if( structType->kind != ObjyTypeKind_Struct )
	{
		TIKI_DEBUG_ERROR( "Can't create object from non struct type." );
		return NULL;
	}

	ObjyObject* parent = NULL;
	if( objyIdIsValid( parentId ) )
	{
		parent = objyObjectStorageFind( &context->objects, parentId );
		if( !parent )
		{
			if( context->rootObject &&
				objyIdIsEqual( parentId, context->rootObject->id ) )
			{
				parent = context->rootObject;
			}
			else
			{
				TIKI_DEBUG_ERROR( "Could not find parent object with id '" OBJY_ID_FORMAT_STRING "' to create child object.", OBJY_ID_FORMAT_DATA( parentId ) );
				return NULL;
			}
		}
	}

	ObjyObject* object;
	const uint64 index = tikiPoolAllocateZero( &context->objects.pool, &object );
	if( index == TIKI_POOL_INVALID_INDEX )
	{
		TIKI_DEBUG_ERROR( "Failed to allocate object." );
		return NULL;
	}

	object->id			= id;
	object->index		= index;
	object->context		= context;
	object->type		= structType;
	object->name		= tikiStringViewAllocateCopy( context->allocator, name );

	if( !object->name.data )
	{
		TIKI_DEBUG_ERROR( "Failed to allocate object name." );
		tikiPoolFree( &context->objects.pool, index );
		return NULL;
	}

	if( !objyObjectAddToParent( object, parent ) )
	{
		tikiPoolFree( &context->objects.pool, index );
		return NULL;
	}

	if( baseValue )
	{
		objyObjectStateContextSet( &context->baseState, object, baseValue, true );
	}

	tikiHashMapInsert( &context->objects.idMap, &object );

	return object;
}

void objyObjectStorageDestroyObject( ObjyContext* context, ObjyObject* object )
{
	if( !object )
	{
		return;
	}

	if( !tikiHashMapRemove( &context->objects.idMap, &object ) )
	{
		TIKI_DEBUG_ERROR( "Object is not part of the given context." );
		return;
	}

	objyObjectRemoveFromParent( object );

	//objyValueDestroy( context, object->rootValue );
	tikiMemoryFree( context->allocator, object->name.data );

	memset( object, 0, sizeof( *object ) );
	tikiPoolFree( &context->objects.pool, object->index );
}

ObjyObject* objyObjectStorageFind( ObjyObjectStorage* objects, ObjyId id )
{
	const ObjyId* mapSearchKey = &id;
	ObjyObject** objectEntry = (ObjyObject**)tikiHashMapFind( &objects->idMap, &mapSearchKey );
	if( !objectEntry )
	{
		return NULL;
	}

	return *objectEntry;
}

bool objyObjectAddToParent( ObjyObject* object, ObjyObject* parent )
{
	TIKI_ASSERT( object->parent == NULL );

	object->parent = parent;

	if( parent )
	{
		// TODO: enable this feature for more types
		if( tikiStringViewIsEqualsStr( parent->type->name, "Folder" ) )
		{
			for( ObjyObject* siblingObject = parent->firstChild; siblingObject != NULL; siblingObject = siblingObject->nextSibling )
			{
				if( !tikiStringViewIsEquals( siblingObject->name, object->name ) )
				{
					continue;
				}

				TIKI_DEBUG_ERROR( "Error: It is not allowed to have two Objects with the same name in a Folder." );
				return false;
			}
		}

		if( parent->lastChild )
		{
			parent->lastChild->nextSibling = object;
			object->prevSibling = parent->lastChild;
			parent->lastChild = object;
		}
		else
		{
			parent->firstChild = object;
			parent->lastChild = object;
		}
	}

	return true;
}

void objyObjectRemoveFromParent( ObjyObject* object )
{
	ObjyObject* oldParent = object->parent;
	if( oldParent )
	{
		if( oldParent->firstChild == object )
		{
			oldParent->firstChild = object->nextSibling;
		}

		if( oldParent->lastChild == object )
		{
			oldParent->lastChild = object->prevSibling;
		}
	}

	if( object->prevSibling )
	{
		object->prevSibling->nextSibling = object->nextSibling;
		object->prevSibling = NULL;
	}

	if( object->nextSibling )
	{
		object->nextSibling->prevSibling = object->prevSibling;
		object->nextSibling = NULL;
	}
}

ObjyObject* objyObjectCreateDetached( ObjyContext* context, ObjyId id, const char* name, const ObjyType* structType )
{
	return objyObjectStorageCreateObject( context, id, tikiStringViewCreateFromPointer( name ), structType, ObjyIdInvalid, NULL );
}

void objyObjectDestroy( ObjyContext* context, ObjyObject* object )
{
	objyObjectStorageDestroyObject( context, object );
}

ObjyBlob objyObjectSerialize( ObjyContext* context, const ObjyObject* object, ObjyObjectFormatter* formatter )
{
	return formatter->serializeFunc( object, formatter->userData );
}

ObjyObject* objyObjectDeserializeDetached( ObjyContext* context, ObjyBlob data, ObjyObjectFormatter* formatter )
{
	const ObjyId id = formatter->blobGetId( data, formatter->userData );
	if( !objyIdIsValid( id ) )
	{
		return NULL;
	}

	ObjyObject* object = objyObjectStorageFind( &context->objects, id );
	if( !object )
	{
		// create?
		return NULL;
	}

	ObjyValue* rootValue = objyObjectStateContextFind( context->currentState, id );
	ObjyValue* newRootValue = objyValueCreateCopy( context, rootValue );
	if( !formatter->deserializeFunc( object, newRootValue, data, formatter->userData ) )
	{
		objyValueDestroy( context, newRootValue );
		return NULL;
	}

	//objyValueDestroy( context, object->rootValue );
	//object->rootValue = newRootValue;
	objyObjectStateContextSet( context->currentState, object, newRootValue, false );

	return object;
}

const ObjyObject* objyObjectFind( ObjyContext* context, ObjyId id )
{
	return objyObjectStorageFind( &context->objects, id );
}

ObjyId objyObjectGetId( const ObjyObject* object )
{
	return object->id;
}

const char* objyObjectGetName( const ObjyObject* object )
{
	return object->name.data;
}

const ObjyType* objyObjectGetType( const ObjyObject* object )
{
	return object->type;
}

ObjyContext* objyObjectGetContext( const ObjyObject* object )
{
	return object->context;
}

const ObjyObject* objyObjectGetParent( const ObjyObject* object )
{
	return object->parent;
}

const ObjyObject* objyObjectGetFirstChild( const ObjyObject* object )
{
	return object->firstChild;
}

const ObjyObject* objyObjectGetPrevSibling( const ObjyObject* object )
{
	return object->prevSibling;
}

const ObjyObject* objyObjectGetNextSibling( const ObjyObject* object )
{
	return object->nextSibling;
}

size_t objyObjectGetChildCount( const ObjyObject* object )
{
	return object->childCount;
}

const ObjyObject* objyObjectFindChildByName( const ObjyObject* object, const char* childName )
{
	const TikiStringView childNameView = tikiStringViewCreateFromPointer( childName );

	for( ObjyObject* child = object->firstChild; child != NULL; child = child->nextSibling )
	{
		if( tikiStringViewIsEquals( childNameView, child->name ) )
		{
			return child;
		}
	}

	return NULL;
}

ObjyValue* objyObjectGetValueWritable( ObjyObject* object )
{
	return objyObjectStateContextFind( object->context->currentState, object->id );
}

const ObjyValue* objyObjectGetValue( const ObjyObject* object )
{
	for( ObjyObjectStateContext* contextState = object->context->currentState; contextState != NULL; contextState = contextState->parentState )
	{
		const ObjyValue* value = objyObjectStateContextFind( contextState, object->id );
		if( !value )
		{
			continue;
		}

		return value;
	}

	return NULL;
}

static ObjyValue* objyObjectFindValueInternal( ObjyValue* value, TikiStringView path )
{
	if( !value || !path.length )
	{
		return value;
	}
	else if( path.data[ 0 ] == '[' )
	{
		if( value->kind != ObjyTypeKind_Array )
		{
			TIKI_DEBUG_ERROR( "Error: objyObjectFindValue: Expected 'Array' value but got '%s' value. Path: %.*s", objyTypeKindGetString( value->kind ), path.length, path.data );
			return NULL;
		}

		const char* arrayClose = memchr( path.data, ']', path.length );
		if( !arrayClose )
		{
			TIKI_DEBUG_ERROR( "Error: objyObjectFindValue: No array close square brackets. Path: %.*s", path.length, path.data );
			return NULL;
		}

		char* numberEnd = NULL;
		const long arrayIndex = strtol( path.data + 1, &numberEnd, 10 );
		if( numberEnd != arrayClose )
		{
			TIKI_DEBUG_ERROR( "Error: objyObjectFindValue: Invalid array index. Path: %.*s", path.length, path.data );
			return NULL;
		}

		if( arrayIndex >= value->data.arr.valueCount )
		{
			TIKI_DEBUG_ERROR( "Error: objyObjectFindValue: Array index(%d) out of range(%d). Path: %.*s", arrayIndex, value->data.arr.valueCount, path.length, path.data );
			return NULL;
		}

		return objyObjectFindValueInternal( value->data.arr.values[ arrayIndex ], tikiStringViewCreateBeginEnd( arrayClose + 1, path.data + path.length ) );
	}
	else if( value->kind == ObjyTypeKind_Reference )
	{
		return objyObjectFindValueInternal( value->data.ref.value, path );
	}
	else if( value->kind != ObjyTypeKind_Struct )
	{
		TIKI_DEBUG_ERROR( "Error: objyObjectFindValue: Expected 'Struct' value but got '%s' value. Path: %.*s", objyTypeKindGetString( value->kind ), path.length, path.data );
		return NULL;
	}

	uintsize endAdd = 1;
	const char* fieldEnd = memchr( path.data, '.', path.length );
	if( !fieldEnd )
	{
		endAdd = 0;
		fieldEnd = path.data + path.length;
	}

	const TikiStringView fieldView = tikiStringViewCreateBeginEnd( path.data, fieldEnd );
	for( size_t i = 0; i < value->data.struc.valueCount; ++i )
	{
		ObjyValueField* field = &value->data.struc.values[ i ];
		if( !tikiStringViewIsEquals( fieldView, field->name ) )
		{
			continue;
		}

		return objyObjectFindValueInternal( field->value, tikiStringViewCreateBeginEnd( fieldEnd + endAdd, path.data + path.length ) );
	}

	TIKI_DEBUG_ERROR( "Error: objyObjectFindValue: struct field '%.*s' not found. Path: %.*s", fieldView.length, fieldView.data, path.length, path.data );
	return NULL;
}

const ObjyValue* objyObjectFindValue( const ObjyObject* object, const char* path )
{
	const TikiStringView pathView = tikiStringViewCreateFromPointer( path );
	const ObjyValue* rootValue = objyObjectGetValue( object );
	return objyObjectFindValueInternal( (ObjyValue*)rootValue, pathView );
}

const ObjyValue* objyObjectFindValueLength( const ObjyObject* object, const char* path, size_t pathLength )
{
	const TikiStringView pathView = tikiStringViewCreate( path, pathLength );
	const ObjyValue* rootValue = objyObjectGetValue( object );
	return objyObjectFindValueInternal( (ObjyValue*)rootValue, pathView );
}

bool objyObjectStateContextConstruct( ObjyObjectStateContext* stateContext, ObjyContext* context, ObjyObjectStateContext* parentState )
{
	if( !tikiHashMapConstructSize( &stateContext->idMap, context->allocator, sizeof( ObjyObjectState* ), objyObjectStateIdMapHash, objyObjectStateIdMapIsKeyEquals, 16u ) )
	{
		return false;
	}

	stateContext->context		= context;
	stateContext->parentState	= parentState;

	return true;

}
void objyObjectStateContextDestruct( ObjyObjectStateContext* stateContext )
{
	for( uintsize i = tikiHashMapFindFirstIndex( &stateContext->idMap ); i != TIKI_HASH_MAP_INVALID_INDEX; i = tikiHashMapFindNextIndex( &stateContext->idMap, i ) )
	{
		ObjyObjectState* objectState = *(ObjyObjectState**)tikiHashMapGetEntry( &stateContext->idMap, i );

		if( objectState->isNewObject )
		{
			objyObjectStorageDestroyObject( stateContext->context, objectState->object );
			objectState->object = NULL;
		}

		objyValueStorageFree( &stateContext->context->values, objectState->value );
		objectState->value = NULL;

		tikiPoolFree( &stateContext->context->statePool, objectState->index );
	}

	tikiHashMapDestruct( &stateContext->idMap );

	stateContext->parentState	= NULL;
	stateContext->context		= NULL;
}

ObjyValue* objyObjectStateContextFind( ObjyObjectStateContext* stateContext, ObjyId id )
{
	const ObjyId* mapSearchKey = &id;
	ObjyObjectState** objectState = (ObjyObjectState**)tikiHashMapFind( &stateContext->idMap, &mapSearchKey );
	if( objectState )
	{
		return (*objectState)->value;
	}

	if( stateContext->parentState )
	{
		return objyObjectStateContextFind( stateContext->parentState, id );
	}

	return NULL;
}

ObjyValue* objyObjectStateContextFindOrCreate( ObjyObjectStateContext* stateContext, ObjyObject* object )
{
	ObjyValue* value = objyObjectStateContextFind( stateContext, object->id );
	if( value )
	{
		return value;
	}

	value = objyValueCreateStruct( stateContext->context );
	if( !objyObjectStateContextSet( stateContext, object, value, false ) )
	{
		objyValueDestroy( stateContext->context, value );
		return NULL;
	}

	return value;
}

bool objyObjectStateContextSet( ObjyObjectStateContext* stateContext, ObjyObject* object, ObjyValue* value, bool isNewObject )
{
	bool isNew;
	ObjyObjectState** mapObjectState = tikiHashMapInsertNew( &stateContext->idMap, &object, &isNew );
	if( !mapObjectState )
	{
		return false;
	}

	ObjyObjectState* objectState;
	if( !isNew )
	{
		objectState = *mapObjectState;
		if( objectState->value )
		{
			objyValueStorageFree( &stateContext->context->values, objectState->value );
		}

		objectState->value = value;
		return true;
	}

	const uint64 index = tikiPoolAllocateZero( &stateContext->context->statePool, &objectState );
	if( index == TIKI_POOL_INVALID_INDEX )
	{
		return false;
	}

	objectState->id				= object->id;
	objectState->index			= index;
	objectState->object			= object;
	objectState->isNewObject	= isNewObject;
	objectState->value			= value;

	*mapObjectState = objectState;
	return true;
}

static TikiHash objyObjectIdMapHash( const void* entry )
{
	const ObjyObject* object = *(const ObjyObject**)entry;
	return tikiHashMurmur3( &object->id, sizeof( object->id ) );
}

static bool objyObjectIdMapIsKeyEquals( const void* lhs, const void* rhs )
{
	const ObjyObject* lhsObject = *(const ObjyObject**)lhs;
	const ObjyObject* rhsObject = *(const ObjyObject**)rhs;
	return memcmp( &lhsObject->id, &rhsObject->id, sizeof( lhsObject->id ) ) == 0;
}

static TikiHash objyObjectStateIdMapHash( const void* entry )
{
	const ObjyObjectState* objectState = *(const ObjyObjectState**)entry;
	return tikiHashMurmur3( &objectState->id, sizeof( objectState->id ) );
}

static bool objyObjectStateIdMapIsKeyEquals( const void* lhs, const void* rhs )
{
	const ObjyObjectState* lhsState = *(const ObjyObjectState**)lhs;
	const ObjyObjectState* rhsState = *(const ObjyObjectState**)rhs;
	return memcmp( &lhsState->id, &rhsState->id, sizeof( lhsState->id ) ) == 0;
}
