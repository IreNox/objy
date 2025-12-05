#include "objy/objy.h"

#include "objy_internal.h"

#include "tiki_memory.h"

#include <string.h>

ObjySystem* objyCreate( const ObjyParameters* parameters )
{
	TikiAllocator allocator;
	tikiMemoryAllocatorPrepare( &allocator, parameters->allocator.mallocFunc, parameters->allocator.reallocFunc, parameters->allocator.freeFunc, parameters->allocator.userData );

	ObjySystem* system = TIKI_MEMORY_NEW_ZERO( &allocator, ObjySystem );

	tikiMemoryAllocatorFinalize( &system->allocator, &allocator );

	system->rootContext = objyContextCreate( system, NULL );

	return system;
}

void objyDestroy( ObjySystem* system )
{
	if( !system )
	{
		return;
	}

	objyContextDestroy( system->rootContext );
	system->rootContext = NULL;

	objyTypeCollectionDestruct( &system->types );

	tikiMemoryFree( &system->allocator, system );
}

ObjyContext* objyGetRootContext( ObjySystem* system )
{
	return system->rootContext;
}

ObjyContext* objyGetContext( ObjyObject* object )
{
	return object->context;
}

ObjyContext* objyContextCreate( ObjySystem* system, ObjyObject* parentObject )
{
	ObjyContext* context;
	const uint64 index = tikiPoolAllocateZero( &system->contextPool, &context );
	if( index == TIKI_POOL_INVALID_INDEX )
	{
		return NULL;
	}

	context->allocator	= &system->allocator;
	context->system		= system;
	context->index		= index;

	if( !objyObjectStorageConstruct( &context->objects, context->allocator ) )
	{
		objyContextDestroy( context );
		return NULL;
	}

	objyValueStorageConstruct( &context->values, context->allocator );
	objyChangeStorageConstruct( &context->changes, context->allocator );

	return context;
}

void objyContextDestroy( ObjyContext* context )
{
	if( !context )
	{
		return;
	}

	objyObjectStorageDestruct( &context->objects );
	objyValueStorageDestruct( &context->values );
	objyChangeStorageDestruct( &context->changes );

	tikiPoolFree( &context->system->contextPool, context->index );
}

bool objyIdIsValid( ObjyId id )
{
	return memcmp( &id, &ObjyIdInvalid, sizeof( id ) ) != 0;
}
