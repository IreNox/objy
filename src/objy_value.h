#pragma once

#include "objy/objy.h"

#include "tiki_pool.h"

typedef struct ObjyTypeCollection ObjyTypeCollection;

typedef struct ObjyValueStructField
{
	const char*				name;
	ObjyValue*				value;
} ObjyValueStructField;

typedef struct ObjyValueStructData
{
	ObjyValueStructField*	values;
	uintsize				valueCount;
	uintsize				valueCapacity;
} ObjyValueStructData;

typedef struct ObjyValueArrayData
{
	ObjyValue**				values;
	uintsize				valueCount;
	uintsize				valueCapacity;
} ObjyValueArrayData;

typedef union ObjyValueData
{
	ObjyId					id;
	bool					b;		// bool
	uint64					uint;	// unsigned int
	sint64					sint;	// signed int
	double					fp;		// floating point
	ObjyValueStructData		struc;	// struct
	ObjyValueArrayData		arr;	// array
	TikiStringView			str;	// string
	ObjyValue*				opt;	// optional
	ObjyValue*				var;	// variant
} ObjyValueData;

struct ObjyValue
{
	uint64					index;
	const ObjyType*			type;
	ObjyValueData			data;
};

typedef struct ObjyValueStorage
{
	TikiAllocator*			allocator;
	ObjyTypeCollection*		types;

	TikiPool				pool;
} ObjyValueStorage;

void		objyValueStorageConstruct( ObjyValueStorage* storage, ObjyTypeCollection* types, TikiAllocator* allocator );
void		objyValueStorageDestruct( ObjyValueStorage* storage );

ObjyValue*	objyValueStorageAllocate( ObjyValueStorage* storage, const ObjyType* type, ObjyTypeKind requiredTypeKind );
void		objyValueStorageFree( ObjyValueStorage* storage, ObjyValue* value );
