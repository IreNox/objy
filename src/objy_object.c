#include "objy_object.h"

#include "objy_internal.h"

#include <string.h>

static ObjyObject*	objyObjectFindInternal( ObjyContext* context, ObjyId id );

ObjyObject* objyObjectCreateDetached( ObjyContext* context, ObjyId id, const char* name, ObjyType* structType/*, ObjyObject* parent*/ )
{
	if( !objyIdIsValid( id ) )
	{
		TIKI_DEBUG_ERROR( "Invalid ID to create object." );
		return NULL;
	}
	else if( tikiHashMapFind( &context->objectIdMap, &id ) )
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
	const uint64 index = tikiPoolAllocateZero( &context->objectPool, &object );
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
		tikiPoolFree( &context->objectPool, index );
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

	tikiHashMapInsert( &context->objectIdMap, object );

	return object;
}

void objyObjectDestroy( ObjyContext* context, ObjyObject* object )
{
	if( !object )
	{
		return;
	}

	tikiHashMapRemove( &context->objectIdMap, object );

	// remove from parent

	objyValueDestroy( context, object->rootValue );
	tikiMemoryFree( context->allocator, object->name.data );

	memset( object, 0, sizeof( *object ) );
	tikiPoolFree( &context->objectPool, object->index );
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
	return (ObjyObject*)tikiHashMapFind( &context->objectIdMap, &id );
}

const ObjyObject* objyObjectFind( ObjyContext* context, ObjyId id )
{
	return (const ObjyObject*)tikiHashMapFind( &context->objectIdMap, &id );
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

//ObjyObject* objyObjectGetChildren( ObjyObject* object )
//{
//
//}
//
//size_t					objyObjectGetChildCount( ObjyObject* object );
//ObjyObject*				objyObjectFindChildByName( ObjyObject* object, const char* childName );

const ObjyValue* objyObjectGetValue( const ObjyObject* object )
{
	return object->rootValue;
}

const ObjyValue* objyObjectFindValue( const ObjyObject* object, const char* path );