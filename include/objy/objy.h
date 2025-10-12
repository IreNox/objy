#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ObjySystem ObjySystem;
typedef struct ObjyMutex ObjyMutex;
typedef struct ObjyContext ObjyContext;
typedef struct ObjyType ObjyType;
typedef struct ObjyObject ObjyObject;
typedef struct ObjyValue ObjyValue;
typedef struct ObjyChangeSet ObjyChangeSet;

typedef struct ObjyId
{
	uint32_t			data1;
	uint16_t			data2;
	uint16_t			data3;
	uint8_t				data4[ 8 ];
} ObjyId;

static const ObjyId InvalidObjectId = { 0, 0, 0, { 0, 0, 0, 0 } };

bool objyIdIsValid( ObjyId id );

typedef struct ObjyBlob
{
	const void*			data;
	size_t				size;
} ObjyBlob;

typedef void* (*ObjyAllocatorMallocFunc)( size_t size, void* userData );
typedef void* (*ObjyAllocatorReallocFunc)( void* memory, size_t oldSize, size_t newSize, void* userData );
typedef void (*ObjyAllocatorFreeFunc)( void* memory, void* userData );

typedef struct ObjyAllocator
{
	ObjyAllocatorMallocFunc		mallocFunc;		// set to NULL to use default malloc/free
	ObjyAllocatorReallocFunc	reallocFunc;	// can be NULL
	ObjyAllocatorFreeFunc		freeFunc;
	void*						userData;
} ObjyAllocator;

//typedef ObjyMutex* (*ObjyMutexCreateFunc)();
//typedef void (*ObjyMutexDestryFunc)( ObjyMutex* mutex );
//typedef void (*ObjyMutexLock)( ObjyMutex* mutex );
//typedef void (*ObjyMutexUnlock)( ObjyMutex* mutex );
//
//typedef struct ObjyMutexDefinition
//{
//	ObjyMutexCreateFunc	createFunc;
//	ObjyMutexDestryFunc	destroyFunc;
//	ObjyMutexLock		lockFunc;
//	ObjyMutexUnlock		unlockFunc;
//} ObjyMutexDefinition;

typedef ObjyBlob (*ObjyObjectSerializeFunc)( const ObjyObject* object, void* userData );
typedef bool (*ObjyObjectDeserializeFunc)( ObjyObject* object, ObjyValue* objectRootValue, ObjyBlob data, void* userData );
typedef ObjyId (*ObjyObjectBlobGetIdFunc)( ObjyBlob data, void* userData );
typedef void (*ObjyObjectBlobFree)( ObjyBlob blob, void* userData );

typedef struct ObjyObjectFormatter
{
	ObjyObjectSerializeFunc		serializeFunc;
	ObjyObjectDeserializeFunc	deserializeFunc;
	ObjyObjectBlobGetIdFunc		blobGetId;
	ObjyObjectBlobFree			freeFunc;
	void*						userData;
} ObjyObjectFormatter;

typedef ObjyBlob (*ObjyChangeSetSerializeFunc)( ObjyChangeSet* chnageSet, void* userData );
typedef bool (*ObjyChangeSetDeserializeFunc)( ObjyChangeSet* chnageSet, ObjyBlob data, void* userData );
typedef void (*ObjyChangeSetBlobFree)( ObjyBlob blob, void* userData );

typedef struct ObjyChangeSetFormatter
{
	ObjyChangeSetSerializeFunc		serializeFunc;
	ObjyChangeSetDeserializeFunc	deserializeFunc;
	ObjyChangeSetBlobFree			freeFunc;
	void*							userData;
} ObjyChangeSetFormatter;

typedef struct ObjyParameters					// Fill with zero for default parameters
{
	ObjyAllocator		allocator;				// Override memory Allocator. Default use malloc/free
	//ObjyMutexDefinition	mutex;
} ObjyParameters;

// Base

ObjySystem*				objyCreate( const ObjyParameters* parameters );
void					objyDestroy( ObjySystem* system );

ObjyContext*			objectGetRootContext( ObjySystem* system );
ObjyContext*			objectGetContext( ObjyObject* object );

ObjyContext*			objyContextCreate( ObjySystem* system, ObjyObject* parentObject );
void					objyContextDestroy( ObjyContext* context );

// Types

typedef enum ObjyTypeKind
{
	ObjyTypeKind_Id,
	ObjyTypeKind_Bool,
	ObjyTypeKind_Integer,
	ObjyTypeKind_Float,
	ObjyTypeKind_String,
	ObjyTypeKind_Struct,
	ObjyTypeKind_Array,
	ObjyTypeKind_Optional,
	ObjyTypeKind_Variant
} ObjyTypeKind;

typedef struct ObjyTypeField
{
	const char*			name;
	const ObjyType*		type;
} ObjyTypeField;

ObjyType*				objyTypeCreateValue( ObjySystem* system, const char* name, ObjyTypeKind kind, size_t bitCount, bool signedInt, void* userData );	// bitCount: only for ObjyTypeKind_Integer and ObjyTypeKind_Float, signedInt: only for ObjyTypeKind_Integer
ObjyType*				objyTypeCreateStruct( ObjySystem* system, const char* name, const ObjyTypeField* fields, size_t fieldCount, void* userData );
ObjyType*				objyTypeCreateStructInherited( ObjySystem* system, const char* name, ObjyType* baseType, const ObjyTypeField* fields, size_t fieldCount, void* userData );
ObjyType*				objyTypeCreateArray( ObjySystem* system, ObjyType* baseType, void* userData );
ObjyType*				objyTypeCreateOption( ObjySystem* system, ObjyType* baseType, void* userData );
ObjyType*				objyTypeCreateVariant( ObjySystem* system, ObjyType* baseType, void* userData );
void					objyTypeDestroy( ObjySystem* system, ObjyType* type );

const ObjyType*			objyTypeFind( ObjySystem* system, const char* name );

const char*				objyTypeGetName( const ObjyType* type );
ObjyTypeKind			objyTypeGetKind( const ObjyType* type );
ObjyType*				objyTypeGetBaseType( const ObjyType* type );
void*					objyTypeGetUserData( const ObjyType* type );
const ObjyTypeField*	objyTypeGetStructLocalFields( const ObjyType* type );
size_t					objyTypeGetStructLocalFieldCount( const ObjyType* type );
const ObjyTypeField*	objyTypeGetStructGlobalFields( const ObjyType* type );
size_t					objyTypeGetStructGlobalFieldCount( const ObjyType* type );
size_t					objyTypeGetValueBitCount( const ObjyType* type );

// Object

ObjyObject*				objyObjectCreateDetached( ObjyContext* context, ObjyId id, const char* name, ObjyType* structType/*, ObjyObject* parent*/ );
void					objyObjectDestroy( ObjyContext* context, ObjyObject* object );

ObjyBlob				objyObjectSerialize( ObjyContext* context, const ObjyObject* object, ObjyObjectFormatter* formatter );
bool					objyObjectDeserialize( ObjyContext* context, ObjyBlob data, ObjyObjectFormatter* formatter );
ObjyObject*				objyObjectDeserializeDetached( ObjyContext* context, ObjyBlob data, ObjyObjectFormatter* formatter );

const ObjyObject*		objyObjectFind( ObjyContext* context, ObjyId id );

ObjyId					objyObjectGetId( const ObjyObject* object );
const char*				objyObjectGetName( const ObjyObject* object );
const ObjyType*			objyObjectGetType( const ObjyObject* object );
const ObjyObject*		objyObjectGetParent( const ObjyObject* object );
const ObjyObject*		objyObjectGetFirstChild( const ObjyObject* object );
const ObjyObject*		objyObjectGetNextChild( const ObjyObject* object );
size_t					objyObjectGetChildCount( const ObjyObject* object );
const ObjyObject*		objyObjectFindChildByName( const ObjyObject* object, const char* childName );

ObjyValue*				objyObjectGetValueWritable( ObjyObject* object );
const ObjyValue*		objyObjectGetValue( const ObjyObject* object );
const ObjyValue*		objyObjectFindValue( const ObjyObject* object, const char* path );

// Value

ObjyValue*				objyValueCreateId( ObjyContext* context, ObjyId newValue, const ObjyType* idType );
ObjyValue*				objyValueCreateBool( ObjyContext* context, bool newValue, const ObjyType* boolType );
ObjyValue*				objyValueCreateUInt( ObjyContext* context, uint64_t newValue, const ObjyType* intType );
ObjyValue*				objyValueCreateSInt( ObjyContext* context, int64_t newValue, const ObjyType* intType );
ObjyValue*				objyValueCreateFloat( ObjyContext* context, double newValue, const ObjyType* floatType );
ObjyValue*				objyValueCreateString( ObjyContext* context, const char* newValue, const ObjyType* stringType );
ObjyValue*				objyValueCreateStruct( ObjyContext* context, const ObjyType* structType );
ObjyValue*				objyValueCreateArray( ObjyContext* context, const ObjyType* arrayType );
ObjyValue*				objyValueCreateOptional( ObjyContext* context, const ObjyType* optionalType );
ObjyValue*				objyValueCreateVariant( ObjyContext* context, const ObjyType* variantType );
ObjyValue*				objyValueCreateCopy( ObjyContext* context, ObjyValue* value );

void					objyValueDestroy( ObjyContext* context, ObjyValue* value );

const ObjyType*			objyValueGetType( const ObjyValue* value );

ObjyId					objyValueReadId( const ObjyValue* value );
bool					objyValueReadBool( const ObjyValue* value );
uint64_t				objyValueReadUInt( const ObjyValue* value );
int64_t					objyValueReadSInt( const ObjyValue* value );
double					objyValueReadFloat( const ObjyValue* value );
const char*				objyValueReadString( const ObjyValue* value );
const ObjyValue*		objyValueReadStructField( const ObjyValue* value, const char* fieldName );
const ObjyValue*		objyValueReadStructFieldIndex( const ObjyValue* value, size_t index );
const char*				objyValueReadStructFieldIndexName( const ObjyValue* value, size_t index );
size_t					objyValueReadStructFieldCount( const ObjyValue* value );
const ObjyValue*		objyValueReadArray( const ObjyValue* value, size_t index );
size_t					objyValueReadArrayCount( const ObjyValue* value );
const ObjyValue*		objyValueReadOptional( const ObjyValue* value );
const ObjyValue*		objyValueReadVariant( const ObjyValue* value );
const ObjyType*			objyValueReadVariantType( const ObjyValue* value );

bool					objyValueWriteId( ObjyContext* context, ObjyValue* value, ObjyId newValue );
bool					objyValueWriteBool( ObjyContext* context, ObjyValue* value, bool newValue );
bool					objyValueWriteUInt( ObjyContext* context, ObjyValue* value, uint64_t newValue );
bool					objyValueWriteSInt( ObjyContext* context, ObjyValue* value, int64_t newValue );
bool					objyValueWriteFloat( ObjyContext* context, ObjyValue* value, double newValue );
bool					objyValueWriteString( ObjyContext* context, ObjyValue* value, const char* newValue );
bool					objyValueWriteStructField( ObjyContext* context, ObjyValue* value, const char* fieldName, ObjyValue* newValue );
bool					objyValueWriteArray( ObjyContext* context, ObjyValue* value, size_t index, ObjyValue* newValue );
bool					objyValueWriteOptional( ObjyContext* context, ObjyValue* value, ObjyValue* newValue );
bool					objyValueWriteVariant( ObjyContext* context, ObjyValue* value, ObjyValue* newValue );

// Change

ObjyChangeSet*			objyChangeSetBegin( ObjyContext* context );
ObjyChangeSet*			objyChangeSetBeginDetached( ObjyContext* context );
void					objyChangeSetDiscard( ObjyChangeSet* changeSet );
bool					objyChangeSetSubmit( ObjyChangeSet* changeSet );

ObjyBlob				objyChangeSetSerialize( ObjyContext* context, ObjyObject* object, ObjyChangeSetFormatter* formatter );
ObjyChangeSet*			objyChangeSetDeserialize( ObjyContext* context, ObjyBlob data, ObjyChangeSetFormatter* formatter );
bool					objyChangeSetDeserializeMerge( ObjyChangeSet* changeSet, ObjyBlob data, ObjyChangeSetFormatter* formatter );

void					objyChangeSetObjectCreate( ObjyChangeSet* changeSet, ObjyId id, ObjyType* structType );
void					objyChangeSetObjectDelete( ObjyChangeSet* changeSet, ObjyObject* object );
void					objyChangeSetObjectMove( ObjyChangeSet* changeSet, ObjyObject* object, ObjyObject* newParent );

void					objyChangeSetWriteId( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, ObjyId value );
void					objyChangeSetWriteBool( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, bool value );
void					objyChangeSetWriteUInt( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, uint64_t value );
void					objyChangeSetWriteSInt( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, int64_t value );
void					objyChangeSetWriteFloat( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, double value );
void					objyChangeSetWriteString( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const char* value );
void					objyChangeSetWriteArray( ObjyChangeSet* changeSet, ObjyObject* object, const char* path );
void					objyChangeSetWriteOptional( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, bool isSet );
void					objyChangeSetWriteVariant( ObjyChangeSet* changeSet, ObjyObject* object, const char* path, const ObjyType* type );

#ifdef __cplusplus
}
#endif
