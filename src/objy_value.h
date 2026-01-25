#pragma once

#include "objy/objy.h"

#include "tiki_pool.h"

typedef struct ObjyTypeCollection ObjyTypeCollection;

typedef struct ObjyValueField
{
	TikiStringView		name;
	ObjyValue*			value;
} ObjyValueField;

typedef struct ObjyValueStructData
{
	const ObjyType*		structType;
	ObjyValueField*		values;
	uintsize			valueCount;
	uintsize			valueCapacity;
} ObjyValueStructData;

typedef struct ObjyValueArrayData
{
	const ObjyType*		elementType;
	ObjyValue**			values;
	uintsize			valueCount;
	uintsize			valueCapacity;
} ObjyValueArrayData;

typedef union ObjyValueData
{
	ObjyId				id;
	bool				b;		// bool
	uint64				uint;	// unsigned int
	sint64				sint;	// signed int
	double				fp;		// floating point
	ObjyValueStructData	struc;	// struct
	ObjyValueArrayData	arr;	// array
	TikiStringView		str;	// string
	ObjyValue*			ref;	// reference
} ObjyValueData;

struct ObjyValue
{
	uint64				index;
	ObjyTypeKind		kind;
	ObjyValueData		data;
};

typedef struct ObjyValueStorage
{
	TikiAllocator*		allocator;

	TikiPool			pool;
} ObjyValueStorage;

void		objyValueStorageConstruct( ObjyValueStorage* storage, TikiAllocator* allocator );
void		objyValueStorageDestruct( ObjyValueStorage* storage );

ObjyValue*	objyValueStorageAllocate( ObjyValueStorage* storage, ObjyTypeKind typeKind );
void		objyValueStorageFree( ObjyValueStorage* storage, ObjyValue* value );

bool		objyValueCopyData( ObjyContext* context, ObjyValue* targetValue, const ObjyValue* sourceValue );
