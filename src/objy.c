#include "objy/objy.h"

#include "objy_internal.h"

#include "tiki_memory.h"

#include <string.h>

#ifdef _WIN32
#	include <Objbase.h>
#endif

static bool			objySystemCreateBaseTypes( ObjySystem* system );
static ObjyType*	objySystemCreateBaseType( ObjySystem* system, ObjyId id, const char* name, ObjyTypeKind kind, ObjyValue* baseValue );
static ObjyType*	objySystemCreateBaseValueType( ObjySystem* system, ObjyId id, const char* name, ObjyTypeKind kind, uint8 bitCount, bool signedInt );
static ObjyObject*	objySystemCreateBaseTypeField( ObjySystem* system, const ObjyType* parentType, ObjyId id, const char* name, const ObjyType* baseType );

ObjySystem* objyCreate( const ObjyParameters* parameters )
{
	TikiAllocator allocator;
	tikiMemoryAllocatorPrepare( &allocator, parameters->allocator.mallocFunc, parameters->allocator.reallocFunc, parameters->allocator.freeFunc, parameters->allocator.userData );

	ObjySystem* system = TIKI_MEMORY_NEW_ZERO( &allocator, ObjySystem );

	tikiMemoryAllocatorFinalize( &system->allocator, &allocator );
	tikiPoolConstruct( &system->contextPool, &system->allocator, sizeof( ObjyContext ), OBJY_CONTEXT_CHUNK_SIZE );

	if( !objyTypeCollectionConstruct( &system->types, &system->allocator ) )
	{
		objyDestroy( system );
		return NULL;
	}

	system->rootContext = objyContextCreate( system, NULL );

	if( !objySystemCreateBaseTypes( system ) )
	{
		objyDestroy( system );
		return NULL;
	}

	return system;
}

static bool objySystemCreateBaseTypes( ObjySystem* system )
{
	system->typeFolder	= objyTypeCollectionCreate( &system->types, "ObjyFolder", ObjyTypeKind_Struct, system );
	system->typeType	= objyTypeCollectionCreate( &system->types, "ObjyType", ObjyTypeKind_Struct, system );
	if( !system->typeFolder || !system->typeType )
	{
		objyDestroy( system );
		return false;
	}

	ObjyChangeSetResult result;
	system->rootContext->rootObject = objyObjectStorageCreateObject( system->rootContext, ObjyIdRoot, tikiStringViewCreateFromPointer( "ObjyRoot" ), system->typeFolder, ObjyIdInvalid, NULL, &result );

	system->typesFolderObject	= objyObjectStorageCreateObject( system->rootContext, ObjyIdTypesFolder, tikiStringViewCreateFromPointer( "Types" ), system->typeFolder, ObjyIdRoot, NULL, &result );
	system->typeFolder->object	= objyObjectStorageCreateObject( system->rootContext, ObjyIdFolder, system->typeFolder->name, system->typeType, ObjyIdTypesFolder, NULL, &result );
	system->typeType->object	= objyObjectStorageCreateObject( system->rootContext, ObjyIdType, system->typeType->name, system->typeType, ObjyIdTypesFolder, NULL, &result );
	if( !system->typesFolderObject || !system->typeFolder->object || !system->typeType->object )
	{
		objyDestroy( system );
		return false;
	}

	ObjyValue* typeRefValue = objyValueCreateStruct( system->rootContext );
	TIKI_VERIFY( objyValueWriteStructField( system->rootContext, typeRefValue, "BaseType", objyValueCreateId( system->rootContext, ObjyIdType ) ) );

	system->typeKind	= objySystemCreateBaseType( system, ObjyIdTypeKind, "ObjyTypeKind", ObjyTypeKind_Enum, NULL );
	system->typeField	= objySystemCreateBaseType( system, ObjyIdTypeField, "ObjyTypeField", ObjyTypeKind_Struct, NULL );
	system->typeTypeRef	= objySystemCreateBaseType( system, ObjyIdTypeRef, "ObjyTypeRef", ObjyTypeKind_Reference, typeRefValue );
	system->typeBool	= objySystemCreateBaseValueType( system, ObjyIdTypeBool, "Bool", ObjyTypeKind_Bool, 0, false );
	system->typeUInt8	= objySystemCreateBaseValueType( system, ObjyIdTypeUInt8, "UInt8", ObjyTypeKind_Integer, 8, false );
	system->typeUInt16	= objySystemCreateBaseValueType( system, ObjyIdTypeUInt16, "UInt16", ObjyTypeKind_Integer, 16, false );
	system->typeUInt32	= objySystemCreateBaseValueType( system, ObjyIdTypeUInt32, "UInt32", ObjyTypeKind_Integer, 32, false );
	system->typeUInt64	= objySystemCreateBaseValueType( system, ObjyIdTypeUInt64, "UInt64", ObjyTypeKind_Integer, 64, false );
	system->typeSInt8	= objySystemCreateBaseValueType( system, ObjyIdTypeSInt8, "SInt8", ObjyTypeKind_Integer, 8, true );
	system->typeSInt16	= objySystemCreateBaseValueType( system, ObjyIdTypeSInt16, "SInt16", ObjyTypeKind_Integer, 16, true );
	system->typeSInt32	= objySystemCreateBaseValueType( system, ObjyIdTypeSInt32, "SInt32", ObjyTypeKind_Integer, 32, true );
	system->typeSInt64	= objySystemCreateBaseValueType( system, ObjyIdTypeSInt64, "SInt64", ObjyTypeKind_Integer, 64, true );
	system->typeFloat	= objySystemCreateBaseValueType( system, ObjyIdTypeFloat, "Float", ObjyTypeKind_Integer, 32, true );
	system->typeDouble	= objySystemCreateBaseValueType( system, ObjyIdTypeDouble, "Double", ObjyTypeKind_Integer, 64, true );
	if( !system->typeFolder || !system->typeKind  || !system->typeField || !system->typeTypeRef )
	{
		objyDestroy( system );
		return false;
	}

	system->typeTypeRef->baseType = system->typeType;

	for( uintsize i = 0; i < TIKI_ARRAY_COUNT( ObjyIdObjyTypeKindValues ); ++i )
	{
		system->typeKindObjects[ i ] = objyObjectStorageCreateObject( system->rootContext, ObjyIdObjyTypeKindValues[ i ], tikiStringViewCreateFromPointer( objyTypeKindGetString( i ) ), system->typeField, ObjyIdTypeField, NULL, &result );
	}

	const ObjyTypeField typeTypeFields[] =
	{
		{ "Kind",			system->typeKind },
		{ "BaseType",		system->typeTypeRef },
		{ "ValueBitCount",	system->typeUInt8 },
		{ "SignedInteger",	system->typeBool },
		{ "Version",		system->typeUInt64 },
	};
	objyTypeCollectionSetFields( &system->types, system->typeType, typeTypeFields, TIKI_ARRAY_COUNT( typeTypeFields ) );

	const ObjyTypeField typeFieldFields[] =
	{
		{ "Type",			system->typeTypeRef },
	};
	objyTypeCollectionSetFields( &system->types, system->typeField, typeFieldFields, TIKI_ARRAY_COUNT( typeFieldFields ) );

	system->typeTypeKindObject			= objySystemCreateBaseTypeField( system, system->typeType, ObjyIdObjyTypeKind,			"Kind", system->typeKind );
	system->typeTypeBaseTypeObject		= objySystemCreateBaseTypeField( system, system->typeType, ObjyIdObjyTypeBaseType,		"BaseType", system->typeTypeRef );
	system->typeTypeValueBitCountObject	= objySystemCreateBaseTypeField( system, system->typeType, ObjyIdObjyTypeValueBitCount, "ValueBitCount", system->typeUInt8 );
	system->typeTypeSignedIntegerObject	= objySystemCreateBaseTypeField( system, system->typeType, ObjyIdObjyTypeSignedInteger, "SignedInteger", system->typeBool );
	system->typeTypeVersionObject		= objySystemCreateBaseTypeField( system, system->typeType, ObjyIdObjyTypeSignedInteger, "Version", system->typeUInt64 );

	system->typeFieldTypebject			= objySystemCreateBaseTypeField( system, system->typeType, ObjyIdObjyTypeFieldType, "Type", system->typeTypeRef );

	//ObjyObject* typesObject = objyObjectStorageFind( &system->rootContext->objects, ObjyIdFolderTypes );
	//if( !typesObject )
	//{
	//
	//}

	return true;
}

static ObjyType* objySystemCreateBaseType( ObjySystem* system, ObjyId id, const char* name, ObjyTypeKind kind, ObjyValue* baseValue )
{
	if( !baseValue )
	{
		baseValue = objyValueCreateStruct( system->rootContext );
	}

	TIKI_VERIFY( objyValueWriteStructField( system->rootContext, baseValue, "Kind", objyValueCreateId( system->rootContext, ObjyIdObjyTypeKindValues[ kind ] ) ) );

	ObjyType* type = objyTypeCollectionCreate( &system->types, name, kind, system );
	ObjyChangeSetResult result;
	type->object = objyObjectStorageCreateObject( system->rootContext, id, tikiStringViewCreateFromPointer( name ), system->typeType, ObjyIdTypesFolder, baseValue, &result );
	TIKI_ASSERT( type->object != NULL );

	return type;
}

static ObjyType* objySystemCreateBaseValueType( ObjySystem* system, ObjyId id, const char* name, ObjyTypeKind kind, uint8 bitCount, bool signedInt )
{
	ObjyValue* typeValue = objyValueCreateStruct( system->rootContext );

	if( kind == ObjyTypeKind_Integer || kind == ObjyTypeKind_Float )
	{
		TIKI_VERIFY( objyValueWriteStructField( system->rootContext, typeValue, "ValueBitCount", objyValueCreateUInt( system->rootContext, bitCount ) ) );
	}

	if( kind == ObjyTypeKind_Integer )
	{
		TIKI_VERIFY( objyValueWriteStructField( system->rootContext, typeValue, "SignedInteger", objyValueCreateBool( system->rootContext, signedInt ) ) );
	}

	ObjyType* type = objySystemCreateBaseType( system, id, name, kind, typeValue );
	type->bitCount	= bitCount;
	type->signedInt	= signedInt;

	return type;
}

static ObjyObject*	objySystemCreateBaseTypeField( ObjySystem* system, const ObjyType* parentType, ObjyId id, const char* name, const ObjyType* baseType )
{
	ObjyValue* typeFieldValue = objyValueCreateStruct( system->rootContext );
	TIKI_VERIFY( objyValueWriteStructField( system->rootContext, typeFieldValue, "Type", objyValueCreateId( system->rootContext, baseType->object->id ) ) );

	ObjyChangeSetResult result;
	return objyObjectStorageCreateObject( system->rootContext, id, tikiStringViewCreateFromPointer( name ), system->typeField, parentType->object->id, typeFieldValue, &result );
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
	context->rootObject	= parentObject;

	if( !objyObjectStorageConstruct( &context->objects, context->allocator ) )
	{
		objyContextDestroy( context );
		return NULL;
	}

	objyValueStorageConstruct( &context->values, context->allocator );
	objyChangeStorageConstruct( &context->changes, context->allocator );

	tikiPoolConstruct( &context->statePool, context->allocator, sizeof( ObjyObjectState ), OBJY_OBJECT_STATE_CHUNK_SIZE );
	objyObjectStateContextConstruct( &context->baseState, context, NULL );
	context->currentState = &context->baseState;

	return context;
}

void objyContextDestroy( ObjyContext* context )
{
	if( !context )
	{
		return;
	}

	objyChangeStorageDestroyChangeSets( &context->changes );

	objyObjectStateContextDestruct( &context->baseState );
	tikiPoolDestruct( &context->statePool );

	objyObjectStorageDestruct( &context->objects );
	objyValueStorageDestruct( &context->values );
	objyChangeStorageDestruct( &context->changes );

	tikiPoolFree( &context->system->contextPool, context->index );
}

const ObjyObject* objyContextGetFirstObject( const ObjyContext* context )
{
	return context->rootObject->firstChild;
}

bool objyIdIsValid( ObjyId id )
{
	return memcmp( &id, &ObjyIdInvalid, sizeof( id ) ) != 0;
}

bool objyIdIsEqual( ObjyId lhs, ObjyId rhs )
{
	return memcmp( &lhs, &rhs, sizeof( lhs ) ) == 0;
}

void objyIdCreate( ObjyId* id )
{
#ifdef _WIN32
	GUID guid;
	TIKI_VERIFY( SUCCEEDED( CoCreateGuid( &guid ) )  );
	memcpy( id, &guid, sizeof( *id ) );
#else
#	error Platform not implemented
#endif
}