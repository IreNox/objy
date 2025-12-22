#include "objy_object.h"

#include "objy_internal.h"

#include <stdlib.h>

static TikiHash		objyObjectIdMapHash( const void* entry );
static bool			objyObjectIdMapIsKeyEquals( const void* lhs, const void* rhs );

static ObjyValue*	objyObjectFindValueInternal( ObjyValue* value, TikiStringView path );

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

ObjyObject* objyObjectStorageCreateObject( ObjyContext* context, ObjyId id, TikiStringView name, const ObjyType* structType, ObjyId parentId )
{
	if( !objyIdIsValid( id ) )
	{
		TIKI_DEBUG_ERROR( "Invalid ID to create object." );
		return NULL;
	}
	else if( tikiHashMapFind( &context->objects.idMap, &id ) )
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

	ObjyObject* parent = objyObjectStorageFind( &context->objects, parentId );
	if( objyIdIsValid( parentId ) )
	{
		parent = objyObjectStorageFind( &context->objects, parentId );
		if( !parent )
		{
			TIKI_DEBUG_ERROR( "Could not find parent object with id '" OBJY_ID_FORMAT_STRING "' to create child object.", OBJY_ID_FORMAT_DATA( parentId ) );
			return NULL;
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
	object->rootValue	= objyValueCreateStruct( context, structType );

	if( !object->name.data )
	{
		TIKI_DEBUG_ERROR( "Failed to allocate object name." );
		tikiPoolFree( &context->objects.pool, index );
		return NULL;
	}

	objyObjectAddToParent( object, parent );

	tikiHashMapInsert( &context->objects.idMap, object );

	return object;
}

void objyObjectStorageDestroyObject( ObjyContext* context, ObjyObject* object )
{
	if( !object )
	{
		return;
	}

	tikiHashMapRemove( &context->objects.idMap, object );

	objyObjectRemoveFromParent( object );

	objyValueDestroy( context, object->rootValue );
	tikiMemoryFree( context->allocator, object->name.data );

	memset( object, 0, sizeof( *object ) );
	tikiPoolFree( &context->objects.pool, object->index );
}

ObjyObject* objyObjectStorageFind( ObjyObjectStorage* objects, ObjyId id )
{
	ObjyObject** objectEntry = (ObjyObject**)tikiHashMapFind( &objects->idMap, &id );
	if( !objectEntry )
	{
		return NULL;
	}

	return *objectEntry;
}

ObjyObject*	objyObjectAddToParent( ObjyObject* object, ObjyObject* parent )
{
	TIKI_ASSERT( object->parent == NULL );

	object->parent = parent;

	if( parent )
	{
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
}

ObjyObject*	objyObjectRemoveFromParent( ObjyObject* object )
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
	return objyObjectStorageCreateObject( context, id, tikiStringViewCreateFromPointer( name ), structType, ObjyIdInvalid );
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

	ObjyValue* newRootValue = objyValueCreateCopy( context, object->rootValue );
	if( !formatter->deserializeFunc( object, newRootValue, data, formatter->userData ) )
	{
		objyValueDestroy( context, newRootValue );
		return NULL;
	}

	objyValueDestroy( context, object->rootValue );
	object->rootValue = newRootValue;

	return object;
}

const ObjyObject* objyObjectFind( ObjyContext* context, ObjyId id )
{
	const ObjyObject** objectEntry = (const ObjyObject**)tikiHashMapFind( &context->objects.idMap, &id );
	if( !objectEntry )
	{
		return NULL;
	}

	return *objectEntry;
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
	return object->rootValue;
}

const ObjyValue* objyObjectGetValue( const ObjyObject* object )
{
	for( ObjyContextState* contextState = object->context->currentState; contextState != NULL; contextState = contextState->parentState )
	{
		const ObjyObjectState** objectState = tikiHashMapFind( &contextState->idMap, &object->id );
		if( !objectState )
		{
			continue;
		}

		return (*objectState)->value;
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
		return objyObjectFindValueInternal( value->data.ref, path );
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

	TIKI_DEBUG_ERROR( "Error: objyObjectFindValue: Struct field '%.*s' not found. Path: %.*s", fieldView.length, fieldView.data, path.length, path.data );
	return NULL;
}

const ObjyValue* objyObjectFindValue( const ObjyObject* object, const char* path )
{
	const TikiStringView pathView = tikiStringViewCreateFromPointer( path );
	return objyObjectFindValueInternal( object->rootValue, pathView );
}

const ObjyValue* objyObjectFindValueLength( const ObjyObject* object, const char* path, size_t pathLength )
{
	const TikiStringView pathView = tikiStringViewCreate( path, pathLength );
	return objyObjectFindValueInternal( object->rootValue, pathView );
}

static TikiHash objyObjectIdMapHash( const void* entry )
{
	const ObjyObject* object = (const ObjyObject*)entry;
	return tikiHashMurmur3( &object->id, sizeof( object->id ) );
}

static bool objyObjectIdMapIsKeyEquals( const void* lhs, const void* rhs )
{
	const ObjyObject* lhsObject = (const ObjyObject*)lhs;
	const ObjyObject* rhsObject = (const ObjyObject*)rhs;
	return memcmp( &lhsObject->id, &rhsObject->id, sizeof( lhsObject->id ) ) == 0;
}
