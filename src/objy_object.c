#include "objy_object.h"

#include "objy_internal.h"

#include <string.h>

static TikiHash	objyObjectIdMapHash( const void* entry );
static bool		objyObjectIdMapIsKeyEquals( const void* lhs, const void* rhs );

static ObjyObject*	objyObjectFindInternal( ObjyContext* context, ObjyId id );

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

ObjyObject* objyObjectCreateDetached( ObjyContext* context, ObjyId id, const char* name, ObjyType* structType )
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

	if( !name )
	{
		TIKI_DEBUG_ERROR( "Object needs a name." );
		return NULL;
	}

	if( !structType || structType->kind != ObjyTypeKind_Struct )
	{
		TIKI_DEBUG_ERROR( "Invalid type to create object." );
		return NULL;
	}

	ObjyObject* object;
	const uint64 index = tikiPoolAllocateZero( &context->objects.pool, &object );
	if( index == TIKI_POOL_INVALID_INDEX )
	{
		TIKI_DEBUG_ERROR( "Failed to allocate object." );
		return NULL;
	}

	const uintsize nameLength = strlen( name );
	char* nameCopy = (char*)tikiMemoryAlloc( context->allocator, nameLength + 1 );
	if( !nameCopy )
	{
		TIKI_DEBUG_ERROR( "Failed to allocate object name." );
		tikiPoolFree( &context->objects.pool, index );
		return NULL;
	}

	strcpy( nameCopy, name );

	object->id			= id;
	object->index		= index;
	object->context		= context;
	//object->parent		= parent;
	object->type		= structType;
	object->name.data	= nameCopy;
	object->name.length	= nameLength;
	object->rootValue	= objyValueCreateStruct( context, structType );

	// add to parent

	tikiHashMapInsert( &context->objects.idMap, object );

	return object;
}

void objyObjectDestroy( ObjyContext* context, ObjyObject* object )
{
	if( !object )
	{
		return;
	}

	tikiHashMapRemove( &context->objects.idMap, object );

	// remove from parent

	objyValueDestroy( context, object->rootValue );
	tikiMemoryFree( context->allocator, object->name.data );

	memset( object, 0, sizeof( *object ) );
	tikiPoolFree( &context->objects.pool, object->index );
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

	ObjyObject* object = objyObjectFindInternal( context, id );
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

static ObjyObject* objyObjectFindInternal( ObjyContext* context, ObjyId id )
{
	return (ObjyObject*)tikiHashMapFind( &context->objects.idMap, &id );
}

const ObjyObject* objyObjectFind( ObjyContext* context, ObjyId id )
{
	return (const ObjyObject*)tikiHashMapFind( &context->objects.idMap, &id );
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

ObjyValue* objyObjectGetValueWritable( ObjyObject* object )
{
	return object->rootValue;
}

const ObjyValue* objyObjectGetValue( const ObjyObject* object )
{
	return object->rootValue;
}

const ObjyValue* objyObjectFindValue( const ObjyObject* object, const char* path );

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
