#include "objy/objy.h"

#include "objy_internal.h"

#include "tiki_memory.h"

#include <string.h>

//static const ObjyId InvalidObjectId = { 0, 0, 0, { 0, 0, 0, 0 } };

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
	return NULL;
}

void objyContextDestroy( ObjyContext* context )
{

}

bool objyIdIsValid( ObjyId id )
{
	return memcmp( &id, &InvalidObjectId, sizeof( id ) ) != 0;
}
