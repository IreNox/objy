#include "objy_value.h"

#include "tiki_types.h"

void objyValueStorageConstruct( ObjyValueStorage* storage, ObjyTypeCollection* types, TikiAllocator* allocator )
{
	storage->allocator	= allocator;
	storage->types		= types;

	tikiPoolConstruct( &storage->pool, allocator, sizeof( ObjyValue ), 1024 );
}

void objyValueStorageDestruct( ObjyValueStorage* storage )
{
	tikiPoolDestruct( &storage->pool );

	storage->types		= NULL;
	storage->allocator	= NULL;
}

ObjyValue* objyValueStorageAllocate( ObjyValueStorage* storage, const ObjyType* type, ObjyTypeKind requiredTypeKind )
{
	if( !type || type->kind != requiredTypeKind )
	{
		return NULL;
	}

	ObjyValue* value;
	const uint64 index = tikiPoolAllocateZero( &storage->pool, &value );
	if( index == TIKI_POOL_INVALID_INDEX )
	{
		return NULL;
	}

	value->index	= index;
	value->type		= type;

	return value;
}

void objyValueStorageFree( ObjyValueStorage* storage, ObjyValue* value )
{
	if( !value )
	{
		return;
	}

	if( value->type->kind == ObjyTypeKind_Struct &&
		value->data.struc.values )
	{
		for( uintsize i = 0; i < value->data.struc.valueCount; ++i )
		{
			objyValueStorageFree( storage, value->data.struc.values[ i ].value );
		}

		tikiMemoryFree( storage->allocator, value->data.struc.values );
	}
	else if( value->type->kind == ObjyTypeKind_Array &&
		value->data.arr.values )
	{
		for( uintsize i = 0; i < value->data.arr.valueCount; ++i )
		{
			objyValueStorageFree( storage, value->data.arr.values[ i ] );
		}

		tikiMemoryFree( storage->allocator, value->data.arr.values );
	}
	else if( value->type->kind == ObjyTypeKind_String )
	{
		tikiMemoryFree( storage->allocator, value->data.str.data );
	}
	else if( value->type->kind == ObjyTypeKind_Optional )
	{
		objyValueStorageFree( storage, value->data.opt );
	}
	else if( value->type->kind == ObjyTypeKind_Variant )
	{
		objyValueStorageFree( storage, value->data.var );
	}

	tikiPoolFree( &storage->pool, value->index );
}

ObjyValue* objyValueCreateId( ObjyContext* context, ObjyId newValue, const ObjyType* idType )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, idType, ObjyTypeKind_Id );
	if( !value )
	{
		return NULL;
	}

	value->data.id = newValue;

	return value;
}

ObjyValue* objyValueCreateBool( ObjyContext* context, bool newValue, const ObjyType* boolType )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, boolType, ObjyTypeKind_Bool );
	if( !value )
	{
		return NULL;
	}

	value->data.b = newValue;

	return value;
}

ObjyValue* objyValueCreateUInt( ObjyContext* context, uint64_t newValue, const ObjyType* intType )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, intType, ObjyTypeKind_Integer );
	if( !value )
	{
		return NULL;
	}

	value->data.uint = newValue;

	return value;
}

ObjyValue* objyValueCreateSInt( ObjyContext* context, int64_t newValue, const ObjyType* intType )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, intType, ObjyTypeKind_Integer );
	if( !value )
	{
		return NULL;
	}

	value->data.sint = newValue;

	return value;
}

ObjyValue* objyValueCreateFloat( ObjyContext* context, double newValue, const ObjyType* floatType )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, floatType, ObjyTypeKind_Float );
	if( !value )
	{
		return NULL;
	}

	value->data.fp = newValue;

	return value;
}

ObjyValue* objyValueCreateString( ObjyContext* context, const char* newValue, const ObjyType* stringType )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, stringType, ObjyTypeKind_String );
	if( !value )
	{
		return NULL;
	}

	if( !objyValueWriteString( context, value, newValue ) )
	{
		objyValueStorageFree( &context->values, value );
		return NULL;
	}

	return value;
}

ObjyValue* objyValueCreateStruct( ObjyContext* context, const ObjyType* structType )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, structType, ObjyTypeKind_Struct );
	if( !value )
	{
		return NULL;
	}

	return value;
}

ObjyValue* objyValueCreateArray( ObjyContext* context, const ObjyType* arrayType )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, arrayType, ObjyTypeKind_Array );
	if( !value )
	{
		return NULL;
	}

	return value;
}

ObjyValue* objyValueCreateOptional( ObjyContext* context, const ObjyType* optionalType )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, optionalType, ObjyTypeKind_Optional );
	if( !value )
	{
		return NULL;
	}

	return value;
}

ObjyValue* objyValueCreateVariant( ObjyContext* context, const ObjyType* variantType )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, variantType, ObjyTypeKind_Struct );
	if( !value )
	{
		return NULL;
	}

	return value;
}

ObjyValue* objyValueCreateCopy( ObjyContext* context, ObjyValue* value )
{
}

void objyValueDestroy( ObjyContext* context, ObjyValue* value )
{
	objyValueStorageFree( &context->values, value );
}

const ObjyType* objyValueGetType( const ObjyValue* value )
{
	if( !value )
	{
		return NULL;
	}

	return value->type;
}

ObjyId objyValueReadId( const ObjyValue* value )
{
	if( !value )
	{
		return InvalidObjectId;
	}
	else if( value->type->kind != ObjyTypeKind_Id )
	{
		TIKI_DEBUG_WARNING( "Value is not a ID." );
		return InvalidObjectId;
	}

	return value->data.id;
}

bool objyValueReadBool( const ObjyValue* value )
{
	if( !value )
	{
		return false;
	}
	else if( value->type->kind != ObjyTypeKind_Bool )
	{
		TIKI_DEBUG_WARNING( "Value is not a bool." );
		return false;
	}

	return value->data.b;
}

uint64_t objyValueReadUInt( const ObjyValue* value )
{
	if( !value )
	{
		return 0;
	}
	else if( value->type->kind != ObjyTypeKind_Integer )
	{
		TIKI_DEBUG_WARNING( "Value is not a integer." );
		return 0;
	}

	return value->data.uint;
}

int64_t objyValueReadSInt( const ObjyValue* value )
{
	if( !value )
	{
		return 0;
	}
	else if( value->type->kind != ObjyTypeKind_Integer )
	{
		TIKI_DEBUG_WARNING( "Value is not a integer." );
		return 0;
	}

	return value->data.sint;
}

double objyValueReadFloat( const ObjyValue* value )
{
	if( !value )
	{
		return 0.0;
	}
	else if( value->type->kind != ObjyTypeKind_Float )
	{
		TIKI_DEBUG_WARNING( "Value is not a float." );
		return 0.0;
	}

	return value->data.fp;
}

const char* objyValueReadString( const ObjyValue* value )
{
	if( !value )
	{
		return NULL;
	}
	else if( value->type->kind != ObjyTypeKind_String )
	{
		TIKI_DEBUG_WARNING( "Value is not a string." );
		return NULL;
	}

	return value->data.str.data;
}

const ObjyValue* objyValueReadStructField( const ObjyValue* value, const char* fieldName )
{
	if( !value )
	{
		return NULL;
	}
	else if( value->type->kind != ObjyTypeKind_Struct )
	{
		TIKI_DEBUG_WARNING( "Value is not a struct." );
		return NULL;
	}

	for( uintsize i = 0; i < value->data.struc.valueCount; ++i )
	{
		if( strcmp( value->data.struc.values[ i ].name, fieldName ) )
		{
			continue;
		}

		return value->data.struc.values[ i ].value;
	}

	return NULL;
}

const ObjyValue* objyValueReadStructFieldIndex( const ObjyValue* value, size_t index )
{
	if( !value )
	{
		return NULL;
	}
	else if( value->type->kind != ObjyTypeKind_Struct )
	{
		TIKI_DEBUG_WARNING( "Value is not a struct." );
		return NULL;
	}

	if( index >= value->data.struc.valueCount )
	{
		return NULL;
	}

	return value->data.struc.values[ index ].value;
}

const char* objyValueReadStructFieldIndexName( const ObjyValue* value, size_t index )
{
	if( !value )
	{
		return NULL;
	}
	else if( value->type->kind != ObjyTypeKind_Struct )
	{
		TIKI_DEBUG_WARNING( "Value is not a struct." );
		return NULL;
	}

	if( index >= value->data.struc.valueCount )
	{
		return NULL;
	}

	return value->data.struc.values[ index ].name;
}

size_t objyValueReadStructFieldCount( const ObjyValue* value )
{
	if( !value )
	{
		return 0;
	}
	else if( value->type->kind != ObjyTypeKind_Struct )
	{
		TIKI_DEBUG_WARNING( "Value is not a struct." );
		return 0;
	}

	return value->data.struc.valueCount;
}

const ObjyValue* objyValueReadArray( const ObjyValue* value, size_t index )
{
	if( !value )
	{
		return NULL;
	}
	else if( value->type->kind != ObjyTypeKind_Array )
	{
		TIKI_DEBUG_WARNING( "Value is not a array." );
		return NULL;
	}

	if( index >= value->data.arr.valueCount )
	{
		return NULL;
	}

	return value->data.arr.values[ index ];
}

size_t objyValueReadArrayCount( const ObjyValue* value )
{
	if( !value )
	{
		return 0;
	}
	else if( value->type->kind != ObjyTypeKind_Array )
	{
		TIKI_DEBUG_WARNING( "Value is not a array." );
		return 0;
	}

	return value->data.arr.valueCount;
}

const ObjyValue* objyValueReadOptional( const ObjyValue* value )
{
	if( !value )
	{
		return NULL;
	}
	else if( value->type->kind != ObjyTypeKind_Optional )
	{
		TIKI_DEBUG_WARNING( "Value is not a optional." );
		return NULL;
	}

	return value->data.opt;
}

const ObjyValue* objyValueReadVariant( const ObjyValue* value )
{
	if( !value )
	{
		return NULL;
	}
	else if( value->type->kind != ObjyTypeKind_Variant )
	{
		TIKI_DEBUG_WARNING( "Value is not a variant." );
		return NULL;
	}

	return value->data.var;
}

const ObjyType* objyValueReadVariantType( const ObjyValue* value )
{
	if( !value )
	{
		return NULL;
	}
	else if( value->type->kind != ObjyTypeKind_Variant )
	{
		TIKI_DEBUG_WARNING( "Value is not a variant." );
		return NULL;
	}

	if( !value->data.var )
	{
		return NULL;
	}

	return value->data.var->type;
}

bool objyValueWriteId( ObjyContext* context, ObjyValue* value, ObjyId newValue )
{
	if( !value || value->type->kind != ObjyTypeKind_Id )
	{
		return false;
	}

	value->data.id = newValue;
	return true;
}

bool objyValueWriteBool( ObjyContext* context, ObjyValue* value, bool newValue )
{
	if( !value || value->type->kind != ObjyTypeKind_Bool )
	{
		return false;
	}

	value->data.b = newValue;
	return true;
}

bool objyValueWriteUInt( ObjyContext* context, ObjyValue* value, uint64_t newValue )
{
	if( !value || value->type->kind != ObjyTypeKind_Integer )
	{
		return false;
	}

	value->data.uint = newValue;
	return true;
}

bool objyValueWriteSInt( ObjyContext* context, ObjyValue* value, int64_t newValue )
{
	if( !value || value->type->kind != ObjyTypeKind_Integer )
	{
		return false;
	}

	value->data.sint = newValue;
	return true;
}

bool objyValueWriteFloat( ObjyContext* context, ObjyValue* value, double newValue )
{
	if( !value || value->type->kind != ObjyTypeKind_Float )
	{
		return false;
	}

	value->data.fp = newValue;
	return true;
}

bool objyValueWriteString( ObjyContext* context, ObjyValue* value, const char* newValue )
{
	if( !value || value->type->kind != ObjyTypeKind_String )
	{
		return false;
	}

	const uintsize stringLength = strlen( newValue );
	char* stringData = tikiMemoryAlloc( context->allocator, stringLength + 1u );
	if( !stringData )
	{
		return false;
	}

	strcpy( stringData, newValue );

	value->data.str.data	= newValue;
	value->data.str.length	= stringLength;
	return true;
}

bool objyValueWriteStructField( ObjyContext* context, ObjyValue* value, const char* fieldName, ObjyValue* newValue )
{
	if( !value || value->type->kind != ObjyTypeKind_Struct )
	{
		return false;
	}

	ObjyTypeField* typeField = NULL;
	for( uintsize i = 0; i < value->type->globalFieldCount; ++i )
	{
		if( strcmp( value->type->globalFields[ i ].name, fieldName ) != 0 )
		{
			continue;
		}

		typeField = &value->type->globalFields[ i ];
		if( typeField->type != newValue->type )
		{
			TIKI_DEBUG_WARNING( "Struct type mismatch. Field '%s' has type '%s' but try to set value of type '%s'.", fieldName, typeField->type->name.data, newValue->type->name.data );
			return false;
		}
	}

	if( !typeField )
	{
		TIKI_DEBUG_WARNING( "Struct field not found. Field '%s' doesn't exists in type '%s'.", fieldName, value->type->name.data );
		return false;
	}

	ObjyValueStructField* valueField = NULL;
	ObjyValueStructData* structData = &value->data.struc;
	for( uintsize i = 0; i < value->data.struc.valueCount; ++i )
	{
		if( strcmp( structData->values[ i ].name, fieldName ) )
		{
			continue;
		}

		valueField = &structData->values[ i ];
		break;
	}

	if( !valueField )
	{
		if( !TIKI_MEMORY_ARRAY_CHECK_CAPACITY_ZERO( context->allocator, structData->values, structData->valueCapacity, structData->valueCount + 1 ) )
		{
			return false;
		}

		valueField = &value->data.struc.values[ value->data.struc.valueCount ];
		value->data.struc.valueCount++;

		valueField->name = typeField->name;
	}
	else
	{
		objyValueStorageFree( &context->values, valueField->value );
	}

	valueField->value = newValue;
	return true;
}

bool objyValueWriteArray( ObjyContext* context, ObjyValue* value, size_t index, ObjyValue* newValue )
{
	if( !value || value->type->kind != ObjyTypeKind_Array )
	{
		return false;
	}

	if( value->type->baseType != newValue->type )
	{
		TIKI_DEBUG_WARNING( "Array type mismatch. Element has type '%s' but array has of type '%s'.", newValue->type->name.data, value->type->baseType->name.data );
		return false;
	}

	ObjyValueArrayData* arrayData = &value->data.arr;
	if( !TIKI_MEMORY_ARRAY_CHECK_CAPACITY_ZERO( context->allocator, arrayData->values, arrayData->valueCapacity, index + 1 ) )
	{
		return false;
	}

	if( arrayData->values[ index ] )
	{
		objyValueStorageFree( &context->values, arrayData->values[ index ] );
	}

	arrayData->values[ index ] = newValue;
	return true;
}

bool objyValueWriteOptional( ObjyContext* context, ObjyValue* value, ObjyValue* newValue )
{
	if( !value || value->type->kind != ObjyTypeKind_Optional )
	{
		return false;
	}

	if( value->type->baseType != newValue->type )
	{
		TIKI_DEBUG_WARNING( "Optional type mismatch. Value has type '%s' but optional has of type '%s'.", newValue->type->name.data, value->type->baseType->name.data );
		return false;
	}

	if( value->data.opt )
	{
		objyValueStorageFree( &context->values, value->data.opt );
	}

	value->data.opt = newValue;
	return true;
}

bool objyValueWriteVariant( ObjyContext* context, ObjyValue* value, ObjyValue* newValue )
{
	if( !value || value->type->kind != ObjyTypeKind_Variant )
	{
		return false;
	}

	bool found = false;
	for( const ObjyType* baseType = newValue->type; baseType != NULL; baseType = baseType->baseType )
	{
		if( baseType != value->type->baseType )
		{
			continue;
		}

		found = true;
		break;
	}

	if( !found )
	{
		TIKI_DEBUG_WARNING( "Variant type mismatch. Value has type '%s' what is not a sub type of type '%s'.", newValue->type->name.data, value->type->baseType->name.data );
		return false;
	}

	if( value->data.var )
	{
		objyValueStorageFree( &context->values, value->data.var );
	}

	value->data.var = newValue;
	return true;
}
