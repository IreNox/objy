#include "objy_value.h"

#include "tiki_types.h"

void objyValueStorageFreeValueData( ObjyValueStorage* storage, ObjyValue* value );

void objyValueStorageConstruct( ObjyValueStorage* storage, TikiAllocator* allocator )
{
	storage->allocator	= allocator;

	tikiPoolConstruct( &storage->pool, allocator, sizeof( ObjyValue ), 1024 );
}

void objyValueStorageDestruct( ObjyValueStorage* storage )
{
	tikiPoolDestruct( &storage->pool );

	storage->allocator	= NULL;
}

ObjyValue* objyValueStorageAllocate( ObjyValueStorage* storage, ObjyTypeKind typeKind )
{
	ObjyValue* value;
	const uint64 index = tikiPoolAllocateZero( &storage->pool, &value );
	if( index == TIKI_POOL_INVALID_INDEX )
	{
		return NULL;
	}

	value->index	= index;
	value->kind		= typeKind;

	return value;
}

void objyValueStorageFree( ObjyValueStorage* storage, ObjyValue* value )
{
	if( !value )
	{
		return;
	}

	objyValueStorageFreeValueData( storage, value );
	tikiPoolFree( &storage->pool, value->index );
}

void objyValueStorageFreeValueData( ObjyValueStorage* storage, ObjyValue* value )
{
	if( value->kind == ObjyTypeKind_Struct &&
		value->data.struc.values )
	{
		ObjyValueStructData* structData = &value->data.struc;

		for( uintsize i = 0; i < structData->valueCount; ++i )
		{
			objyValueStorageFree( storage, structData->values[ i ].value );
		}

		tikiMemoryFree( storage->allocator, structData->values );

		structData->values			= NULL;
		structData->valueCount		= 0;
		structData->valueCapacity	= 0;
	}
	else if( value->kind == ObjyTypeKind_Array &&
		value->data.arr.values )
	{
		ObjyValueArrayData* arrayData = &value->data.arr;

		for( uintsize i = 0; i < arrayData->valueCount; ++i )
		{
			objyValueStorageFree( storage, arrayData->values[ i ] );
		}

		tikiMemoryFree( storage->allocator, arrayData->values );

		arrayData->values			= NULL;
		arrayData->valueCount		= 0;
		arrayData->valueCapacity	= 0;
	}
	else if( value->kind == ObjyTypeKind_String )
	{
		tikiMemoryFree( storage->allocator, value->data.str.data );

		value->data.str.data	= NULL;
		value->data.str.length	= 0;
	}
	else if( value->kind == ObjyTypeKind_Reference )
	{
		objyValueStorageFree( storage, value->data.ref.value );

		value->data.ref.typeId	= ObjyIdInvalid;
		value->data.ref.value	= NULL;
	}
}

ObjyValue* objyValueCreate( ObjyContext* context, const ObjyType* type )
{
	return objyValueStorageAllocate( &context->values, type->kind );
}

ObjyValue* objyValueCreateId( ObjyContext* context, ObjyId newValue )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, ObjyTypeKind_Id );
	if( !value )
	{
		return NULL;
	}

	value->data.id = newValue;

	return value;
}

ObjyValue* objyValueCreateBool( ObjyContext* context, bool newValue )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, ObjyTypeKind_Bool );
	if( !value )
	{
		return NULL;
	}

	value->data.b = newValue;

	return value;
}

ObjyValue* objyValueCreateUInt( ObjyContext* context, uint64_t newValue )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, ObjyTypeKind_Integer );
	if( !value )
	{
		return NULL;
	}

	value->data.uint = newValue;

	return value;
}

ObjyValue* objyValueCreateSInt( ObjyContext* context, int64_t newValue )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, ObjyTypeKind_Integer );
	if( !value )
	{
		return NULL;
	}

	value->data.sint = newValue;

	return value;
}

ObjyValue* objyValueCreateFloat( ObjyContext* context, double newValue )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, ObjyTypeKind_Float );
	if( !value )
	{
		return NULL;
	}

	value->data.fp = newValue;

	return value;
}

ObjyValue* objyValueCreateString( ObjyContext* context, const char* newValue )
{
	return objyValueCreateStringLength( context, newValue, strlen( newValue ) );
}

ObjyValue* objyValueCreateStringLength( ObjyContext* context, const char* newValue, size_t stringLength )
{
	ObjyValue* value = objyValueStorageAllocate( &context->values, ObjyTypeKind_String );
	if( !value )
	{
		return NULL;
	}

	if( !objyValueWriteStringLength( context, value, newValue, stringLength ) )
	{
		objyValueStorageFree( &context->values, value );
		return NULL;
	}

	return value;
}

ObjyValue* objyValueCreateStruct( ObjyContext* context )
{
	return objyValueStorageAllocate( &context->values, ObjyTypeKind_Struct );
}

ObjyValue* objyValueCreateArray( ObjyContext* context )
{
	return objyValueStorageAllocate( &context->values, ObjyTypeKind_Array );
}

ObjyValue* objyValueCreateReference( ObjyContext* context, const ObjyType* refType, ObjyValue* refValue )
{
	if( !refType || !refValue )
	{
		return NULL;
	}

	ObjyValue* value = objyValueStorageAllocate( &context->values, ObjyTypeKind_Reference );
	if( !value )
	{
		return NULL;
	}

	value->data.ref.typeId	= refType->object->id;
	value->data.ref.value	= value;

	return value;
}

ObjyValue* objyValueCreateCopy( ObjyContext* context, const ObjyValue* valueToCopy )
{
	if( !valueToCopy )
	{
		return NULL;
	}

	ObjyValue* value = objyValueStorageAllocate( &context->values, valueToCopy->kind );
	if( !value )
	{
		return NULL;
	}

	if( !objyValueCopyData( context, value, valueToCopy ) )
	{
		objyValueStorageFree( &context->values, value );
		return NULL;
	}

	return value;
}

bool objyValueCopyData( ObjyContext* context, ObjyValue* targetValue, const ObjyValue* sourceValue )
{
	objyValueStorageFreeValueData( &context->values, targetValue );

	bool result = false;
	switch( sourceValue->kind )
	{
	case ObjyTypeKind_Id:
		result = objyValueWriteId( context, targetValue, sourceValue->data.id );
		break;

	case ObjyTypeKind_Bool:
		result = objyValueWriteBool( context, targetValue, sourceValue->data.b );
		break;

	case ObjyTypeKind_Integer:
		result = objyValueWriteSInt( context, targetValue, sourceValue->data.sint );
		break;

	case ObjyTypeKind_Float:
		result = objyValueWriteFloat( context, targetValue, sourceValue->data.fp );
		break;

	case ObjyTypeKind_String:
		result = objyValueWriteStringLength( context, targetValue, sourceValue->data.str.data, sourceValue->data.str.length );
		break;

	case ObjyTypeKind_Struct:
		{
			if( TIKI_MEMORY_ARRAY_CHECK_CAPACITY_ZERO( context->allocator, targetValue->data.struc.values, targetValue->data.struc.valueCapacity, sourceValue->data.struc.valueCount ) )
			{
				return false;
			}

			targetValue->data.struc.valueCount = sourceValue->data.struc.valueCount;

			for( size_t i = 0; i < sourceValue->data.struc.valueCount; ++i )
			{
				targetValue->data.struc.values[ i ].name	= sourceValue->data.struc.values[ i ].name;
				targetValue->data.struc.values[ i ].value	= objyValueCreateCopy( context, sourceValue->data.struc.values[ i ].value );

				if( sourceValue->data.struc.values[ i ].value &&
					!targetValue->data.struc.values[ i ].value )
				{
					result = false;
					break;
				}
			}
		}
		break;

	case ObjyTypeKind_Array:
		{
			if( TIKI_MEMORY_ARRAY_CHECK_CAPACITY_ZERO( context->allocator, targetValue->data.arr.values, targetValue->data.arr.valueCapacity, sourceValue->data.arr.valueCount ) )
			{
				return false;
			}

			targetValue->data.arr.valueCount = sourceValue->data.arr.valueCount;

			for( size_t i = 0; i < sourceValue->data.arr.valueCount; ++i )
			{
				targetValue->data.arr.values[ i ] = objyValueCreateCopy( context, sourceValue->data.arr.values[ i ] );

				if( sourceValue->data.arr.values[ i ] &&
					!targetValue->data.arr.values[ i ] )
				{
					result = false;
					break;
				}
			}
		}
		break;

	case ObjyTypeKind_Reference:
		if( sourceValue->data.ref.value )
		{
			result = objyValueWriteReference( context, targetValue, sourceValue->data.ref.typeId, objyValueCreateCopy( context, sourceValue->data.ref.value ) );
		}
		break;

	default:
		break;
	}

	if( !result )
	{
		return false;
	}

	return true;
}

void objyValueDestroy( ObjyContext* context, ObjyValue* value )
{
	objyValueStorageFree( &context->values, value );
}

ObjyTypeKind objyValueGetKind( const ObjyValue* value )
{
	if( !value )
	{
		return (ObjyTypeKind)-1;
	}

	return value->kind;
}

ObjyId objyValueReadId( const ObjyValue* value )
{
	if( !value )
	{
		return ObjyIdInvalid;
	}
	else if( value->kind != ObjyTypeKind_Id )
	{
		TIKI_DEBUG_WARNING( "Value is not a ID." );
		return ObjyIdInvalid;
	}

	return value->data.id;
}

bool objyValueReadBool( const ObjyValue* value )
{
	if( !value )
	{
		return false;
	}
	else if( value->kind != ObjyTypeKind_Bool )
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
	else if( value->kind != ObjyTypeKind_Integer )
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
	else if( value->kind != ObjyTypeKind_Integer )
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
	else if( value->kind != ObjyTypeKind_Float )
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
	else if( value->kind != ObjyTypeKind_String )
	{
		TIKI_DEBUG_WARNING( "Value is not a string." );
		return NULL;
	}

	return value->data.str.data;
}

const ObjyValue* objyValueReadStructField( const ObjyValue* value, const char* fieldName )
{
	return objyValueReadStructFieldLength( value, fieldName, strlen( fieldName ) );
}

const ObjyValue* objyValueReadStructFieldLength( const ObjyValue* value, const char* fieldName, size_t fieldNameLength )
{
	if( !value )
	{
		return NULL;
	}
	else if( value->kind != ObjyTypeKind_Struct )
	{
		TIKI_DEBUG_WARNING( "Value is not a struct." );
		return NULL;
	}

	const TikiStringView fieldNameView = tikiStringViewCreate( fieldName, fieldNameLength );
	for( uintsize i = 0; i < value->data.struc.valueCount; ++i )
	{
		if( tikiStringViewIsEquals( value->data.struc.values[ i ].name, fieldNameView ) )
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
	else if( value->kind != ObjyTypeKind_Struct )
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
	else if( value->kind != ObjyTypeKind_Struct )
	{
		TIKI_DEBUG_WARNING( "Value is not a struct." );
		return NULL;
	}

	if( index >= value->data.struc.valueCount )
	{
		return NULL;
	}

	return value->data.struc.values[ index ].name.data;
}

size_t objyValueReadStructFieldCount( const ObjyValue* value )
{
	if( !value )
	{
		return 0;
	}
	else if( value->kind != ObjyTypeKind_Struct )
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
	else if( value->kind != ObjyTypeKind_Array )
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
	else if( value->kind != ObjyTypeKind_Array )
	{
		TIKI_DEBUG_WARNING( "Value is not a array." );
		return 0;
	}

	return value->data.arr.valueCount;
}

const ObjyValue* objyValueReadReference( const ObjyValue* value )
{
	if( !value )
	{
		return NULL;
	}
	else if( value->kind != ObjyTypeKind_Reference )
	{
		TIKI_DEBUG_WARNING( "Value is not a reference." );
		return NULL;
	}

	return value->data.ref.value;
}

const ObjyType* objyValueReadReferenceType( const ObjyValue* value )
{
	if( !value )
	{
		return NULL;
	}
	else if( value->kind != ObjyTypeKind_Reference )
	{
		TIKI_DEBUG_WARNING( "Value is not a reference." );
		return NULL;
	}

	return NULL; // value->data.ref.typeId;
}

bool objyValueWriteId( ObjyContext* context, ObjyValue* value, ObjyId newValue )
{
	if( !value || value->kind != ObjyTypeKind_Id )
	{
		return false;
	}

	value->data.id = newValue;
	return true;
}

bool objyValueWriteBool( ObjyContext* context, ObjyValue* value, bool newValue )
{
	if( !value || value->kind != ObjyTypeKind_Bool )
	{
		return false;
	}

	value->data.b = newValue;
	return true;
}

bool objyValueWriteUInt( ObjyContext* context, ObjyValue* value, uint64_t newValue )
{
	if( !value || value->kind != ObjyTypeKind_Integer )
	{
		return false;
	}

	value->data.uint = newValue;
	return true;
}

bool objyValueWriteSInt( ObjyContext* context, ObjyValue* value, int64_t newValue )
{
	if( !value || value->kind != ObjyTypeKind_Integer )
	{
		return false;
	}

	value->data.sint = newValue;
	return true;
}

bool objyValueWriteFloat( ObjyContext* context, ObjyValue* value, double newValue )
{
	if( !value || value->kind != ObjyTypeKind_Float )
	{
		return false;
	}

	value->data.fp = newValue;
	return true;
}

bool objyValueWriteString( ObjyContext* context, ObjyValue* value, const char* string )
{
	return objyValueWriteStringLength( context, value, string, strlen( string ) );
}

bool objyValueWriteStringLength( ObjyContext* context, ObjyValue* value, const char* string, size_t stringLength )
{
	if( !value || value->kind != ObjyTypeKind_String )
	{
		return false;
	}

	char* newString = tikiMemoryAlloc( context->allocator, stringLength + 1u );
	if( !newString )
	{
		return false;
	}

	strcpy( newString, string );

	value->data.str.data	= newString;
	value->data.str.length	= stringLength;
	return true;
}

bool objyValueWriteStructField( ObjyContext* context, ObjyValue* value, const char* fieldName, ObjyValue* newValue )
{
	return objyValueWriteStructFieldLength( context, value, fieldName, strlen( fieldName ), newValue );
}

bool objyValueWriteStructFieldLength( ObjyContext* context, ObjyValue* value, const char* fieldName, size_t fieldNameLength, ObjyValue* newValue )
{
	if( !value || value->kind != ObjyTypeKind_Struct )
	{
		return false;
	}

	//const ObjyType* structType = value->data.struc.structType;
	//const TikiStringView fieldNameView = tikiStringViewCreate( fieldName, fieldNameLength );
	const TikiStringView fieldNameView = tikiStringPoolAddPointer( &context->system->types.stringPool, fieldName );

	//ObjyTypeField* typeField = NULL;
	//for( uintsize i = 0; i < structType->globalFieldCount; ++i )
	//{
	//	if( tikiStringViewIsEquals( tikiStringViewCreateFromPointer( structType->globalFields[ i ].name ), fieldNameView ) != 0 )
	//	{
	//		continue;
	//	}

	//	typeField = &structType->globalFields[ i ];
	//	if( typeField->type->kind != newValue->kind )
	//	{
	//		TIKI_DEBUG_WARNING( "Struct type mismatch. Field '%.*s' has type '%s' but try to set value of type '%s'.", fieldNameLength, fieldName, objyTypeKindGetString( typeField->type->kind ), objyTypeKindGetString( newValue->kind ) );
	//		return false;
	//	}
	//}

	//if( !typeField )
	//{
	//	TIKI_DEBUG_WARNING( "Struct field not found. Field '%.*s' doesn't exists in type '%.*s'.", fieldNameLength, fieldName, TIKI_STRING_VIEW_DATA( structType->name ) );
	//	return false;
	//}

	ObjyValueField* valueField = NULL;
	ObjyValueStructData* structData = &value->data.struc;
	for( uintsize i = 0; i < value->data.struc.valueCount; ++i )
	{
		if( tikiStringViewIsEquals( structData->values[ i ].name, fieldNameView ) )
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

		valueField->name = fieldNameView; // tikiStringViewCreateFromPointer( typeField->name );
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
	if( !value || value->kind != ObjyTypeKind_Array )
	{
		return false;
	}

	//const ObjyType* elementType = value->data.arr.elementType;
	//if( elementType->kind != newValue->kind )
	//{
	//	TIKI_DEBUG_WARNING( "Array type mismatch. Element has type '%s' but array has of type '%s'.", objyTypeKindGetString( newValue->kind ), objyTypeKindGetString( elementType->kind ) );
	//	return false;
	//}

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

bool objyValueWriteReference( ObjyContext* context, ObjyValue* value, ObjyId typeId, ObjyValue* newValue )
{
	if( !value || value->kind != ObjyTypeKind_Reference )
	{
		return false;
	}
	else if( newValue && newValue->kind != ObjyTypeKind_Struct )
	{
		return false;
	}

	//const ObjyType* structType = newValue->data.struc.structType;

	//bool found = false;
	//for( const ObjyType* baseType = structType; baseType != NULL; baseType = baseType->baseType )
	//{
	//	if( baseType != value->type->baseType )
	//	{
	//		continue;
	//	}

	//	found = true;
	//	break;
	//}

	//if( !found )
	//{
	//	TIKI_DEBUG_WARNING( "Reference type mismatch. Value has type '%s' what is not a sub type of type '%s'.", newValue->type->name.data, value->type->baseType->name.data );
	//	return false;
	//}

	if( value->data.ref.value )
	{
		objyValueStorageFree( &context->values, value->data.ref.value );
	}

	value->data.ref.typeId	= typeId;
	value->data.ref.value	= newValue;

	return true;
}
