#include "objy_type.h"

#include <string.h>

static TikiHash		objyTypeHash( const void* entry );
static bool			objyTypeEquals( const void* lhs, const void* rhs );

static ObjyType*	objyTypeCollectionCreateInternal( ObjyTypeCollection* types, const char* name, ObjyTypeKind kind, void* userData );
static void			objyTypeDestroyInternal( ObjyTypeCollection* types, ObjyType* type );

bool objyTypeCollectionConstruct( ObjyTypeCollection* types, TikiAllocator* allocator )
{
	types->allocator	= allocator;
	types->firstType	= NULL;
	types->lastType	= NULL;

	if( !tikiHashMapConstructSize( &types->nameMap, types->allocator, sizeof( ObjyType* ), objyTypeHash, objyTypeEquals, 64u ) ||
		!tikiStringPoolConstruct( &types->stringPool, types->allocator ) )
	{
		objyTypeCollectionDestruct( types );
		return false;
	}

	tikiPoolConstruct( &types->pool, types->allocator, sizeof( ObjyType ), OBJY_TYPE_CHUNK_SIZE );

	return true;
}

void objyTypeCollectionDestruct( ObjyTypeCollection* types )
{
	while( types->firstType )
	{
		objyTypeDestroyInternal( types, types->firstType );
	}

	tikiHashMapDestruct( &types->nameMap );
	tikiPoolDestruct( &types->pool );
}

const ObjyType* objyTypeCollectionFind( ObjyTypeCollection* types, const char* name )
{
	const TikiStringView nameString = { name, strlen( name ) };
	const ObjyType** type = (const ObjyType**)tikiHashMapFind( &types->nameMap, &name );
	if( !type )
	{
		return NULL;
	}

	return *type;
}

static ObjyType* objyTypeCollectionCreateInternal( ObjyTypeCollection* types, const char* name, ObjyTypeKind kind, void* userData )
{
	const TikiStringView pooledName = tikiStringPoolAddPointer( &types->stringPool, name );
	if( tikiHashMapFind( &types->nameMap, &pooledName ) )
	{
		TIKI_DEBUG_ERROR( "Duplicate type name: %s", name );
		return NULL;
	}

	ObjyType* type;
	const uint64 typeIndex = tikiPoolAllocateZero( &types->pool, (void**)&type );
	if( typeIndex == TIKI_POOL_INVALID_INDEX )
	{
		return NULL;
	}

	type->name		= pooledName;
	type->index		= typeIndex;
	type->kind		= kind;
	type->userData	= userData;

	if( !types->firstType )
	{
		types->firstType = type;
	}
	else
	{
		types->lastType->nextType = type;
	}

	type->prevType = types->lastType;
	types->lastType = type;

	tikiHashMapInsert( &types->nameMap, type );
	return type;
}

ObjyType* objyTypeCreateValue( ObjySystem* system, const char* name, ObjyTypeKind kind, size_t bitCount, bool signedInt, void* userData )
{
	ObjyType* type = objyTypeCollectionCreateInternal( &system->types, name, kind, userData );
	if( !type )
	{
		return NULL;
	}

	type->bitCount	= (uint8)bitCount;
	type->signedInt	= signedInt;

	return type;
}

ObjyType* objyTypeCreateStruct( ObjySystem* system, const char* name, const ObjyTypeField* fields, size_t fieldCount, void* userData )
{
	return objyTypeCreateStructInherited( system, name, NULL, fields, fieldCount, userData );
}

ObjyType* objyTypeCreateStructInherited( ObjySystem* system, const char* name, ObjyType* baseType, const ObjyTypeField* fields, size_t fieldCount, void* userData )
{
	ObjyType* type = objyTypeCollectionCreateInternal( &system->types, name, ObjyTypeKind_Struct, userData );
	if( !type )
	{
		return NULL;
	}

	type->baseType = baseType;

	const uintsize totalFieldCount = fieldCount + (baseType ? baseType->globalFieldCount : 0);
	ObjyTypeField* newFields = TIKI_MEMORY_ARRAY_NEW( &system->allocator, ObjyTypeField, totalFieldCount );

	if( baseType )
	{
		memcpy( newFields, baseType->globalFields, sizeof( *fields ) * baseType->globalFieldCount );
		memcpy( newFields + baseType->globalFieldCount, fields, sizeof( *fields ) * fieldCount );

		type->localFields		= newFields + baseType->globalFieldCount;
		type->localFieldCount	= fieldCount;
		type->globalFields		= newFields;
		type->globalFieldCount	= baseType->globalFieldCount + fieldCount;
	}
	else
	{
		memcpy( newFields, fields, sizeof( *fields ) * fieldCount );

		type->localFields		= newFields;
		type->localFieldCount	= fieldCount;
		type->globalFields		= newFields;
		type->globalFieldCount	= fieldCount;
	}

	for( uintsize i = 0; i < type->localFieldCount; ++i )
	{
		ObjyTypeField* field = &type->localFields[ i ];

		const TikiStringView fieldNameString = tikiStringPoolAddPointer( &system->types.stringPool, field->name );
		field->name = fieldNameString.data;
	}

	return type;
}

ObjyType* objyTypeCreateArray( ObjySystem* system, ObjyType* baseType, void* userData )
{
	if( !baseType )
	{
		return NULL;
	}

	const uintsize nameLength = baseType->name.length + 3u;
	char* name = alloca( nameLength );
	memcpy( name, baseType->name.data, baseType->name.length );
	strcpy( name + baseType->name.length, "[]" );

	ObjyType* type = objyTypeCollectionCreateInternal( &system->types, name, ObjyTypeKind_Array, userData );
	if( !type )
	{
		return NULL;
	}

	type->baseType = baseType;

	return type;
}

ObjyType* objyTypeCreateReference( ObjySystem* system, ObjyType* baseType, void* userData )
{
	if( !baseType )
	{
		return NULL;
	}

	const uintsize nameLength = baseType->name.length + 2u;
	char* name = alloca( nameLength );
	memcpy( name, baseType->name.data, baseType->name.length );
	strcpy( name + baseType->name.length, "*" );

	ObjyType* type = objyTypeCollectionCreateInternal( &system->types, name, ObjyTypeKind_Reference, userData );
	if( !type )
	{
		return NULL;
	}

	type->baseType = baseType;

	return type;
}

void objyTypeDestroy( ObjySystem* system, ObjyType* type )
{
	objyTypeDestroyInternal( &system->types, type );
}

static void objyTypeDestroyInternal( ObjyTypeCollection* types, ObjyType* type )
{
	if( type->globalFields )
	{
		tikiMemoryFree( types->allocator, type->globalFields );
	}

	if( type == types->firstType )
	{
		types->firstType = type->nextType;
	}

	if( type == types->lastType )
	{
		types->lastType = type->prevType;
	}

	if( type->nextType )
	{
		type->nextType->prevType = type->prevType;
	}

	if( type->prevType )
	{
		type->prevType->nextType = type->nextType;
	}

	tikiPoolFree( &types->pool, type->index );
}

const ObjyType* objyTypeFind( ObjySystem* system, const char* name )
{
	return objyTypeCollectionFind( &system->types, name );
}

const char* objyTypeGetName( const ObjyType* type )
{
	return type->name.data;
}

ObjyTypeKind objyTypeGetKind( const ObjyType* type )
{
	return type->kind;
}

ObjyType* objyTypeGetBaseType( const ObjyType* type )
{
	return type->baseType;
}

void* objyTypeGetUserData( const ObjyType* type )
{
	return type->userData;
}

const ObjyTypeField* objyTypeGetStructLocalFields( const ObjyType* type )
{
	return type->localFields;
}

size_t objyTypeGetStructLocalFieldCount( const ObjyType* type )
{
	return type->localFieldCount;
}

const ObjyTypeField* objyTypeGetStructGlobalFields( const ObjyType* type )
{
	return type->globalFields;
}

size_t objyTypeGetStructGlobalFieldCount( const ObjyType* type )
{
	return type->globalFieldCount;
}

size_t objyTypeGetValueBitCount( const ObjyType* type )
{
	return type->bitCount;
}

static TikiHash objyTypeHash( const void* entry )
{
	const ObjyType* type = *(const ObjyType**)entry;
	return tikiHashMurmur3( type->name.data, type->name.length );
}

static bool objyTypeEquals( const void* lhs, const void* rhs )
{
	const TikiStringView* lhsName = (const TikiStringView*)lhs;
	const TikiStringView* rhsName = (const TikiStringView*)rhs;

	if( lhsName->length != rhsName->length )
	{
		return false;
	}
	else if( lhsName->length == 0 )
	{
		return true;
	}

	return memcmp( lhsName->data, rhsName->data, lhsName->length ) == 0;
}
